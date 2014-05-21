@ECHO OFF
: Automatic GDI-based extraction ...
title=FamilyGuy's SelfBootDATA Pack! - Gdi2Data v2.1
ECHO.
ECHO     This batch file will extract the data
ECHO     from a multiple-tracks Dreamcast game
ECHO     using its .GDI file to get the needed
ECHO     informations. If the GDI file can not
ECHO     be found, you'll have to specify some
ECHO     informations manually!
ECHO.
ECHO                FamilyGuy 2009
ECHO.
pausetitle
set DO=0
set ?=0
for %%1 in (*.gdi) do set /A ?=?+1
if %?%==0 goto MANUAL
If not %?%==1 goto ERROR1
For %%1 in (*.gdi) do set GDIN=%%1
For %%1 in (*.gdi) do copy "%%1" disc.gdi >nul
ECHO.
title=FamilyGuy's SelfBootDATA Pack! - Gdi2Data v2.1 - Processing %GDIN% ...
ECHO     Processing %GDIN% ...
ECHO.
for /F "tokens=1" %%1 in (disc.gdi) do set NUM=%%1
if %NUM%==3 set DO=1
ECHO     Number of tracks: %NUM%
for /F "tokens=5 skip=%NUM%" %%1 in (disc.gdi) do set T=%%1
ECHO     Last data track : %T%
for /F "tokens=2 skip=%NUM%" %%1 in (disc.gdi) do set LBA1=%%1
SET /A LBA=LBA1+150
ECHO     %T% LBA : %LBA1%
ECHO     Extraction LBA  : %LBA% (%LBA1%+150)
For %%1 in (%T%) do set TN=%%~n1
for /F "tokens=4 skip=%NUM%" %%1 in (disc.gdi) do set TYPE=%%1
del disc.gdi >nul
if %TYPE%==2352 ECHO     Track type      : BIN (2352 bytes/sector) ...
if %TYPE%==2048 ECHO     Track type      : ISO (2048 bytes/sector) ...
if %TYPE%==2352 If not exist track03.bin goto Error2
if %TYPE%==2048 If not exist track03.iso goto Error2
if not exist %T% goto Error3
if %TYPE%==2352 goto BIN
if %TYPE%==2048 goto EXT

:BIN
ECHO.
ECHO     Converting track03.bin into track03.iso ...
ECHO.
copy sys\bin2iso.exe bin2iso.exe >nul
bin2iso track03.bin track03.iso
if %DO%==1 GOTO EXT
ECHO.
ECHO     Converting %T% into %TN%.iso ...
ECHO.
bin2iso %T% %TN%.iso

:EXT
IF %TYPE%==2352 del bin2iso.exe >nul
ECHO.
ECHO     Extracting %TN%.iso to the data folder with the TOC from track03.iso ...
ECHO.
copy sys\extract.exe data\extract.exe >nul
move TRACK03.iso data/track03.iso >nul
IF NOT %DO%==1 move %TN%.iso data/%TN%.iso >nul
cd data
extract track03.iso %TN%.iso %LBA%
del extract.exe >nul
move TRACK03.iso ..\track03.iso >nul
IF NOT %DO%==1 move %TN%.iso ..\%TN%.iso >nul
move ip.bin ..\ip.bin >nul
cd..
IF %TYPE%==2352 del track03.iso >nul
IF NOT %DO%==1 if %TYPE%==2352 del %TN%.iso >nul
GOTO END

:ERROR1
ECHO.
ECHO        There is %?% gdi files in the
ECHO          root folder of this pack.
ECHO. 
ECHO        Please use only ONE GDI file!
ECHO.
ECHO                FamilyGuy 2009
ECHO.
PAUSE >nul
EXIT

:Error2
ECHO.
ECHO     ERROR !!
IF %TYPE%==2352 ECHO     track03.bin doesn't exist!
IF %TYPE%==2048 ECho     track03.iso doesn't exist!
ECHO     Make sure it is in the the root folder of the pack!
ECHO.
if not %DO%==1 If not exist %T% goto Error4
ECHO     FamilyGuy 2009
pause >nul
exit

:Error3
ECHO.
ECHO     ERROR !!
:Error4
ECHO     %T% doesn't exist!
ECHO     Make sure it is in the the root folder of the pack!
ECHO.
ECHO     FamilyGuy 2009
pause >nul
exit

: Manual user-input extraction ...
:MANUAL
CLS
ECHO.
ECHO     No GDI file found! You'll have to 
ECHO     specify certain things manually!!
ECHO.
ECHO     Launching manual mode ...
ECHO.
ECHO     Semi-Manual extraction of the data from a multi-tracks
ECHO     game to the data folder. It works for both ISO and BIN
ECHO     files! In the latter case, the tracks will be converted
ECHO     into iso before extraction.
ECHO.
ECHO     It will also place the ip.bin in the root folder of the pack.
ECHO.     
ECHO     For 3track-only games, input the name of the 3rd track both
ECHO     times it ask for a track, and put 45000 as the starting LBA.
ECHO.
ECHO.
ECHO.
set /p T3="Enter the name of the 1st track with the extension (ex: XYZ.bin):"
for %%1 in (%T3%) do (
set T3X=%%~x1
set T3N=%%~n1
)
If %T3X%==.bin Echo %T3% will be converted to iso ...
ECHO.
set /p TX="Enter the name of the last track with the extension (ex: ZYX.iso):"
for %%1 in (%TX%) do (
set TXX=%%~x1
set TXN=%%~n1
)
If %TX%==%T3% Echo Both tracks are the same!
if not %TX%==%T3% (
If %TXX%==.bin Echo %TX% will be converted to iso ...
)
ECHO.

ECHO.
set /p LBA1="Enter the Starting LBA of the last track of the GD-ROM:"
set /A LBA = "LBA1"+150
ECHO.
ECHO.

IF not exist %T3% GOTO ERR1
IF not exist %TX% GOTO ERR2

if %T3X%==.bin (
ECHO Converting %T3% into %T3N%.iso
copy "sys\bin2iso.exe" bin2iso.exe >nul
bin2iso %T3% %T3N%.iso
del bin2iso.exe >nul
)
if %TXX%==.bin (
if %TX%==%T3% goto EXTRACT
ECHO Converting %TX% into %TXN%.iso
copy "sys\bin2iso.exe" bin2iso.exe >nul
bin2iso %TX% %TXN%.iso
del bin2iso.exe >nul
)

GOTO EXTRACT

:ERR1
IF not exist %TX% GOTO ERR3

ECHO.
ECHO     ERROR !
ECHO     %T3% doesn't exist!
ECHO     Make sure %T3% is in the the root folder of the pack!
ECHO.
ECHO     FamilyGuy 2009
pause >nul
exit

:ERR2
ECHO.
ECHO     ERROR !!
ECHO     %Tx% doesn't exist!
ECHO     Make sure %Tx% is in the the root folder of the pack!
ECHO.
ECHO     FamilyGuy 2009
pause >nul
exit

:ERR3
ECHO.
ECHO     ERROR !!!
ECHO     %T3% and %TX% don't exist!
ECHO     Make sure they are in the the root folder of the pack!
ECHO.
ECHO     FamilyGuy 2009
pause >nul
exit

:EXTRACT
ECHO.
ECHO.
ECHO     Extracting %TXN%.iso to the data folder with the TOC from %T3N%.iso ...
ECHO.
move %T3N%.iso data\%T3N%.iso >nul
if not %TX%==%T3% move %TXN%.iso data\%TXN%.iso >nul
copy sys\extract.exe data\extract.exe >nul
cd data >nul
extract %T3N%.iso %TXN%.iso %LBA%
ECHO     Moving ip.bin ...
move ip.bin ..\ip.bin
ECHO     Deleting temp files ...
del extract.exe >nul
move %T3N%.iso ..\%T3N%.iso
if not %TX%==%T3% move %TXN%.iso ..\%TXN%.iso
cd .. >nul

ECHO.
ECHO     The tracks have been extracted to the data folder !
ECHO.

If %T3X%==.bin (
del %T3N%.iso >nul
goto HERE
)

:HERE
if not %T3%==%TX% (
If %TXX%==.bin
del %TXN%.iso >nul
GOTO THERE
)


:THERE
GoTO END

:END
title=FamilyGuy's SelfBootDATA Pack! - Gdi2Data v2.1
Echo.
ECHO     DONE!
ECHO.
ECHO     Familyguy 2009