@Echo off
SET PATH=sys
For %%1 in (*.raw) do (
	echo Adding WAV header to %%1
	raw2wav.exe "%%1" "%%1.wav"
	echo Converting PCM to ADPCM ITU G.723 (Yamaha)...
	wav2adpcm.exe "%%1.wav" "%%1:~0,-4%"
	del "%%1.wav"
	del "%%1"
	goto :break
)
:break
echo Complete!
