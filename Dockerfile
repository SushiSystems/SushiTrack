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
    loguru==0.7.3 \
    matplotlib==3.10.9 \
    opencv-python==4.13.0.92 \
    pillow==12.2.0 \
    pycocotools==2.0.11 \
    rich==15.0.0 \
    tqdm==4.67.3 \
    torch==2.5.1 torchvision==0.20.1 torchaudio==2.5.1 --extra-index-url https://download.pytorch.org/whl/cu121

COPY . /workspace/sushitrack/

# `st` and `sushitrack` CLI symlinks for non-interactive and interactive container usage.
RUN chmod +x /workspace/sushitrack/cli/st.sh /workspace/sushitrack/cli/sushitrack.sh && \
    ln -sf /workspace/sushitrack/cli/st.sh /usr/local/bin/st && \
    ln -sf /workspace/sushitrack/cli/sushitrack.sh /usr/local/bin/sushitrack

CMD ["/bin/bash"]

