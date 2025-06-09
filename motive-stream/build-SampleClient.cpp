#!/bin/bash

set -e  # Exit on error

echo "Compiling simple-NatNet.cpp..."
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/dependencies/NatNet/lib
g++ examples/samples/SampleClient/SampleClient.cpp -Idependencies/NatNet/include -Ldependencies/NatNet/lib/ -lNatNet -o sample-client
echo "Build complete!"
