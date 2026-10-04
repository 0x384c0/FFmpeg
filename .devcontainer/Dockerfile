FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        binutils-mingw-w64-x86-64 \
        build-essential \
        ca-certificates \
        gcc-mingw-w64-x86-64 \
        g++-mingw-w64-x86-64 \
        git \
        mingw-w64-x86-64-dev \
        nasm \
        pkg-config \
        yasm \
    && rm -rf /var/lib/apt/lists/*

ENV CC=x86_64-w64-mingw32-gcc \
    CXX=x86_64-w64-mingw32-g++ \
    AR=x86_64-w64-mingw32-ar \
    RANLIB=x86_64-w64-mingw32-ranlib \
    STRIP=x86_64-w64-mingw32-strip
# Pre-create the volume mount point so Docker seeds the volume with vscode ownership
RUN mkdir -p /home/vscode/.claude \
    && chown vscode:vscode /home/vscode/.claude
