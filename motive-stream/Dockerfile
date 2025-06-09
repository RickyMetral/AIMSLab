FROM ubuntu:20.04

# Prevent interactive prompts during install
ARG DEBIAN_FRONTEND=noninteractive

# Set working directory
WORKDIR /vrpn-connection


# Copy libraries and source files
COPY . /vrpn-connection/

# Expose port if needed
EXPOSE 8889/udp

# Install essential packages (bash, build tools, Python, etc.)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    python3 \
    python3-pip \
    git \
    curl \
    wget \
    vim \
    bash \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Default shell when container is run
CMD ["/bin/bash"]
