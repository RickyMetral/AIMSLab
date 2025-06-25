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

_________________

CrazyFlie Setup

Install From Source the cfclient: https://www.bitcraze.io/documentation/repository/crazyflie-clients-python/master/installation/install/

Then download the firmware here(Only Works on Linux) to change the crazyflie files: https://www.bitcraze.io/documentation/tutorials/getting-started-with-development/

Make sure to download the dependencies to build the firmware:
https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/building-and-flashing/build/#dependencies

To send packets to crazyflie run ```pip intall cflib```


