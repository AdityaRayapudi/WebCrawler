FROM ubuntu:latest

RUN apt-get update && apt-get install -y \ cmake \ g++ \ make \ && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY ..

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel
