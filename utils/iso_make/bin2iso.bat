@Echo off
SET PATH=sys
echo Convertig bin to iso...
rem For %%1 in (*.bin) do iat --iso -i "%%1" -o "%%1".iso
For %%1 in (*.bin) do bin2iso.exe "%%1" "%%1".iso
echo Complete!