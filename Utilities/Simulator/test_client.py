#!/usr/bin/python3

import socket

TCP_IP = '127.0.0.1'
TCP_PORT = 8080
BUFFER_SIZE = 1024



def str_to_vector(s):
    data = map(float, s.split(","))
    return list(data)


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
while True:
    data = s.recv(BUFFER_SIZE)
    
    raw = data.decode("ascii") 
    
    pos, vel, acc, rot = map(str_to_vector, raw.split(":"))
    
    print(pos, vel, acc, rot)
    
    
s.close()


