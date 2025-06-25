'''Script to receive the UDP stream from Motive VM and sends it to the crazyflie. Keeps the crazyflie external pose updated'''
import threading
import struct
import socket 
import time
import cflib.crtp
from cflib.crazyflie.extpos import Extpos
from cflib.crazyflie.log import LogConfig
from cflib.crazyflie import Crazyflie
from cflib.crtp.crtpstack import CRTPPacket
from cflib.crazyflie.syncCrazyflie import SyncCrazyflie
from cflib.positioning.motion_commander import MotionCommander

# URI to the Crazyflie to connect to
uri = 'radio://0/80/2M/E7E7E7E7E7'
host_ip = "0.0.0.0"

CRTP_PORT_LOCALIZATION = 6
EXT_POSE = 8
GENERIC_TYPE = 1


def simple_takeoff(scf):
    with MotionCommander(scf, default_height=10) as mc:
        time.sleep(2)
        mc.stop()

def add_logs(scf, groupName, paramaterdict: dict):
    """Takes in a dictionary of the paramater(groupname.value) and its dtype(float, int, etc.)"""
    logconf = LogConfig(name=groupName, period_in_ms=10)
    for key, value in paramaterdict.items():
            logconf.add_variable(key, value)
    scf.cf.log.add_config(logconf)
    logconf.data_received_cb.add_callback(log_pos_callback)
    logconf.start()
    return logconf

def pose_stream_thread_packet(scf, sock):
    """Sends the packets to crazyflie by directly sending CRTP Packet"""
    sock.settimeout(0.5) #Sets a timeout for out socket in case data stream stops
    while True:
        response, ipAddr = sock.recvfrom(1024)#Receives UDP packet
        decodePacket(response)
        if len(response) == 29: 
            scf.cf.send_packet(response)
            print("Sent Packet")
        time.sleep(0.01)#Must sleep or else crazyflie cannot hanle the stream

def pose_stream_thread_extpos(extPose, sock):
    """Sends crazyflie pose data using its own API  and unpacking """
    sock.settimeout(0.5) #Sets a timeout for out socket in case data stream stops
    while True:
        response, ipAddr = sock.recvfrom(1024)#Receives UDP packet
        x, y, z, qx, qy, qz, qw = struct.unpack("<7f", response[0:28])
        if len(response) == 29: 
            extPose.send_extpose(x,y,z,qx,qy,qz,qw)
            print("Sent Packet")
        time.sleep(0.01)#Must sleep or else crazyflie cannot hanle the stream


#Logging the received pose info from crazyflie
def log_pos_callback(timestamp, data, logconf):
    print(data)

#Function to decode packet into CRTP format(Not necessary if sending with ExtPose class)
def decodePacket(packet_recv):
    pk = CRTPPacket()
    pk.port = CRTP_PORT_LOCALIZATION
    pk.channel = GENERIC_TYPE
    pk.data = bytearray()
    pk.data.append(EXT_POSE)
    pk.data += packet_recv
    return pk

def main():
    cflib.crtp.init_drivers()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#create UDP socket 
    sock.bind((host_ip, 10444)) #Bind socket to our specified port

    # Syncs to the crazyflie and establishes our connection
    with SyncCrazyflie(uri, Crazyflie(rw_cache="./cache")) as scf:
        try:
            #Logs the statestimate of the drone
            extPose = Extpos(crazyflie = scf.cf)
            scf.cf.param.set_value("kalman.resetEstimation", "1")  # Reset when pose starts
            #asking for crazyflie to send us pose data on callback
            positionLog = add_logs(scf, "Position", { 
                                       "stateEstimate.x" : "float",
                                       "stateEstimate.y" : "float", 
                                        "stateEstimate.z" : "float" })
            #Logs the roll pitch and yaw
            stabilizerLog = add_logs(scf, "Stabilizer", {
                                    "stabilizer.pitch": "float",
                                    "stabilizer.roll": "float",
                                    "stabilizer.yaw": "float"})

            thread = threading.Thread(target=pose_stream_thread_packet, args=(scf, sock), daemon=True)
            thread.start()

            scf.cf.platform.send_arming_request(True)
            time.sleep(1.0)
            simple_takeoff(scf)

        except socket.timeout:
            print("Socket Timed out")
        except KeyboardInterrupt:
            print("Interrupted. Closing.")
        finally:
            scf.cf.param.set_value("stabilizer.estimator", 1)
            sock.close()


if __name__ == '__main__':
    main()