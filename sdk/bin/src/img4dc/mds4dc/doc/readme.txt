
                    * * * [b i g _ f u r y ] S i Z i O U S * * *
                           http://sbibuilder.shorturl.com/
        _____________    _____________    _____________________________________
       /            /   /            /   /           /    /      /            /
      /     _______/___/_______     /___/           /    /      /     _______/
     /     /      /   /            /   /     /     /    /      /     /      /
    /            /   /            /   /     /     /    /      /            /
   /________/   /   /     _______/   /     /     /    /__    /_________/  /
  /            /   /            /   /           /           /            /
 /____________/___/____________/___/___________/___________/____________/

+-------------------------------------------------------------+
| MDS4DC - Dreamcast Selfboot Media Descriptor Disc Generator |
+-------------------------------------------------------------+

Version...: v0.1b
Date......: 06 june 2007


I) What's that
==============

Hey hey hey all Dreamcast fans ! Heres come another cool proggy for your 
favorite console !

MDS4DC is a tool written in order to generate a *REAL* valid Alcohol 120% 
"Media Descriptor" image (MDF/MDS) from an ISO with the possibility to add 
custom RAW audio tracks.

Features :
- 100% identical images (based on Alcohol 120% 1.9.6 Build 4719).
- Supports Audio/Data and Data/Data image format
- Support CD-DA

With this, creating Dreamcast CD-R's really easy, you have no excuses now !


II) How to use it
=================

You only need mkisofs from cdrtools.

II.1. Audio/Data images (no custom CD-DA)
-----------------------------------------

Just generate your iso with the same command as the Marcus tutorial 
(please note the -G switch to insert the IP.BIN in the iso).

mkisofs -C 0,11702 -V YOUR_VOLUME_NAME -G IP.BIN -joliet -rock -l -o YOUR_ISO.ISO YOUR_SOURCE_DIRECTORY

Then, you can use mds4dc. Type this command :

mds4dc -a OUTPUT.MDS YOUR_ISO.iso

Done !

II.2. Audio/Data images (with custom CD-DA)
-------------------------------------------

Please note that Audio tracks are in RAW format, *NOT* WAV !

First, you must calculate the MSINFO (LBA) value from your RAW audio 
tracks for your data track. For that, you can use the lbacalc proggy 
inside this package.

Example :

lbacalc track1.raw track2.raw track3.raw

The proggy shows you a value (for example) : 12696

This's your data track MSINFO.

Now, generate your iso with mkisofs :

mkisofs -C 0,12696 -V YOUR_VOLUME_NAME -G IP.BIN -joliet -rock -l -o YOUR_ISO.ISO YOUR_SOURCE_DIRECTORY

Then use mds4dc :

mds4dc -c OUTPUT.MDS YOUR_ISO.iso track1.raw track2.raw track3.raw

Done !

II.3. Data/Data images
----------------------

mkisofs -V YOUR_VOLUME_NAME -G IP.BIN -joliet -rock -l -o YOUR_ISO.ISO YOUR_SOURCE_DIRECTORY

mds4dc -d OUTPUT.MDS YOUR_ISO.ISO


III) Credits
============

Greetings goes to :

BlackAura, Fackue, Henrik Stokseth, Heiko Eissfeldt, Joerg Schilling, 
Marcus Comstedt, Ron, JMD, speud and all the rest i forgot !

Special smack à ma ptite bestiole, hein Mily ? ;)


SiZ!^DCS
"Can you feel the power of the DCS ?"

