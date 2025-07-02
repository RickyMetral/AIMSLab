#include <iostream>
#include <fstream>
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


std::string serverIp = "127.0.0.1";
UDPConnection udpConnection(serverIp, 10443, 10444);

std::ostream& operator<<(std::ostream& os, const Pose_msg& msg){
    os << "Quaternions: " << msg.quaternion.x << msg.quaternion.y 
        <<  msg.quaternion.y <<  msg.quaternion.w << "\n"
        "Position: " << msg.point.x << msg.point.y << msg.point.z << std::endl;
    return os;
}

void serializePoseMsg(const Pose_msg& packet, char* buffer){
	memcpy(buffer, &packet.quaternion.x, sizeof(packet.quaternion.x));
	memcpy(buffer + sizeof(packet.quaternion.y) , &packet.quaternion.y, sizeof(packet.quaternion.y));
	memcpy(buffer + 2 * sizeof(packet.quaternion.z), &packet.quaternion.z, sizeof(packet.quaternion.z));
	memcpy(buffer + 3 * sizeof(packet.quaternion.w), &packet.quaternion.w, sizeof(packet.quaternion.w));
	memcpy(buffer + 4 * sizeof(packet.point.x), &packet.point.x, sizeof(packet.point.x));
	memcpy(buffer + 5 * sizeof(packet.point.y), &packet.point.y, sizeof(packet.point.y));
	memcpy(buffer + 6 * sizeof(packet.point.z), &packet.point.z, sizeof(packet.point.z));
}

void deserializePoseMsg(Pose_msg& packet, char* buffer){
	memcpy(&packet.quaternion.x, buffer, sizeof(packet.quaternion.x));
	memcpy(&packet.quaternion.y, buffer + sizeof(packet.quaternion.y), sizeof(packet.quaternion.y));
	memcpy(&packet.quaternion.z, buffer + 2 * sizeof(packet.quaternion.z), sizeof(packet.quaternion.z));
	memcpy(&packet.quaternion.w, buffer + 3 * sizeof(packet.quaternion.w), sizeof(packet.quaternion.w));
	memcpy(&packet.point.x, buffer + 4 * sizeof(packet.point.x), sizeof(packet.point.x));
	memcpy(&packet.point.y, buffer + 5 * sizeof(packet.point.y), sizeof(packet.point.y));
	memcpy(&packet.point.z, buffer + 6 * sizeof(packet.point.z), sizeof(packet.point.z));

}
// Callback function for receiving tracker data
void VRPN_CALLBACK handle_pose(void* userData, const vrpn_TRACKERCB t)
{
    Pose_msg pose_data;
    pose_data.quaternion.x = t.quat[0];
    pose_data.quaternion.y = t.quat[1];
    pose_data.quaternion.z = t.quat[2];
    pose_data.quaternion.w = t.quat[3];
    pose_data.point.x = t.pos[0];
    pose_data.point.y = t.pos[1];
    pose_data.point.z = t.pos[2];
    std::cout << pose_data << std::endl;
    char buffer[sizeof(pose_data)];
    serializePoseMsg(pose_data, buffer);
    udpConnection.send(reinterpret_cast<const char*>(buffer), sizeof(pose_data), 0);
}



int main()
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
