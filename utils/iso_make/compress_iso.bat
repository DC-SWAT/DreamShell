@Echo off
SET PATH=sys
echo Compressing ISO to CSO with LZO...
For %%1 in (*.iso) do ciso lzo 9 "%%1" "%%1".cso
echo Complete!