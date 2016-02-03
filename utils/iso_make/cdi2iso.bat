@Echo off
SET PATH=sys
echo Convertig CDI to ISO...
For %%1 in (*.cdi) do cdirip "%%1" && del -f s01t01.wav tdisc.cue "%%1.iso" && rename s02t02.iso "%%1.iso"