#!/bin/bash

set -e  # Exit on error
cd ~/AIMSLab/motive-stream/src
echo "Compiling simple-NatNet.cpp..."
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/dependencies/vrpn/ #Allows the compiler to find the dynamically linked binaries
g++ simple-vrpn.cpp -o ../bin/vrpn_client -I/usr/local/include -L/usr/local/lib -lvrpn -lquat -lpthread
echo "Build complete!"
