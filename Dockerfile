FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/sushitrack

COPY environment.yml .
RUN pip3 install --no-cache-dir numpy pandas scipy pytest

COPY . /workspace/sushitrack/

# Aliaslar
RUN echo "alias ds='/workspace/sushitrack/.tools/sushitrack.sh'" >> ~/.bashrc && \
    echo "alias sushitrack='/workspace/sushitrack/.tools/sushitrack.sh'" >> ~/.bashrc

CMD ["/bin/bash"]
