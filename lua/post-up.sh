#!/bin/bash

redis-cli SCRIPT LOAD "$(cat /path/to/script.lua)" >/dev/null
