DreamShell
==========

The Dreamshell is the operating system for the Sega Dreamcast based on the KallistiOS kernel.
It has a dynamical loadable modular system and XML interface for creating applications in both C/C++ and Lua script.
You can see many examples already in ready-made applications and modules, unique drivers for different devices, formats and interfaces. Examples of simple decoding of audio and video, data compression, network stack, emulation, scripts and more. From hardcore low-level assembler to high-level applications.
Also here there are firmwares for SPU subsystem and BIOS system calls emulation.  
We are glad if this code will help someone and we will be glad of your support too.


## Build
- Make directories: `/opt/toolchains/dc` and `/usr/local/dc/kos`
- Checkout KallistiOS to `/usr/local/dc/kos/kos`
- Checkout KallistiOS ports to `/usr/local/dc/kos/kos-ports`
- Checkout DreamShell to `/usr/local/dc/kos/kos/ds`

### Environment
```console
sudo apt install tolua
cd /usr/local/dc/kos/kos
source ./environ.sh
cd ds/sdk/toolchain && ./donwload.sh && ./unpack.sh
make && cd ../../../
make && cd ../kos-ports && ./utils/build-all.sh
cd ./lib && rm -f libfreetype.a liboggvorbisplay.a libogg.a
cd ../../kos/ds/sdk/bin/src && make && make install && cd ../../../
```

### Core and libraries
```console
cd ./lib && make
cd ../ && make
```

### Modules, applications and commands
```console
cd ./modules && make && make install
cd ../commands && make && make install
cd ../applications && make && make install
cd ../
```

### Firmwares
```console
cd ./firmware/bootloader && make && make release
cd ../isoldr/loader && make && make install
cd ../../aica && make && make install
cd ../../
```

### Run
- dc-tool-ip: `make run`
- dc-tool-serial: `make run-serial`
- lxdream emulator: `make lxdream`
- nulldc emulator: `make nulldc`
- make cdi image: `make cdi`

## Links
- Website: http://www.dc-swat.ru/ 
- Forum: http://www.dc-swat.ru/forum/ 
- Donate: http://www.dc-swat.ru/page/donate/
