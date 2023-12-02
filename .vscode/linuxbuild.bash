#!/bin/bash
if [[ ! -a built ]]; then mkdir built; fi
cd built
g++ -std=c++17 -fno-rtti -O3 -D NDEBUG -Wall -Wextra -Wpedantic -o ./base85 $(realpath ..)/*.cpp
