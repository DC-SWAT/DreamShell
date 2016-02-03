@Echo off
SET PATH=sys
For %%1 in (*.raw) do (
	echo Adding WAV header to %%1
	raw2wav.exe "%%1" "%%1.wav"
	echo Converting PCM to ADPCM (Yamaha)...
	ffmpeg -i "%%1.wav" -map_metadata -1 -c adpcm_yamaha "%%1:~0,-4%"
	del "%%1.wav"
	del "%%1"
	goto :break
)
:break
echo Complete!
