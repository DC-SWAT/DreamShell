@Echo off
SET PATH=sys
For %%1 in (*.raw) do (
	echo Adding WAV header to %%1
	raw2wav.exe "%%1" "%%1".tmp 2 44100 16
	echo "Converting PCM to Yamaha ADPCM..."
	ffmpeg -i "%%1".tmp -map_metadata -1 -c adpcm_yamaha "%%1".wav
	del "%%1".tmp
	del "%%1"
)
echo Complete!
