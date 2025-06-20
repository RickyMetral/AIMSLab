2 different stream files in src:

motive-pose-stream-aze.cpp (this is what Ayan uses for mocap->unity)
motive-pose-stream.cpp (this is the original stream that Ricky created)

Accordingly change the CMakeLists to build the correct .cpp file for your use case

Be aware of the data types in Messages.hpp

------

commands to build:

cmake --build build/

OR
cd build
cmake ..
make
-------

then simply exec the binary

E.g. ./bin/motive-pose-stream