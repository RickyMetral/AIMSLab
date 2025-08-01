# AIMSLab Docs

This a repository containing most of the code written by me over summer 25 for the additive manufacturing REU with Dr. Baidya and Dr. Aqlan. The repo contains links to the ROS2 package needed to receive mocap data on the Starling 2, how to install the VRPN library to receive mocap data, the python instructions to get the crazyflie flying using mocap and the 3D print anomaly detection model I made.
_________________
# Motive Stream Docs

Using stream files in src:

1. To make a new streaming file copy the motive-pose-stream file and edit the rigid body name to match the rigid body to be tracked. From there you can make any desired changes to the file(i.e chage the serializiaton format, etc.)
2. Go to the top of the Cmake file and change the the project name to the desired name for your binary file and change the SOURCES variable to use the specified streaming file.
3. Follow the instructions below to build your file and the binary will be in the bin folder, which can be run as shown below. (./bin/<binary_file>)

motive-pose-stream.cpp (this is the original stream that Ricky created)

Be aware of the data types in Messages.hpp

Instructions to build stream files:

```
cd motive-stream
mkdir build
cd build
cmake ..
make
```
P.S (after instantaiting the make files, you can also build from anywhere by running: ```cmake --build <path/to/build-dir>/build```

then simply exec the binary:

E.g. ```./bin/motive-pose-stream```

_________________

# BUILDING THE VRPN Library (Linux Only)
[https://github.com/vrpn/vrpn](url)

To get the vrpn libraries do the following: 

```
cd motive-stream
sudo apt-get install libusb-1.0-0-dev libboost-all-dev
mkdir dependencies && cd dependencies
git clone https://github.com/vrpn/vrpn.git && cd vrpn
mkdir build && cd build
cmake ..
make -j$(nproc) 
sudo make install  
```
Unfortunately, I was only able to get the library working if you install the object files system wide. That may be an improvement to make in the future. 
__________________

# CrazyFlie Setup
***Disclaimer: This script has only been tested in Ubuntu so Windows will most likely not work***
1. Install From Source the cfclient: [https://www.bitcraze.io/documentation/repository/crazyflie-clients-python/master/installation/install/](url)
2. 3. In the crazyflie-clients-python directory, run:```cd src``` and ```mkdir aimslab```
3. Transfer all the crazyflie files from this repo into the aimslab directory you just made
4. If you haven't already make a python venv in the crazyflie-client=-python directyory using ```python3 -m venv .venv```, activate you venv using: ```source .venv/bin/activate```and run ```pip install -r requirements.txt``` to install dependencies.
5. If for some reason pip installing from the requirements.txt fails, you can use ```pip install motioncapture cflib scipy numpy``` for all necessary dependencies.

To connect to the crazyflie make sure you use theh crazyradio dongle

If you need to dowload the firmware follow these instructions:

  Download the firmware here to change the crazyflie files: https://www.bitcraze.io/documentation/tutorials/getting-started-with-development/
  
  Make sure to download the dependencies to build the firmware:
  https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/building-and-flashing/build/#dependencies
_______________

# m500 Docs:

The modal m500 should automatically be on AIMSnet as a device
To ssh into the drone, use ssh root@192.168.1.83, the password is default for modalai: oelinux123

These are the modalAi docs:
https://docs.modalai.com/
______________

# Starling 2 Docs:
To gain access to the firmware use adb shell with a usb cable or use ssh.
To ssh into the drone, use ssh root@192.168.1.151, the password is default for modalai: oelinux123. The drone is already on AIMSnet so you don't have to worry about setting that up. To ssh into it, make sure your device is also on AIMSnet.

The drone box contains, the battery, a power module for the drone to use instead of batteries, the drone and adapters for the XT connections
The charger is seperate from the drone and is in the cabinet in the support lab. It is in a white box in the top left of the cabinet. BEFORE CHARGING/USING ANY OF THE BATTERIES MAKE SURE TO FOLLOW CONVENTIONS FOR CHARGING/USING LiPo/LiIon BATTERIES.

To view the camera overlays of the drone make sure you are on the AIMSnet wifi. Then open your browser and type in the voxls ip (192.168.1.151). You should see the voxl portal with info on the starling's current state. More info on the portal can be found in Modlai docs.

The Ros2 pkg to send vrpn stream info onto starling is here: [https://github.com/RickyMetral/optitrack](url). It should already be on the starling 2 drone in a workspace named aimslab_ws in the root dir. The rest of the docs for the package will be in the repo. Please refer to ModalAi docs before beginning development on the drone. The learning curve is big, but very necessary. Start with the developer bootcamp in their documentation and go from there. 

View modalai techincal docs:
https://docs.modalai.com/mavlink/

