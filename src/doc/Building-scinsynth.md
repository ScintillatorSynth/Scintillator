Building scinsynth From Source {#BuildingScinsynth}
==============================

Putting some basic notes about building stuff here, will organize into a more
formal guide later.

```
sudo apt-get install libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev gperf python3-distutils libavcodec-dev
libavformat-dev libswscale-dev libavutil-dev libwayland-dev
```

Also the Vulkan SDK, instructions at https://vulkan.lunarg.com/doc/view/1.1.114.0/linux/getting_started_ubuntu.html

From the Vulkan ppa you will want:

```
sudo apt-get install shaderc spirv-headers spirv-tools glslang-dev vulkan-headers libvulkan-dev vulkan-validationlayers xvfb
```

If you want to run the integration tests you'll need to build SwiftShader and will also need to install some additional
dependencies:

```
sudo apt-get install python3-yaml imagemagick
```

## Python 3

Recent builds of some of the vulkan-related dependencies (effcee at least) are using a cmake command to locate python
that assumes that the in-path python is Python 3. Ubuntu for the [forseeable future](https://wiki.ubuntu.com/Python)
will keep the ```/usr/bin/python``` symbolic link pointing to Python 2, with ```/usr/bin/python3``` pointing to an
updated version of Python 3. There is an updated cmake command to find Python 3 specifically that is aware of this
convention, but requirres a newer version of CMake than the dependences are ready to require. So the cmake configuration
step for Scintillator will fail unless Python 3 is in-path as ```python```. There are a few different ways to do this,
in varying levels of invasiveness, suggested on StackOverflow as well as elsewhere.

The way we do it on travis-ci right now is to supply -DPYTHON_EXECUTABLE=`which python3` to cmake as an argument.


MacOS
-----

First install the necessary tools to build. These are in addition, generally, to the build pre-requisites for
SuperCollider. if a computer is set up to build SuperCollider that is a good starting place to build Scintillator.
You will need to install these additional dependencies:

```
brew install ninja doxygen automake git lame libass libtool shtool texi2html wget nasm
```

## ffmpeg-dev

This is a large download and the build takes a bit of time to complete, so the build is separated from the main build
and for development should only need to be performed on setup.

```
cd third_party/ffmpeg-dev
mkdir build
cd build
cmake -G Xcode ..
cmake --build .
cmake --install .
```

## vulkan-dev

Same as ffmpeg-dev in both philosphy and execution. Since MacOS doesn't have native Vulkan support we build the
requisite libraries, including MoltenVK, from source and prepare them for building against the main compilation.

```
cd third_party/vulkan-dev
mkdir build
cd build
cmake -G Xcode ..
cmake --build .
cmake --install .
```


