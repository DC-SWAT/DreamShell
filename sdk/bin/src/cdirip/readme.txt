
CDIrip 0.6.3 by DeXT/Lawrence Williams

http://cdirip.cjb.net


Disclaimer
----------

Please note that this software is a generic-purpose tool, which must not be
used to duplicate Copyrigth-protected content. The user is only allowed to do
a legitimate use of it. I (the author) do not endorse any illegal use of this
software. This is provided for free, and comes without any kind of warranty.
The author will not be responsible for the consequences that may derive from
the proper or improper use of this software. Use it at your own risk.

As of 0.6.3, I am maintaining this program on SourceForge under the GPL, along with Debian packages.

Purpose
-------

This is a small proggie capable of extracting all the tracks contained in a
CDI (DiscJuggler) image. Both 2.0 and 3.0 CDI image versions are supported.
This will allow users which cannot use this software (either by software
incompatibilities or having an unsupported OS such as Linux or Mac OS) to burn
the contents of such images, which have a propietary format not supported by
any other recording software.

So this program will allow you to record the contents of a DiscJuggler image
with another CD-recording application. Any program that understands standard
WAV audio files and ISO or BIN images will do the job, such as CDR-Win,
cdrecord, WinOnCD or Fireburner.


Usage
-----

You can choose two ways of using this software. The simplest one is simply
doing a double clic over program's icon, the other is using command-line from
a MS-DOS window. Althought best results are still obtained from command-line
usage, this is not needed by the vast majority of users.

If you choose to use command-line, then you'll have to open a MS-DOS window,
go where cdirip is located, then type a proper command line. Basic syntax is:

cdirip image.cdi [dest_path] [-options]

Where "image.cdi" is the image you want to extract (it can also be preceeded
by an optional path). There is support for long filenames inside so you can
use any dumb name for your image files (you'll have to enclose it between
quotes if there are spaces in path/file name. You also don't need to write
".cdi" extension since CDIrip will do it for you.

The second (optional) parameter is the destination path where to save the
tracks being extracted. If you do not enter any, CDIrip will let you to
choose it, or extract to current directory, depending on first parameter.

There is a set of options which alters program behaviour. You have to type
them just after image filename (i.e.: cdirip image.cdi -option1 -option2...):

Information:

-info    only shows information about tracks inside the image (don't save)
-speed   show ripping speed info in Kb/s while extracting

Data output format:

-iso     convert all Mode2 tracks to standard ISO/2048 format (default)
-bin     force BIN format for every Mode2 or raw data track (auto for 2-ISO)
-mac     convert to "Mac" ISO/2056 format

Audio output format:

-wav     save audio tracks as WAV (default)
-raw     save audio tracks as RAW ("LSB first" format)
-swap    swap audio tracks while saving ("MSB first" format, for cdrecord)
-cda     save audio tracks as CDA ("MSB first", for cdrecord too)
-aiff    save audio tracks as AIFF ("MSB first")

Track cutting:

-cut     cuts last 2 sectors of first track only
-cutall  cuts last 2 sectors of every track (can be combined with -cut)
-full    save full Data tracks (i.e. do not cut automatically - see below)
-pregap  extract pregap area and append to the end of previous track

Presets for recording apps:

-cdrecord    WAV/ISO format, cut all tracks
-winoncd     RAW/ISO format, cut all tracks (RAW format is LSB first)
-fireburner  WAV/BIN format, cut all tracks

As an example, if you want to save info about an image into a text file you
can write in the command line:

cdirip image.cdi -info >info.txt

If you don't write CDI image filename CDIrip will ask you for one. From 0.5
you can now type options without being preceeded by an image filename (for
example: "cdirip -cutall", which will ask you for the source image file name
and then will extract it with the "-cutall" option active. There is a new BAT
file wich have the following options active by default:

cdiripcut.bat = cdirip -cutall -iso

You can now simply double clic the BAT file if you are planning to use a
different software that CDR-Win for burning. For CDR-Win you can keep using
cdirip.exe with no options.

CDIrip will ask for a destination path where to store tracks being extracted,
but only if you did not enter any source image filename (i.e. simply by
double-clicking it). Otherwise it will extract it to the current directory. You
can avoid this by entering an optional destination path just after source
image filename. Examples:

cdirip image.cdi d:\

cdirip image.cdi d:\ -cutall


How it works
------------

Once executed, the program will analyze the image and extract all the tracks
found inside it. All audio tracks will be saved as "TAudioXX.wav", where XX
is the absolute track number. Data tracks are saved as "TDataXX.iso". For
CD-XA Multisession discs (with 2 data tracks), all data tracks will be saved
in BIN/2336 or 2352 format (same as source format), with name "TDataXX.bin".

There is an option, "-iso", which forces ISO/2048 conversion to all data
tracks, just like CDIrip 0.3 did. This is only needed if you plan to use
cdrecord or WinOnCD for burning CD-XA Multisession (2 data tracks) discs, just
because these software don't have support for BIN format but ISO instead.
Please note that in this case cuesheets will not be created for ISO tracks
because of the lack of support for Mode2/2048 tracks in cuesheets.

While ripping, the program will show some data about the tracks being saved.
That is: track number, track type (Audio, Mode1 or Mode2), length (in sectors)
and the physical LBA where this track starts in the CD. The most important one
is the LBA of the last data track, just because in a Multisession CD, the
second session MUST start at the same exact number as shown by CDIrip for it
to work. So I'd recommend you write down that number for later use.

The program will also generate a cuesheet file named "TDisc.cue" with all
tracks found in the first session, along with proper PREGAP entries. Normally
only tracks from the first session will be included in the cuesheet, since
CDR-Win doesn't support cuesheets for second session. So data tracks in 2nd
and following sessions will be converted to ISO/2048 format and saved alone,
without entry in the cuesheet. These must be manually recorded with CDR-Win
in a second step specifying "MODE2" format (please read below for more info).

There is a special case with CD-XA Multisession discs (i.e. 2 data tracks),
where two cuesheets will be generated, one for each session, and data tracks
will be saved by default in BIN format instead of ISO. This is intended for
use with Fireburner since CDR-Win doesn't seem to support this kind of CD.
If you want to use a different software for recording these images such as
cdrecord or WinOnCD, simply add "-iso" option while extracting (see below).

From 0.6 upwards there is the new "-pregap" option. When you specify it,
CDIrip won't discard pregap areas (pauses between tracks, which can contain
real music data) and will append them to the end of the previous track.
PREGAP entries in cuesheets will be removed, too. This way you can burn these
tracks with no pauses between them. Note that if you don't use the cuesheet,
the resulting CD may be wrong! (i.e. if you add "-pregap" option, you MUST
ensure that your recording software won't add pauses between tracks).
Currently cdrecord doesn't support burning such CDs (it always adds pauses
between tracks). The 2-second pause before 1st track is still required, as
stated by CD-ROM standard.

Note also that on some images with negative (!) track sizes, this option is
mandatory so CDIrip can properly manage these. In this case, CDIrip will
notify you about this, if needed.


Examples
--------

cdirip image.cdi               --> simplest way (or just double clic)

cdirip image.cdi -info         --> show info only (sessions, tracks, LBA etc)

cdirip image.cdi -cut          --> cut FIRST track only

cdirip image.cdi -cutall       --> cut ALL tracks (for cdrecord, WinOnCD...)

cdirip image.cdi -full         --> do not cut data tracks when saving

cdirip image.cdi -iso          --> save all data tracks as ISO

cdirip image.cdi -bin          --> save all data tracks as BIN + 2 cuesheets

cdirip image.cdi -raw          --> save audio tracks as RAW (LSB first)

cdirip image.cdi -raw -swap    --> save audio tracks as RAW (MSB first)

cdirip image.cdi -cda          --> save audio tracks as CDA (MSB first)

cdirip image.cdi -cut -cutall  --> cut 4 sect from first track then 2 to rest

cdirip image.cdi -cutall -full --> cut audio tracks only (trick ;)

cdirip image.cdi -pregap       --> extract pregaps between tracks, too


Recommended options
-------------------

CDIrip is mainly designed to be used with CDR-Win software for the recording
process. However, you can also use a different software, althought you'll need
to use some options. Below are recommended options for each recording
software:

CDR-Win:    none
cdrecord:   -cutall -iso         (or cdiripcut.bat)
WinOnCD:    -cutall -iso -raw
Fireburner: -cutall -bin

As a bonus, a small set of new options have been added to automatically
choose the best suited options for these burning apps. These new options are:

-cdrecord     equals to -cutall -iso
-winoncd      equals to -cutall -iso -raw
-fireburner   equals to -cutall -bin

Different options could be needed for other software, being the more probably
"-cutall -iso" (or cdiripcut.bat), i.e. the same as cdrecord. This set will
produce ISO and WAV images (both cutted by 2 sectors), ready for most
recording software.

RAW audio tracks are recommended for WinOnCD due to its method of adding
tracks to custom project. Default format for these is  known as "Intel" or
"LSB first". This can be changed with "-swap" option.

Swapped RAW audio tracks (i.e. "Motorola" or "MSB first" format) is only
needed by cdrecord, in case you have problems with WAV audio tracks. In this
case you have to add following options to command-line: "-raw -swap"

If you experience problems with audio tracks being burned as "static noise",
this seems due to a bug in some burners' firmware or software drivers. This
can be fixed with the use of "-swap" option alone, i.e. without "-raw" option.
WAV files produced this way won't sound right when played from computer but
will be well recorded by these burners.


Behavior
--------

* Single Data or Audio CDs

It will save all data or audio tracks found along with proper cuesheet. This
can be directly loaded with CDR-Win.

 TDisc.cue (TData01.iso)

  or 

 TDisc.cue (TAudio01.wav, TAudio02.wav...)


* Mixed-mode CDs

Same as above, it will save first data track in Mode1/2048 format then audio
tracks, all in a single cuesheet. This can also be directly loaded and
recorded with CDR-Win with no problems.

 TDisc.cue (TData01.iso, TAudio02.wav, TAudio03.wav...)


* CD-Extra

Two sessions, first one with audio track(s) then second one with a data track
(usually in Mode2/2336 format). A cuesheet will be generated for FIRST session
containing audio tracks only. Data track in second session will be converted
to ISO/2048 format and saved separately so it can be loaded with CDR-Win and
recorded in second step, specifying MODE2 as track type (please read below).

 Tdisc.cue (TAudio01.wav, TAudio02.wav...)

  and

 TData03.iso (alone)


* CD-XA Multisession CDs ("2-ISO")

Two data tracks, both in Mode2 (CD-XA) format. This is new behaviour since 0.4
version so read carefully:

If this kind of CD is detected, CDIrip's normal behaviour will change and now
it will save all data tracks in BIN format. First data track will also be
cutted automatically to avoid the "2 sectors issue" stated above. Finally TWO
cuesheets will be generated, one for each session. This is intended to be
recorded with Fireburner since CDR-Win doesn't seem to support these kind of
CDs.

 TDisc.cue (TData01.bin)  (cutted) (can also contain audio tracks)

  and

 TDisc2.cue (TData02.bin)

You can avoid this behaviour by specifying "-iso" option, then it will act
just like CDIrip 0.3, saving all data tracks as ISO format and without
cuesheets. This is needed if you want to use cdrecord or WinOnCD for burning.
In this case the resulting files would be:

 TData01.iso (cutted)

  and

 TData02.iso


Summary table
-------------

Here you have a summary table with output format for every type of image:

* cdirip (no options)

                 Data     Audio    1st Cuesheet    2nd Cuesheet
  Single
  Session        ISO       WAV         yes             -

  CD-Extra       ISO       WAV         yes             no

  MS (2-ISO)     BIN        -          yes             yes


* cdirip -iso

                 Data     Audio    1st Cuesheet    2nd Cuesheet
  Single
  Session        ISO       WAV         yes             -

  CD-Extra       ISO       WAV         yes             no

  MS (2-ISO)     ISO        -          no              no


* cdirip -bin
                 Data     Audio    1st Cuesheet    2nd Cuesheet
  Single
  Session        BIN       WAV         yes              -

  CD-Extra       BIN       WAV         yes             yes

  MS (2-ISO)     BIN        -          yes             yes


Recording
---------

* Single Audio, single Data and Mixed-mode CDs

Just load CDR-Win and then load the cuesheet generted by CDIrip. Now press
"Start Recording". Easy, isn't it? :)

Please note that Mixed-mode CDs (which have a data track and then some audio
tracks), the "Open New Session" checkbox shouldn't be checked since this is a
single session disc.


* CD-Extra

This kind of image has one or more audio tracks in the first session then a
data track on the second. Below are instructions for use with CDR-Win:

Step 1: burning first session

Load CDR-Win, push the "Record Disk" button, and then "Load Cuesheet". Load
the "TDisc.cue" file generated by CDIrip. Check the "Open New Session"
checkbox (leave the rest as default) and then press "Start Recording". This
will record the first session.

NOTE: If you cannot check the "Open New Session" checkbox at first step, your
burner is not properly supported by CDR-Win. Then you'll have to use a
different recording software (at least for the first session only). Users have
reported that Fireburner, cdrecord and WinOnCD works (and probably NTI
CD-Maker and Ahead Nero, too). But please note that you'll have to use "-cut"
or "-cutall" options with any of these software, because of the "2 sectors
issue" common to all these software.

Step 2: checking LBA (optional)

Once recorded, you can check that the starting LBA for the second session is
the right one, if you want to be sure. This is not neccesary althought
recommended. For that you'll need "cdrecord" package. Just type in the
command line:

cdrecord dev=0,1,0 -msinfo

Note that you could need to change device settings (0,1,0) according with your
recorder setup. See CDR-Win's "Devices and Settings" window for that info.

This command will show you 2 numbers (x,y). The second one MUST be the same
LBA value as shown by CDIrip in the last data track while ripping. If it is
not, it won't work (see "2 sectors issue" above).

You can also use ISObuster to test LBA of last data track, but this can be
done only after recording entire disc. Just load the disc into ISObuster,
click on the second session and look at the start sector value below.

Step 3: burning second session

If everything goes right, now you have to add the second session. Press the
"File Backup and Tools" button in main CDR-Win window. In Function choose
"Record an ISO9660 Image File", then add the ISO image. In Recording Options
select "MODE2" as Track Mode. You may want to deselect "Write Postgap" since
it writes additional 150 sectors after the data track, which are not needed.
All the remaining options should be leaved as default values (Finalize Session
must be checked). Finally, press "START" button.

That's it!

If CDR-Win didn't work for you, you can still use other software such as
Fireburner, cdrecord or WinOnCD, but you must extract all tracks with
"-cutall" option since these software needs it.

Fireburner: extract with "-cutall" option, then load cuesheet and burn as
described below for CD-XA Multisession discs, but only for first session
(for second one you still will have to use CDR-Win or cdrecord). You can use
Fireburner for both sessions by specifying "-cutall -bin" options.

cdrecord: extract with "-cutall" option, then use following command lines
(stuff between parentheses is for multiple audio tracks only):

cdrecord dev=0,1,0 speed=4 -multi -audio TAudio01.wav (-audio TAudio02.wav...)

cdrecord dev=0,1,0 speed=4 -multi -xa1 TData01.iso

Note: in cdrecord, if you choose RAW format for audio tracks i'd recommend you
to add also "-swap" option so they are saved in "Motorola" (MSB first) format,
which is best suited for cdrecord. For example: cdirip -cutall -raw -swap


* CD-XA Multisession ("2-ISO" images)

These kind of CDs have two data tracks, each one in a different session. This
can only be recorded with Fireburner, cdrecord and WinOnCD, AFAIK.

Fireburner: click on "Visual CUE burner / Binchunker", press right mouse
button and select "Load tracks from *.CUE". Load first cuesheet, then press
right mouse button again and select "Burn / Test Burn". In burning window
uncheck "Test Burn" and leave checked "Eject CD", "Multi-session" and "Close
Session". On Recording Method select "Track at Once (TAO)" and click OK. This
will record first session.

Once recorded go to Visual CUE window again, right-click and select "Clear CD
Layout". Then repeat the same method for second cuesheet.

cdrecord: extract tracks with "-iso" option, then record them with following
command lines:

cdrecord dev=0,1,0 speed=4 -multi -xa1 TData01.iso

cdrecord dev=0,1,0 speed=4 -multi -xa1 TData02.iso

That's it!


Troubles
--------

If you are having troubles while writting these images back to CD that's 99%
due to a common issue related with track sizes, where either the recording
software or the burner itself is adding 2 extra sectors to every track, thus
ruining multisession CDs such as CD-Extra ones. Here you have a brief
explanation of this issue and how to solve it:

*** short tracks issue ***

It seems that most burners out there aren't capable of burning tracks below
302 sectors in size when in Track-at-Once mode (common in Multisession discs).
If you try to burn such a track, the burner will add padding data up to the
minimum size of 302. The only workaround for this is either finding a burner
not affected by this problem, or trying to burn in RAW/DAO mode which has to
be supported by both burner and recording software. Another workaround would
be converting a CD-Extra type CD to XA-multisession one with 2 data tracks. In
that case, ISOfix can be useful.

*** 2 sectors issue ***

Most recording apps such as cdrecord, Fireburner or WinOnCD (but not CDR-Win)
tends to add 2 extra sectors to the end of EVERY audio tracks when burning in
TAO mode. So if you are going to use any of these you should add the "-cutall"
option to the command line so every track will be cutted by 2 sectors when
extracted. Note that this won't fix the "short track issue" stated above,
thought.

In that case, for multiple audio track images you can use both "-cut" and
"-cutall" options together, so forst track will be cutted by 4 sectors then 2
from every other track. CDIfix does a similar job but will support any number
of short tracks issue.

*** data tracks issue ***

I've found out that EVERY burning software always adds 2 blank sectors at the
end of every data track. Multisession CDs with a data track in the first
session have to be cutted for it to work correctly. CDIrip detects these cases
and does this automatically (althought can be avoided with "-full" option, if
needed, but NOT recommended).


History
-------

CDIrip is a Win32 console application coded in straight C and compiled with
BloodShed Dev-C++ v4.0, an Open Source Win32 compiler based on GNU C compiler,
which is freely available at: http://www.bloodshed.net

 0.6.2 (2002/06/08)

  - Support for DiscJuggler 4.0 CDI images
  - CDIrip now opens CDR-WIN 5.x (*.cdr) images, too
  - CDIrip sources are now much more portable (removed endian-dependant stuff)
  - Added "Mac" ISO output support (new option -mac)
  - Added CDA/AIFF audio output support (options -cda, -aiff)

 0.6c (2001/12/27)

  - Added support for CDI 3.5.826+ images

 0.6b (2001/12/07)
 
  - Fixed bug in 0.6a causing some old images (3.0, 2.0) not being recognized

 0.6a (2001/11/29)
 
  - Fixed bug with some DiscJuggler 3.5 images (extracted 1st session only)

 0.6 (2001/09/07)

  - Support for new DiscJuggler 3.00.780+ CDI images
  - Destination path dialog (when double-clicked)
  - Destination path can now be entered in command-line
  - MAJOR speedup (now uses two 1024 Kb buffers for read and write)
  - New "/pregap" option to extract pregaps, instead of discarding them
  - New "/speed" option to show ripping speed in Kb/s
  - All options can now start with "-" character (un*x-like)
  - Fixed 0-length pregap handling in cuesheet
  - Fixed negative track size handling (must add /pregap option)
  - Fixed default track cutting misbehaviour (cutted last track by default)
  - Fixed Mode1/2352 (raw) to 2048 (ISO) conversion bug (sorry :)
  - Switched to Bloodshed Dev-C++ free compiler (tiny executable size!)

 0.5a (2001/02/13)
 
  - Fixed stupid bug in audio swap routine

 0.5 (2001/02/05)

  - Support for CDI v3.0 images
  - Support for raw data tracks (2352 bytes/sector) either Mode1 and Mode2
  - Audio tracks can now be byte-swapped with "/swap" option (for cdrecord)
  - Tracks in second cuesheet now starts at no.1 (for multisession CDs)
  - Truncated tracks are now detected and skipped
  - Better error handling
  - Windows Open File dialog and error Message Boxes for easier usage
  - Options can now be written without an image filename (will ask for it)
  - New preset options for burning apps: /cdrecord, /winoncd, /fireburner
  - New progress counter (faster, bigger and better ;)
  - Big internal cleanup
  - Small bugfixes

 0.4 (2000/10/12)

  - No need to use DOS-prompt for basic usage: just do double click to it
  - Support for Mixed-mode (Mode1/2048) and CD-XA Multisession (Mode2/2336)
    data tracks in cuesheet
  - Data tracks in first session are now automatically cutted by 2 sectors
  - Data tracks in CD-XA Multisession discs are now saved in BIN/2336 format
  - Audio tracks are now saved in WAV format
  - Added "/raw" option to force RAW format to audio tracks
  - Added "/iso" option to force ISO/2048 conversion to all data tracks
  - Added "/bin" option to force BIN/2336 format + 2 cuesheets
  - Added "/full" option to avoid cutting data tracks
  - Added "/cut" and "/cutall" options to cut tracks
  - Added "/info" option
  - Added progress counter (wow!)
  - Small bugfixes

 0.3 (2000/09/06)

  - Automatic cuesheet generation

 0.2 (2000/09/04)

  - Support added for Mode1/2048 tracks

 0.1 (2000/09/03)

  - First, internal release


(C) 2001 DeXT
