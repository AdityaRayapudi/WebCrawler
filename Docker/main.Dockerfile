FROM ubuntu:latest AS build

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libhiredis-dev \
    libc-ares-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY cpp/ cpp/
COPY main.cpp .
COPY CMakeLists.txt .
COPY config.h.in .

RUN cmake -B build -S . \
    && cmake --build build --config Release

FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    libhiredis-dev \
    libc-ares2 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /app/cpp/Seeds.txt /usr/local/bin/Seeds.txt
COPY --from=build /app/build/crawler /usr/local/bin/crawler

ENTRYPOINT ["/usr/local/bin/crawler"]