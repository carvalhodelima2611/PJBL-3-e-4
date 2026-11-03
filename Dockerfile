# Ambiente reproduzivel do compilador Macaronica (versoes fixas: Ubuntu 24.04 + LLVM/MLIR 18)
#   docker build -t macaronica .
#   docker run --rm -it -v "$PWD":/trabalho macaronica make test
#   docker run --rm -it -v "$PWD":/trabalho macaronica make bench
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential bc python3 python3-matplotlib \
        llvm-18 clang-18 mlir-18-tools libmlir-18-dev libomp-18-dev \
        gcc-avr avr-libc binutils-avr simavr avrdude \
        qemu-user gcc-aarch64-linux-gnu libc6-dev-arm64-cross \
    && rm -rf /var/lib/apt/lists/*

# ferramentas do LLVM 18 no PATH com os nomes sem sufixo (clang, opt, lli, mlir-opt...)
ENV PATH=/usr/lib/llvm-18/bin:$PATH

WORKDIR /trabalho
CMD ["make", "test"]
