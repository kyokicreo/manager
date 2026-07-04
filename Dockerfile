FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-system-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libpqxx-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --target server

EXPOSE 5555

ENTRYPOINT ["/app/build/server"]
CMD ["/app/config/server_config.json"]