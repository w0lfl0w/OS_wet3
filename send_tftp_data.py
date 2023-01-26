
import socket
import time

#src_port = int(input("give me client's port: \n"))
#dst_port = int(input("give me server's port: \n"))

src_port = 49001
dst_port = 49000

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

s.bind(("127.0.0.1", src_port))
s2.bind(("127.0.0.1", src_port+2))

s.sendto("\x00\x02ldkhjfvldvhk", ("127.0.0.1", dst_port))
time.sleep(1)
data, addr = s.recvfrom(1024)
print("server said: " + data)
s.sendto("\x00\x03\x00\x01ldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhk", ("127.0.0.1", dst_port))
time.sleep(1)
data, addr = s.recvfrom(1024)
print("server said: " + data)
s2.sendto("\x00\x02ldkhjfvldvhk", ("127.0.0.1", dst_port))
time.sleep(1)
data, addr = s2.recvfrom(1024)
print("server said: " + data)
s.sendto("\x00\x03\x00\x02ldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhkldkhjfvldvhk", ("127.0.0.1", dst_port))
time.sleep(1)
data, addr = s.recvfrom(1024)
print("server said: " + data)
s.sendto("\x00\x03\x00\x03a", ("127.0.0.1", dst_port))
time.sleep(1)
data, addr = s.recvfrom(1024)
print("server said: " + data)

#s.sendto("\x00\x03\x00\x07ldkhjfvldvhk", ("127.0.0.1", dst_port))
#time.sleep(1)
#data, addr = s.recvfrom(1024)
#print("server said: " + data)
#s.sendto("\x00\x04ldkhjfvldvhk", ("127.0.0.1", dst_port))
#time.sleep(1)
#data, addr = s.recvfrom(1024)
#print("server said: " + data)
#s.sendto("\x00\x05\x00\x03ldkhjfvldvhk", ("127.0.0.1", dst_port))
#time.sleep(1)
#data, addr = s.recvfrom(1024)
#print("server said: " + data)
#\x00\x05
#data, addr = s.recvfrom(1024)
#print("server said: " + data)
#sleep(1)
s.close()
s2.close()