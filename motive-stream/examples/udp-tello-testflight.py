import socket


def sendMessage(message: str):
        sock.sendto(message.encode(), tello_address)
        print("Sent:", message)

def recvMessage(sock):
        response, ip_address = sock.recvfrom(128)
        print("Received:", response.decode())
# Tello address
tello_address = ('192.168.10.1', 8889)

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


host_ip = "0.0.0.0"
# Bind to a local address (optional)
sock.bind((host_ip, 9000))
message = "command"
sock.sendto(message.encode(), tello_address)
response, ip_address = sock.recvfrom(128)
print("Entered SDK mode")
input("Press Enter To Continue: ")
try:
    sendMessage("takeoff")
    recvMessage(sock)
    while(True):   
        # Send a command
        sendMessage("left 100")
        recvMessage(sock)

        sendMessage("forward 200")
        recvMessage(sock)
        
        sendMessage("right 100")
        recvMessage(sock)

        sendMessage("back 250")
        recvMessage(sock)

        sendMessage("up 100")
        recvMessage(sock)

        sendMessage("down 50")
        recvMessage(sock)

except KeyboardInterrupt:
    # Close the socket (optional)
    sendMessage("land")
    sock.close()

