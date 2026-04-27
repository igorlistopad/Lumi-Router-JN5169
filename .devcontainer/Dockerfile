FROM ubuntu:24.04

# Update package manager and install dependencies
# Remove apt cache to reduce image size
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    wget \
    make \
    bzip2 \
    python3 \
    python3-pip \
    python3-venv \
    openssh-client \
    && rm -rf /var/lib/apt/lists/*

# Create virtual environment for Python
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

# Install Python dependencies for SDK
RUN pip3 install --no-cache-dir xmltodict pycryptodome