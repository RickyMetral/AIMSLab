#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "vrpn_Tracker.h"
#include "vrpn_Connection.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/stdc++.h> 
#include <sys/types.h> 
#include <arpa/inet.h> 
#include "UDPConnection.hpp"
#include "Messages.hpp"


UDPConnection udpConnection("192.168.1.139", 8889);
// Callback function for receiving tracker data
void VRPN_CALLBACK handle_pose(void* userData, const vrpn_TRACKERCB t)
{
    struct Pose_msg* pose_data;
    pose_data->quaternion.x = t.quat[0];
    pose_data->quaternion.y = t.quat[1];
    pose_data->quaternion.z = t.quat[2];
    pose_data->quaternion.w = t.quat[3];
    pose_data->point.x = t.pos[0];
    pose_data->point.y = t.pos[1];
    pose_data->point.z = t.pos[2];
    std::cout << "Tracker Position: "
              << t.pos[0] << ", " << t.pos[1] << ", " << t.pos[2] << std::endl;
    udpConnection.send(pose_data, sizeof(pose_data), 0);
    
}

int main(int argc, char** argv)
{
    // Set your tracker name & VRPN server address
    const std::string tracker_name = "starscream";
    const std::string server_address = "192.168.1.42:3883";
    const std::string full_address = tracker_name + "@" + server_address;

    // Use vrpn_get_connection_by_name() instead of implicit connection
    vrpn_Connection* connection = vrpn_get_connection_by_name(full_address.c_str());

    if (connection == nullptr) {
        std::cerr << "Failed to create VRPN connection object." << std::endl;
        return -1;
    }

    // Create Tracker client attached to existing connection
    vrpn_Tracker_Remote tracker(full_address.c_str(), connection);

    // Register callback function
    tracker.register_change_handler(nullptr, handle_pose);

    std::cout << "Attempting connection to VRPN server at: " << full_address << std::endl;

    // Allow time for connection handshake
    const int initial_wait_iterations = 100;
    for (int i = 0; i < initial_wait_iterations; i++) {
        connection->mainloop();
        tracker.mainloop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify connection
    if (connection->connected()) {
        std::cout << "Successfully connected to VRPN server." << std::endl;
    } else {
        std::cerr << "Failed to connect to VRPN server after waiting." << std::endl;
        return -2;
    }

    // Enter main loop to read data
    while (true) {
        connection->mainloop();
        tracker.mainloop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}