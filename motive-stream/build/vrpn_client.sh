#!/bin/bash

set -e  # Exit on error

cd ~/AIMSLab/motive-stream
echo "Compiling simple-vrpn.cpp..."
g++ simple-vrpn.cpp -o bin/vrpn_client -I/usr/local/include -L/usr/local/lib -lvrpn -lquat -lpthread
echo "Build complete!"
