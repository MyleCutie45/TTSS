# Linux build/benchmark environment for the seam carving project.
# Used on machines where native Windows executables cannot run (e.g. Windows
# Smart App Control blocks unsigned, locally compiled programs).
#
#   docker build -t ttss-dev docker
#   docker run --rm -it -v "<repo>:/src" ttss-dev bash
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake ninja-build libopencv-dev git python3 python3-numpy \
        python3-matplotlib python3-pil ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
