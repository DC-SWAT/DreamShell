@Echo off
echo RePacking GDI image...
call Gdi2Data.bat
call Hack_LBA.bat
call Create_ISO.bat