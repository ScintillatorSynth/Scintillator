# Scintillator

Scintillator is a visual synth designed to be controlled via [OSC](http://opensoundcontrol.org/introduction-osc) from
the music programming language [SuperCollider](https://supercollider.github.io/).

This software is distributed in two pieces but both are maintained in this repository. The first piece is the visual
synth itself, called ```scinsynth```, which is C++ program that uses the [Vulkan](https://www.khronos.org/vulkan/) API
to generate video imagery in real time. The source code for scinsynth can be found in [src/](src/), and some related
documentation, including build instructions, are in [src/doc/](src/doc/). The synth relies on a bunch of open source
libraries to help with various functions, these are mostly included as git submodules, and all third-party code lives
in the [third_party](third_party/) directory.

The second half of Scintillator is a collection of SuperCollider sclang class files. These are distributed normally as a
Quark, and indeed this repository is the Scintillator Quark as available from the in-app Quarks system. Documentation
for these classes is in the SuperCollider help format, but can be found in the [HelpSource](HelpSource/) directory.

