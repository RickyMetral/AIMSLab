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

# CrazyFlie Setup

Install From Source the cfclient: https://www.bitcraze.io/documentation/repository/crazyflie-clients-python/master/installation/install/

Then download the firmware here(Only Works on Linux) to change the crazyflie files: https://www.bitcraze.io/documentation/tutorials/getting-started-with-development/

Make sure to download the dependencies to build the firmware:
https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/building-and-flashing/build/#dependencies

To send packets to crazyflie run ```pip intall cflib```

__________________

# BUILDING THE VRPN Library (Linux Only)

To get the vrpn libraries do the following: 
sudo apt-get install libusb-1.0-0-dev libboost-all-dev
cd dependencies && git clone https://github.com/vrpn/vrpn.git && cd vrpn


Create a separate build directory
cd ../../ &&  mkdir build && cd build

Generate Makefiles with CMake
cmake ..
make -j$(nproc) 

ONLY if you want install libraries locally. The project should still run if not 
sudo make install      

_______________

# m500 Docs:

The modal m500 should automatically be on AIMSnet as a device
To ssh into the drone, use ssh root@192.168.1.83, the password is default for modalai: oelinux123

These are the modalAi docs:
https://docs.modalai.com/
______________

# Starling 2 Docs:
To gain access to firmware use adb shell with a usb cable
To ssh into the drone, use ssh root@192.168.1.83, the password is default for modalai: oelinux123

Ros2 pkg to send vrpn stream info onto starling is here: [https://github.com/RickyMetral/optitrack](url)
View modalai techincal docs for specific instructions
https://docs.modalai.com/mavlink/

_________________________________

# How to build with Mavlink:
Clone this header library:
https://github.com/vrpn/vrpn


