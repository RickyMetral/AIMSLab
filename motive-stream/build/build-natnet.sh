#!/bin/bash

set -e  # Exit on error

cd ~/AIMSLab/motive-stream
echo "Compiling simple-NatNet.cpp..."
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/dependencies/NatNet/lib
g++ simple-NatNet.cpp -Idependencies/NatNet/include -Ldependencies/NatNet/lib/ -lNatNet -o bin/simple-natnet
echo "Build complete!"
