
# img4dc - CMake Edition

This is a fork(ish) of the img4dc project which [can be found here](https://sourceforge.net/projects/img4dc/)

If you are using Windows, you probably just want the upstream version as it may have updates.

This project exists because I needed to be able to programmatically download and compile img4dc to build
a Docker image, and the build system in the upstream code wasn't very Linux friendly.

The only changes you'll find here are:

 - CMake is the build system
 - Some stuff may be tweaked to compile nicely on Linux
 - This readme exists :)


