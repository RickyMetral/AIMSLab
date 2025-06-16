#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "vrpn_Tracker.h"
#include "vrpn_Connection.h"

auto time_sent = std::chrono::high_resolution_clock::now();
// Callback function for receiving tracker data
void VRPN_CALLBACK handle_tracker(void* userData, const vrpn_TRACKERCB t)
{
    auto time_recv = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(time_recv-time_sent);
    long long milliseconds = duration.count();
    std::cout << "Time since last packet received: "
	      << milliseconds << "ms" << std::endl;
    std::cout << "Tracker Position Euler: "
              << t.pos[0] << ", " << t.pos[1] << ", " << t.pos[2] << std::endl; 
    std::cout << "Tracker Position Quat: "
              << t.quat[0] << ", " << t.quat[1] << ", " << t.quat[2] << ","  << t.quat[3] << std::endl; 
    time_sent = std::chrono::high_resolution_clock::now();
}

void VRPN_CALLBACK handle_twist(void* userData, const vrpn_TRACKERVELCB twist)
{
    auto time_recv = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(time_recv-time_sent);
    long long milliseconds = duration.count();
    std::cout << "Time since last packet received: "
	      << milliseconds << "ms" << std::endl;
    std::cout << "Tracker Velocity"
              << twist.vel[0] << ", " << twist.vel[1] << ", " << twist.vel[2] << std::endl;
    time_sent = std::chrono::high_resolution_clock::now();
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

    // Register callback
    tracker.register_change_handler(nullptr, handle_tracker);

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

    // Now enter main loop to read data
    while (true) {
        connection->mainloop();
        tracker.mainloop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
