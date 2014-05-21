@Echo off
echo Descrambling...
SET PATH=sys
scramble -d data/1ST_READ.BIN 1ST_READ.BIN
del data/1ST_READ.BIN
move 1ST_READ.BIN data/1ST_READ.BIN