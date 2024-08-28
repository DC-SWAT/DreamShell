DreamShell
==========

The Dreamshell is the operating system for the Sega Dreamcast based on the KallistiOS kernel.
It has a dynamic loadable modular system and  interface for creating applications with XML UI and both C/C++ and Lua script on.
You can see examples in ready-made applications and modules, drivers for various devices, formats and interfaces. Examples for audio and video decoding, compression, packaging, binding, network, emulation, scripts and more. From hardcore low-level assembler to high-level applications.
There are also large subproject is the ISO Loader, which contains emulation of BIOS system calls, CDDA playback and VMU, also it can hooking interrupts for various SDKs and more.


## Build

### Setup environment
##### Packages
```console
sudo apt update
sudo apt install -y gawk patch bzip2 tar make cmake pkg-config
sudo apt install -y gettext wget bison flex sed meson ninja-build
sudo apt install -y build-essential diffutils curl python3 rake
sudo apt install -y genisoimage squashfs-tools texinfo git
sudo apt install -y libgmp-dev libmpfr-dev libmpc-dev libelf-dev libisofs-dev
sudo apt install -y libpng-dev libjpeg-dev liblzo2-dev liblua5.2-dev
cd /tmp
git clone https://github.com/LuaDist/tolua.git
cd /tmp/tolua && mkdir build && cd ./build
cmake ../ && make && sudo make install
```
##### Code
```console
sudo mkdir -p /usr/local/dc/kos
sudo chown -R $(id -u):$(id -g) /usr/local/dc
cd /usr/local/dc/kos
git clone https://github.com/KallistiOS/kos-ports.git
git clone https://github.com/KallistiOS/KallistiOS.git kos
cd /usr/local/dc/kos/kos
git clone https://github.com/DC-SWAT/DreamShell.git ds
git checkout `cat ds/sdk/doc/KallistiOS.txt`
cp ds/sdk/toolchain/environ.sh environ.sh
cp ds/sdk/toolchain/patches/*.diff utils/dc-chain/patches
```
##### Toolchain
```console
sudo mkdir -p /opt/toolchains/dc
sudo chown -R $(id -u):$(id -g) /opt/toolchains/dc
cd /usr/local/dc/kos/kos/utils/dc-chain
cp Makefile.default.cfg Makefile.cfg
make
```
##### SDK
```console
cd /usr/local/dc/kos/kos
source ./environ.sh
make && cd ../kos-ports && ./utils/build-all.sh
cd ${KOS_BASE}/ds/sdk/bin/src && make && make install
cd ${KOS_BASE}/ds
ln -nsf `which tolua` sdk/bin/tolua
ln -nsf `which mkisofs` sdk/bin/mkisofs
ln -nsf `which mksquashfs` sdk/bin/mksquashfs
```

### Use environment
##### for each new terminal type:
```console
cd /usr/local/dc/kos/kos/ds && source ../environ.sh
```

### Build code
##### Full build
```console
make build
```
##### Full clean
```console
make clean-all
```
##### Make release package
```console
make release
```
##### Update code from GitHub
```console
make update
```
##### Update code from GitHub and re-build
```console
make update-build
```
##### Re-build toochain (if updated)
```console
make toolchain
```
##### Core and libraries only
```console
make
```
##### Modules, applications and commands only
```console
cd ${KOS_BASE}/ds/modules && make
cd ${KOS_BASE}/ds/commands && make
cd ${KOS_BASE}/ds/applications && make
```
##### Firmwares only
```console
cd ${KOS_BASE}/ds/firmware/bootloader && make && make release
cd ${KOS_BASE}/ds/firmware/isoldr && make && make install
```

### Running
- dc-tool-ip: `make run`
- dc-tool-serial: `make run-serial`
- lxdream emulator: `make lxdream`
- nulldc emulator: `make nulldc`
- flycast emulator: `make flycast`
- make cdi image: `make cdi`

## Links
- Website: http://www.dc-swat.ru/ 
- Forum: http://www.dc-swat.ru/forum/ 
- Donate: http://www.dc-swat.ru/page/donate/
