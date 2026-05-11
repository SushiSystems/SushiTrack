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

# Python paketlerini yükle
RUN pip3 install --no-cache-dir \
    numpy==2.4.3 \
    pandas==3.0.2 \
    scipy==1.17.1 \
    pytest==9.0.3 \
    absl-py==2.4.0 \
    loguru==0.7.3 \
    matplotlib==3.10.9 \
    motmetrics==1.4.0 \
    onnx==1.21.0 \
    onnxsim==0.6.3 \
    opencv-python==4.13.0.92 \
    pillow==12.1.1 \
    pycocotools==2.0.11 \
    rich==15.0.0 \
    tensorboard==2.20.0 \
    tqdm==4.67.3 \
    thop==0.1.1.post2209072238 \
    torch==2.5.1 torchvision==0.20.1 torchaudio==2.5.1 --extra-index-url https://download.pytorch.org/whl/cu121


COPY . /workspace/sushitrack/

# Aliaslar
RUN echo "alias ds='/workspace/sushitrack/.tools/sushitrack.sh'" >> ~/.bashrc && \
    echo "alias sushitrack='/workspace/sushitrack/.tools/sushitrack.sh'" >> ~/.bashrc

CMD ["/bin/bash"]

