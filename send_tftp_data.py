
import socket

src_port = int(input("give me client's port: \n"))
dst_port = int(input("give me server's port: \n"))


s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

s.bind(("127.0.0.1", src_port))

s.sendto("\x00\x02\x00\x05ldkhjfvldvhk", ("127.0.0.1", dst_port))
data, addr = s.recvfrom(1024)
print("server said: " + data)
#sleep(1)
s.close()