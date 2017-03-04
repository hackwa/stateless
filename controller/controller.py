#!/usr/bin/env python3

import sys
import os
import hashlib

num_elements = 1
replication_factor = 3
repl_slot = [None] * replication_factor

# Assign a new_host to replication slots
def assign_slot(new_host,host_index):
    if host_index > num_elements:
        raise RuntimeError("Host Index greater than Num Elements")
    hashstring = new_host.encode()
    hash_object = hashlib.sha1(hashstring)
    i = 0
    while True:
        if i >= replication_factor:
            break
        hashstring = hash_object.hexdigest().encode()
        index = int(hashstring.decode()[-4:],16) % num_elements
        hash_object.update(hashstring)
        if num_elements > replication_factor:
            if(index == host_index):
                continue
            if index in repl_slot[:-(replication_factor-i)]:
#                print(index,repl_slot,i)
                continue
        repl_slot[i] = index
        i += 1
    for i in range(0,replication_factor):
        print(repl_slot[i])

assign_slot("stateless1",0)
num_elements = 2
assign_slot("stateless2",1)
num_elements = 3
assign_slot("stateless3",2)
num_elements = 4
assign_slot("stateless4",3)
num_elements = 5
assign_slot("stateless5",4)
print("Redistributing..")
assign_slot("stateless1",0)
assign_slot("stateless2",1)
assign_slot("stateless3",2)
assign_slot("stateless4",3)
assign_slot("stateless4",4)
