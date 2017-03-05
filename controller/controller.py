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

replication_factor = 2
repl_slot = [None] * replication_factor
hostlist = []
status = {}

# Assign a new_host to replication slots
def assign_slot(new_host,host_index):
    num_elements = len(hostlist)
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

def send_rpc(host,request):
    None 

def reshard_slots():
    global hostlist
    global repl_slot
    for i in range(0,len(hostlist)):
        assign_slot(hostlist[i],i)
        status[hostlist[i]] = repl_slot[:]
        repl_slot = [None] * replication_factor

@app.route('/')
def index():
    return "Hello, World!"

@app.route('/join',methods=['POST'])
def add_server():
    global repl_slot
    global status
    print(request.json)
    if not request.json or not 'hostname' in request.json:
        flask.abort(400)
    host = request.json['hostname']
    print("Discovered new server",host)
    if host in hostlist:
        hostlist.remove(host)
    hostlist.append(host)
# Run reshard only once
    if(len(hostlist) == replication_factor+1):
        print("num hosts greater than repl factor..resharding")
        reshard_slots()
    else:
        assign_slot(host,len(hostlist)-1)
        print("Assigning the server to slots:",repl_slot)
        status[host] = repl_slot[:]
        repl_slot = [None] * replication_factor
    return "Server added Successfully!"

@app.route('/info',methods=['GET'])
def return_status():
    global status
    return jsonify(status),200

if __name__ == '__main__':
        app.run(debug=True)
