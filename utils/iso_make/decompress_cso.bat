@Echo off
SET PATH=sys
echo Decompressing CSO (LZO) to ISO...
For %%1 in (*.cso) do ciso lzo 0 "%%1" "%%1".iso
echo Complete!