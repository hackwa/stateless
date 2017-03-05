#!/usr/bin/env python3

import sys
import os
import json
import requests
import socket
import fcntl
import struct
from flask import Flask
from flask import request
from flask import jsonify

app = Flask(__name__)

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

@app.route('/')
def index():
    return "Hello client!"

if __name__ == '__main__':
    register_me()
    app.run(debug=True,port=5001)
