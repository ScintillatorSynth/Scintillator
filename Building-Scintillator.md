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


Xenial
------

Requires gperf 3.1, which will need to be built from sources.
Requires openssl 1.1, which will need to be built from sources, or copied from the install-ext/ssl directory.

