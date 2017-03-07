#!/bin/bash

g++ -std=c++14 -fPIC -shared -o playback_preload.so playback_preload.cpp -ldl
