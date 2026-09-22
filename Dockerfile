FROM nvidia/cuda:12.1.1-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    libgtest-dev \
    libgl1-mesa-glx \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/sushitrack

# Python packages
RUN pip3 install --no-cache-dir \
    numpy==2.4.4 \
    pandas==3.0.2 \
    scipy==1.17.1 \
    pytest==9.0.3 \
    absl-py==2.4.0 \
    matplotlib==3.10.9 \
    opencv-python==4.13.0.92 \
    pillow==12.2.0 \
    pycocotools==2.0.11 \
    rich==15.0.0 \
    tqdm==4.67.3 \
    torch==2.5.1 torchvision==0.20.1 torchaudio==2.5.1 --extra-index-url https://download.pytorch.org/whl/cu121

COPY . /workspace/sushitrack/

# Installs the `st` and `sushitrack` console-script entry points onto PATH
# (pip's default install location for a non-`--user` root install on Debian,
# /usr/local/bin, is already on PATH in this image). No separate symlink step:
# the CLI is a pip package (cli/pyproject.toml), not shell wrapper scripts.
RUN pip3 install --no-cache-dir /workspace/sushitrack/cli

CMD ["/bin/bash"]

