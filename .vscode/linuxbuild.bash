#!/bin/bash
if [[ ! -a built ]]; then mkdir built; fi
cd built
g++ -O3 -std=c++17 -fno-rtti -Wall -Wextra -Wpedantic -D NDEBUG -o ./base85 $(realpath ..)/*.cpp
