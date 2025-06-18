#include "UDPConnection.hpp"
#include "Messages.hpp"


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

int main(int argc, char** argv)
{
    Pose_msg packet;
    UDPConnection udpConnection("192.168.1.113", 8889);
    char buffer[1024];
    while(true){
        udpConnection.receive(buffer, sizeof(packet), 0);
        deserializePoseMsg(packet, buffer);
        std::cout << packet << std::endl;
    }

    return 0;
}
