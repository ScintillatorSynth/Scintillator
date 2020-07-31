sudo apt-get install libxrandr libxinerama-dev libxcursor-dev libxi-dev gperf

(if you wanna build docs)
sudo apt-get install doxygen graphviz

git submodule update --init --recursive

install python3 if needed

fetch binary deps:

```
python3 tools/fetch-binary-deps.py
```

cd build
cmake ..
make

If on Xenial Linux you're going to need to build your own SSL from sources.
