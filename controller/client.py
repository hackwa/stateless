#!/usr/bin/env python3

import sys
import os
import json
import requests
import socket
import subprocess
from shutil import copyfile
from shutil import rmtree
import flask

app = flask.Flask(__name__)
running_instances = {}

controller="http://127.0.0.1:5000"
headers={'Content-Type': 'application/json'}

def register_me():
    print("Registering myself with replica controller")
    myhostname=socket.gethostname()
    payload = json.dumps({'hostname':myhostname})
    endpoint = controller + "/join"
    r = requests.post(endpoint,data=payload,headers=headers)
    if r.status_code != 200:
        raise RuntimeError("Unable to register with controller")

def gen_config(port, template="./redis.conf"):
    p = str(port)
    newconfig = p + "/redis_" + p + ".conf"
    if not os.path.exists(template):
        raise RuntimeError("Cannot find template at",template)
    if not os.path.exists(p):
        os.mkdir(p)
    # Overwrite config if exists
    copyfile(template,newconfig)
    with open(newconfig,'a') as conf:
        conf.write("\nport "+p+"\n")
    conf.close()

def start_redis(port):
    p = str(port)
    conf = p + "/redis_" + p + ".conf"
    try:
        retcode = subprocess.call("redis-server "+conf)
        if retcode is not 0:
            print("unale to start",conf)
            return None
        return 1
    except OSError as e:
        print("Execution failed",e)
        return None

def cleanup_redis(port):
    p = str(port)
    if os.path.exists(p):
        rmtree(p)
    killcmd = r"ps aux | grep 'redis-server.*" + port +"'" +\
     r"| grep -v grep | awk '{print $2}' | xargs kill -9"
    subprocess.call(killcmd,shell=True)

@app.route('/')
def index():
    return "Hello client!"

if __name__ == '__main__':
    register_me()
    app.run(debug=True,port=5001)
