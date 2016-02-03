@Echo off
SET PATH=sys
For %%1 in (*.gdi) do (
	echo Optimizing GDI image...
	gdiopt.exe "%%1" disc_optimized.gdi
	goto :break
)
:break
echo Complete!
