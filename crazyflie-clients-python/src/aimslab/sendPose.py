import socket 
import logging
import time
import cflib.crtp
from cflib.crazyflie import Crazyflie
from cflib.crtp.crtpstack import CRTPPacket
from cflib.crazyflie.syncCrazyflie import SyncCrazyflie

# URI to the Crazyflie to connect to
uri = 'radio://0/80/2M/E7E7E7E7E7'
host_ip = "0.0.0.0"

CRTP_PORT_LOCALIZATION = 6
EXT_POSE = 8
GENERIC_TYPE = 1


def sendPose(cf, packet_recv):
    pk = CRTPPacket()
    pk.port = CRTP_PORT_LOCALIZATION
    pk.channel = GENERIC_TYPE
    pk.data = bytearray()
    pk.data.append(EXT_POSE)
    pk.data += packet_recv

    cf.sendPose

def main():
    # Initialize the low-level drivers
    cf = Crazyflie()
    cf.open_link(uri)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((host_ip, 10444))
    time.sleep(2)

    try:
        while(True):
            response, ipAddr = sock.recvfrom(1024)            
            sendPose(cf, response)
            print(response)
    except KeyboardInterrupt:
        cf.close_link()

if __name__ == '__main__':
    main()