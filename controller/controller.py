#!/usr/bin/env python3

import sys
import os
import hashlib
import requests
import flask
from flask import Flask
from flask import request
from flask import jsonify

app = Flask(__name__)

num_elements = 0
replication_factor = 2
repl_slot = [None] * replication_factor
hostlist = []
status = {}

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

def send_rpc(ip,request):
    None 

def reshard_slots():
    global hostlist
    global repl_slot
    for i in range(0,num_elements):
        assign_slot(hostlist[i],i)
        status[hostlist[i]] = repl_slot[:]
        repl_slot = [None] * replication_factor

@app.route('/')
def index():
    return "Hello, World!"

@app.route('/join',methods=['POST'])
def add_server():
    global num_elements
    global repl_slot
    global status
    print(request.json)
    if not request.json or not 'hostip' in request.json:
        flask.abort(400)
    ip = request.json['hostip']
    print("Discovered new server with IP",ip)
    hostlist.append(ip)
    num_elements += 1
    if(num_elements > replication_factor):
        print("num hosts greater than repl factor..resharding")
        reshard_slots()
    else:
        assign_slot(ip,num_elements-1)
        print("Assigning the server to slots:",repl_slot)
        status[ip] = repl_slot[:]
        repl_slot = [None] * replication_factor
    return "Server added Successfully!"

@app.route('/info',methods=['GET'])
def return_status():
    global status
    return jsonify(status),200

if __name__ == '__main__':
        app.run(debug=True)
"""
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
"""
