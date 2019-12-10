Putting some basic notes about building stuff here, will organize into a more
formal guide later.

sudo apt-get install libvulkan-dev libxinerama-dev libxcursor-dev libvulkan-dev vulkan-validationlayers libxi-dev      \
    libxrandr-dev libgl-dev gperf python3-distutils

## Python 3

Recent builds of some of the vulkan-related dependencies (effcee at least) are using a cmake command to locate python
that assumes that the in-path python is Python 3. Ubuntu for the [forseeable future](https://wiki.ubuntu.com/Python)
will keep the ```/usr/bin/python``` symbolic link pointing to Python 2, with ```/usr/bin/python3``` pointing to an
updated version of Python 3. There is an updated cmake command to find Python 3 specifically that is aware of this
convention, but requirres a newer version of CMake than the dependences are ready to require. So the cmake configuration
step for Scintillator will fail unless Python 3 is in-path as ```python```. There are a few different ways to do this,
in varying levels of invasiveness, suggested on StackOverflow as well as elsewhere.

