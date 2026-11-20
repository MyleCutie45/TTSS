// main.cpp - command-line program "seam_carving".
// Interface (fixed by the team contract, docs/architecture.md section 6):
//   seam_carving --input <in> --output <out> --width <W> [--height <H>]
//                [--mode seq|omp|opt] [--threads <N>] [--stages <list>] [--persistent-dp]
//                [--opt <list>] [--dp-block <K>] [--par-min-pixels <N>] [--csv]
//   --mode opt (M5) = optimized OpenMP version (seam_opt.h); --opt / --dp-block / --par-min-pixels
//   select its optimizations. --stages / --persistent-dp apply to --mode omp only.
// Exit codes: 0 = success, 1 = invalid arguments, 2 = cannot read input / write output.
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

#include <opencv2/imgcodecs.hpp>

#include "seam_common.h"
#include "seam_omp.h"
#include "seam_opt.h"
#include "seam_seq.h"

namespace {

constexpr int kExitBadArgs = 1;
constexpr int kExitIoError = 2;

struct Options {
    std::string input, output;
    int width = -1;            // required
    int height = -1;           // -1 = keep input height
    std::string mode = "seq";
    int threads = -1;          // -1 = OpenMP default (omp mode only)
    std::string stages = "energy,dp,min,removal";
    bool persistentDP = false;
    std::string opt = "incr,simd,compact";  // --mode opt: enabled optimizations ("none" = all off)
    int dpBlock = 64;                // --mode opt: DP block rows (0 = sequential DP; 64 = measured best)
    long long parMinPixels = 0;      // --mode opt: no parallel region below this many pixels
    bool csv = false;
};

void printUsage() {
    std::cerr <<
        "Usage: seam_carving --input <in.png> --output <out.png> --width <W> [--height <H>]\n"
        "                    [--mode seq|omp|opt] [--threads <N>] [--stages energy,dp,min,removal]\n"
        "                    [--persistent-dp] [--opt incr,simd,compact|none] [--dp-block <K>]\n"
        "                    [--par-min-pixels <N>] [--csv]\n";
}

int parseInt(const std::string& name, const std::string& value) {
    size_t used = 0;
    int v = std::stoi(value, &used);  // throws std::invalid_argument / std::out_of_range
    if (used != value.size()) throw std::invalid_argument("bad number for " + name + ": " + value);
    return v;
}

// Returns false (after printing a message) if the arguments are invalid.
bool parseArgs(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto needValue = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument("missing value after " + arg);
            return argv[++i];
        };
        if      (arg == "--input")         opt.input = needValue();
        else if (arg == "--output")        opt.output = needValue();
        else if (arg == "--width")         opt.width = parseInt(arg, needValue());
        else if (arg == "--height")        opt.height = parseInt(arg, needValue());
        else if (arg == "--mode")          opt.mode = needValue();
        else if (arg == "--threads")       opt.threads = parseInt(arg, needValue());
        else if (arg == "--stages")        opt.stages = needValue();
        else if (arg == "--persistent-dp") opt.persistentDP = true;
        else if (arg == "--opt")           opt.opt = needValue();
        else if (arg == "--dp-block")      opt.dpBlock = parseInt(arg, needValue());
        else if (arg == "--par-min-pixels") opt.parMinPixels = std::stoll(needValue());
        else if (arg == "--csv")           opt.csv = true;
        else throw std::invalid_argument("unknown option: " + arg);
    }
    if (opt.input.empty() || opt.output.empty() || opt.width == -1) {
        throw std::invalid_argument("--input, --output and --width are required");
    }
    if (opt.mode != "seq" && opt.mode != "omp" && opt.mode != "opt") {
        throw std::invalid_argument("--mode must be 'seq', 'omp' or 'opt'");
    }
    if (opt.dpBlock < 0) throw std::invalid_argument("--dp-block must be >= 0");
    if (opt.parMinPixels < 0) throw std::invalid_argument("--par-min-pixels must be >= 0");
    if (opt.threads != -1 && opt.threads < 1) {
        throw std::invalid_argument("--threads must be >= 1");
    }
    return true;
}

// "--stages energy,dp,min,removal" -> ParallelConfig switches. "none" switches all stages off
// (useful to measure the OpenMP build without any parallel stage).
ParallelConfig makeConfig(const Options& opt) {
    ParallelConfig cfg;
    cfg.threads = (opt.threads == -1) ? omp_impl::defaultThreadCount() : opt.threads;
    cfg.persistentDP = opt.persistentDP;
    cfg.parEnergy = cfg.parDP = cfg.parMin = cfg.parRemoval = false;
    if (opt.stages == "none") return cfg;

    size_t pos = 0;
    while (pos <= opt.stages.size()) {
        const size_t comma = opt.stages.find(',', pos);
        const std::string name = opt.stages.substr(pos, comma == std::string::npos ? std::string::npos
                                                                                   : comma - pos);
        if      (name == "energy")  cfg.parEnergy = true;
        else if (name == "dp")      cfg.parDP = true;
        else if (name == "min")     cfg.parMin = true;
        else if (name == "removal") cfg.parRemoval = true;
        else throw std::invalid_argument("unknown stage '" + name +
                                         "' (allowed: energy,dp,min,removal or none)");
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return cfg;
}

// "--opt incr,simd,compact" -> OptConfig switches ("none" = all three off).
OptConfig makeOptConfig(const Options& opt) {
    OptConfig cfg;
    cfg.threads = (opt.threads == -1) ? omp_impl::defaultThreadCount() : opt.threads;
    cfg.dpBlockRows = opt.dpBlock;
    cfg.parallelMinPixels = opt.parMinPixels;
    cfg.incrementalEnergy = cfg.simdKernels = cfg.compactMemory = false;
    if (opt.opt == "none") return cfg;
    size_t pos = 0;
    while (pos <= opt.opt.size()) {
        const size_t comma = opt.opt.find(',', pos);
        const std::string name = opt.opt.substr(pos, comma == std::string::npos ? std::string::npos
                                                                               : comma - pos);
        if      (name == "incr") cfg.incrementalEnergy = true;
        else if (name == "simd") cfg.simdKernels = true;
        else if (name == "compact") cfg.compactMemory = true;
        else throw std::invalid_argument("unknown optimization '" + name +
                                         "' (allowed: incr,simd,compact or none)");
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return cfg;
}

// Stage list for the CSV line: joined with '+' (a ',' would break the CSV columns).
std::string stagesForCsv(const ParallelConfig& cfg) {
    std::string s;
    auto add = [&](bool on, const char* name) {
        if (!on) return;
        if (!s.empty()) s += "+";
        s += name;
    };
    add(cfg.parEnergy, "energy");
    add(cfg.parDP, cfg.persistentDP ? "dp_persistent" : "dp");
    add(cfg.parMin, "min");
    add(cfg.parRemoval, "removal");
    return s.empty() ? "none" : s;
}

struct RunInfo {          // what the output functions need to know about the run
    int threads = 1;
    std::string schedule = "none";
    std::string stages = "none";
};

void printCsv(const Options& opt, const RunInfo& run, const cv::Mat& in, const cv::Mat& out,
              const StageTimings& t) {
    // Columns (fixed): mode,threads,schedule,stages,in_w,in_h,out_w,out_h,seams,
    //                  t_energy,t_dp,t_min,t_backtrack,t_removal,t_total
    const int seams = (in.cols - out.cols) + (in.rows - out.rows);
    std::printf("%s,%d,%s,%s,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                opt.mode.c_str(), run.threads, run.schedule.c_str(), run.stages.c_str(),
                in.cols, in.rows, out.cols, out.rows, seams,
                t.energy, t.dp, t.minEndpoint, t.backtrack, t.removal, t.total);
}

void printHuman(const Options& opt, const RunInfo& run, const cv::Mat& in, const cv::Mat& out,
                const StageTimings& t) {
    std::cout << "Mode:        " << opt.mode << "\n";
    if (opt.mode != "seq") {
        std::cout << "Threads:     " << run.threads << "\n"
                  << "Schedule:    " << run.schedule << "\n"
                  << "Parallel:    " << run.stages << "\n";
    }
    std::cout << "Input:       " << opt.input << " (" << in.cols << " x " << in.rows << ")\n"
              << "Output:      " << opt.output << " (" << out.cols << " x " << out.rows << ")\n"
              << "Seams:       " << (in.cols - out.cols) << " vertical, "
              << (in.rows - out.rows) << " horizontal\n"
              << "Timings (s): energy " << t.energy << ", dp " << t.dp
              << ", min " << t.minEndpoint << ", backtrack " << t.backtrack
              << ", removal " << t.removal << "\n"
              << "Total (s):   " << t.total << "  (excluding image load/save)\n";
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    ParallelConfig config;
    OptConfig optConfig;
    RunInfo run;
    try {
        parseArgs(argc, argv, opt);
        if (opt.mode == "omp") {
            omp_impl::useStaticScheduleUnlessSet();
            config = makeConfig(opt);
            run.threads = config.threads;
            run.schedule = omp_impl::scheduleDescription();
            run.stages = stagesForCsv(config);
        } else if (opt.mode == "opt") {
            omp_impl::useStaticScheduleUnlessSet();
            optConfig = makeOptConfig(opt);
            run.threads = optConfig.threads;
            run.schedule = omp_impl::scheduleDescription();
            run.stages = opt_impl::describe(optConfig);   // e.g. "incr+simd+dpblock16+thr0"
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        printUsage();
        return kExitBadArgs;
    }

    // IMREAD_COLOR always gives 3 channels (gray PNGs are expanded), as carve() requires.
    const cv::Mat input = cv::imread(opt.input, cv::IMREAD_COLOR);
    if (input.empty()) {
        std::cerr << "Error: cannot read image '" << opt.input << "'\n";
        return kExitIoError;
    }

    const int targetHeight = (opt.height == -1) ? input.rows : opt.height;
    StageTimings timings;
    cv::Mat result;
    try {
        if (opt.mode == "omp")      result = omp_impl::carve(input, opt.width, targetHeight, config, &timings);
        else if (opt.mode == "opt") result = opt_impl::carve(input, opt.width, targetHeight, optConfig, &timings);
        else                        result = seq::carve(input, opt.width, targetHeight, &timings);
    } catch (const std::invalid_argument& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return kExitBadArgs;
    }

    bool written = false;
    try {
        written = cv::imwrite(opt.output, result);
    } catch (const cv::Exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }
    if (!written) {
        std::cerr << "Error: cannot write image '" << opt.output << "'\n";
        return kExitIoError;
    }

    if (opt.csv) printCsv(opt, run, input, result, timings);
    else         printHuman(opt, run, input, result, timings);
    return 0;
}
