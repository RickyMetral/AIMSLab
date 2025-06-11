#!/bin/bash

set -e  # Exit on error
cd ~/AIMSLab/motive-stream
echo "Compiling simple-NatNet.cpp..."
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/dependencies/vrpn/ #Allows the compiler to find the dynamically linked binaries
g++ examples/samples/SampleClient/SampleClient.cpp -Idependencies/vrpn/ -o bin/simple-vrpn
echo "Build complete!"
