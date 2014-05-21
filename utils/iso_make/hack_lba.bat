@Echo off
echo Hacking LBA...
move sys\binhack32.exe binhack32.exe
move sys\hack_params.txt hack_params.txt
move data\1ST_READ.BIN 1ST_READ.BIN
move data\IP.BIN IP.BIN
binhack32.exe < hack_params.txt
move 1ST_READ.BIN data\1ST_READ.BIN
move IP.BIN data\IP.BIN
move binhack32.exe sys\binhack32.exe
move hack_params.txt sys\hack_params.txt