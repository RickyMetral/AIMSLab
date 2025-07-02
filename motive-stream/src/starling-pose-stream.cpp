    #include <iostream>
    #include <math.h>
    #include <fstream>
    #include <string>
    #include <chrono>
    #include <thread>
    #include "vrpn_Tracker.h"
    #include "vrpn_Connection.h"
    #include "UDPConnection.hpp"
    #include "all/mavlink.h"

    #define MAV_COMP_ID_VISION_POSITION_ESTIMATE 106

    UDPConnection PX4("192.168.1.151", 10443, 14540);

    uint64_t get_time_usec(){
        auto now = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        return static_cast<uint64_t>(us);
    }

    // Callback function for receiving tracker data
    void VRPN_CALLBACK handle_pose(void* userData, const vrpn_TRACKERCB t)
    {
        mavlink_message_t message;
        float x = t.pos[0], y = t.pos[2], z = - t.pos[1];//Motive uses z as vertical axis and the mav frame used FRD frame(Thats why z is neg)
        float q[4] = {t.quat[3], t.quat[0], t.quat[1], t.quat[2]};
        float covariance[21] = {
            1, 0,    0,    0,    0,    0,    // x
                   1, 0,    0,    0,    0,    // y
                          0.01, 0,    0,    0,    // z
                                 0.01, 0,    0,   // roll
                                        0.01, 0,  // pitch
                                               0.01 // yaw
        };

        uint8_t buffer[300];    
        constexpr uint8_t system_id = 1;
        constexpr uint8_t component_id = MAV_COMP_ID_VISION_POSITION_ESTIMATE;
        uint64_t time_usec = get_time_usec();

        float roll  = atan2(2*q[2]*q[0] - 2*q[1]*q[3], 1 - 2*q[2]*q[2] - 2*q[3]*q[3]);
        float pitch = atan2(2*q[1]*q[0] - 2*q[2]*q[3], 1 - 2*q[1]*q[1] - 2*q[3]*q[3]);
        float yaw   = asin(2*q[1]*q[2] + 2*q[3]*q[0]);

        mavlink_msg_odometry_pack(
            system_id, component_id,
            &message,
            time_usec,
            MAV_FRAME_BODY_FRD,//Frame id
            MAV_FRAME_LOCAL_NED, //Veloctiy frame id
            x, y, z, //Local position
            q, //quaternion array
            NAN, NAN, NAN, //vx, vy, vz
            NAN, NAN, NAN, //rollspeed, pitchspeed, yawspeed
            covariance, //Covariance for pose
            covariance, //Covariance for velocity
            0, //Reset counter
            MAV_ESTIMATOR_TYPE_MOCAP,//Estimator tpye
            0 // Quality
        );

        uint16_t len = mavlink_msg_to_send_buffer(buffer, &message);
        PX4.send(buffer, len, 0);

        mavlink_msg_vision_position_estimate_pack(
            system_id, component_id,
            &message,
            time_usec,
            x, y, z, //Local position
            roll, pitch, yaw ,
            covariance, //Covariance for pose
            0 //Reset counter
        );


        len = mavlink_msg_to_send_buffer(buffer, &message);
        PX4.send(buffer, len, 0);
        std::cout << "Quaternions: " << t.quat[0] << " " << t.quat[1] 
            <<  t.quat[2] << " " <<  t.quat[3] << "\n" 
            << "Position: " << t.pos[0] << " " << t.pos[1] << " " << t.pos[2] << std::endl;
    }


    int main()
    {
        // Set your tracker name & VRPN server address
        const std::string tracker_name = "crazyflie";
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
        tracker.register_change_handler(nullptr, &handle_pose);

        std::cout << "Attempting connection to VRPN server at: " << full_address << std::endl;

        // Allow time for connection handshake
        const int initial_wait_iterations = 50;
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
            return -1;
        }
        // Enter main loop to read data
        while (true) {
            connection->mainloop();
            tracker.mainloop();
    
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return 0;
    }
