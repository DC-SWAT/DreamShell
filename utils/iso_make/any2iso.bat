@Echo off
SET PATH=sys
echo Convertig any images to ISO
set /P img=" Enter cd image filename: "
iat --iso -i %img% -o %img%.iso
rem For %%1 in (*.cdi) do iat --iso -i "%%1" "%%1".iso
rem For %%1 in (*.nrg) do iat --iso -i "%%1" "%%1".iso