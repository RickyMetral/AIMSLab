import socket

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
try:
    while(True):   
        # Send a command
        message = input("Enter Instruction: ")
        sock.sendto(message.encode(), tello_address)
        print("Sent:", message)

        # Receive response (optional)
        response, ip_address = sock.recvfrom(128)
        print("Received:", response.decode())
except KeyboardInterrupt:
    # Close the socket (optional)
    message = "land"
    sock.sendto(message.encode(), tello_address)
    sock.close()

