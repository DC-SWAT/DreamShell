@Echo off
SET PATH=sys
echo Convertig NGR to ISO...
For %%1 in (*.nrg) do nerorip "%%1" ./ && del -f taudio01.wav "%%1.iso" && rename tdata02.iso "%%1.iso"