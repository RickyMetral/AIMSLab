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

__________________

BUILDING THE VRPN Library (Linux Only)

To get the vrpn libraries do the following: 
sudo apt-get install libusb-1.0-0-dev libboost-all-dev
cd dependencies
git clone https://github.com/vrpn/vrpn.git

# Cd back into motive-stream
cd ..

# Create a separate build directory (best practice for CMake builds)
mkdir build && cd build

# Generate Makefiles with CMake
cmake ..
make -j$(nproc) 

# ONLY if you want install libraries locally. The project should still run if not 
sudo make install      

# If the first call to make does not work, do the following:


# Generates libaries for inside the vrpn repo
cd into dependencies/vrpn/build 
cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release # Change to DBUILD_SHARED_LIBDS=ON if you want to dynamically link instead of static
make

# Then run make from motive-stream/build again