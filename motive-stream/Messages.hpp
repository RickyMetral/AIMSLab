#include <cstdint>

#ifndef MESSAGES_H
#define MESSAGES_H

struct Header_msg{
    uint32_t seq;
    uint32_t  timestamp;
    char* frame_id;
};

struct Quaternion{
    double x;
    double y;
    double z;
    double w;
};

struct Point{
    double x;
    double y;
    double z;
};

struct Vector_3d{
    double x;
    double y;
    double z;
};

struct Vector_2d{
    double x;
    double y;
};

struct Pose_msg{
   Quaternion quaternion;
   Point point; 
};

struct Twist_msg{
    Vector_3d linear;
    Vector_3d angular;
};

struct Accel_msg{
    Vector_3d linear;
    Vector_3d angular;
};

#endif //MESSAGES_H