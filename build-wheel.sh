#!/bin/bash

export CIBW_BEFORE_BUILD_LINUX="yum install -y mesa-libGLU-devel"
cibuildwheel --platform linux  --output-dir dist
