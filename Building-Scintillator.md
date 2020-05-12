```sh
sudo apt-get install libxrandr libxinerama-dev libxcursor-dev libxi-dev gperf

# (if you wanna build docs)
sudo apt-get install doxygen graphviz

git submodule update --init --recursive

# install python3 if needed

mkdir build
cd build
cmake .. # will download and unpack binary dependencies
make
```
