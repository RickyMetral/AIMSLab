#!/bin/bash

set -e  # Exit on error

cd ~/AIMSLab/motive-stream
echo "Compiling simple-NatNet.cpp..."
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/dependencies/NatNet/lib #Allows the compiler to find the dynamically linked binaries
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/dependencies/vrpn/ #Allows the compiler to find the dynamically linked binaries
g++ examples/samples/MinimalClient/MinimalClient.cpp -Idependencies/NatNet/include/ -Ldependencies/NatNet/lib/ -lNatNet -o bin/minimal-client
echo "Build complete!"
