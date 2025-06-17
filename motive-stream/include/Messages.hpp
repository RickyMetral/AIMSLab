#include <cstdint>

#ifndef MESSAGES_H
#define MESSAGES_H

typedef struct Header_msg{
    uint32_t seq;
    uint32_t  timestamp;
    char* frame_id;
}Header_msg;

typedef struct Quaternion{
    double x;
    double y;
    double z;
    double w;
}Quaternion;

typedef struct Point{
    double x;
    double y;
    double z;
}Point;

typedef struct Vector_3d{
    double x;
    double y;
    double z;
}Vector_3d;

typedef struct Vector_2d{
    double x;
    double y;
}Vector_2d;

typedef struct Pose_msg{
   Quaternion quaternion;
   Point point; 
}Pose_msg;

typedef struct Twist_msg{
    Vector_3d linear;
    Vector_3d angular;
}Twist_msg;

typedef struct Accel_msg{
    Vector_3d linear;
    Vector_3d angular;
}Accel_msg;

#endif //MESSAGES_H