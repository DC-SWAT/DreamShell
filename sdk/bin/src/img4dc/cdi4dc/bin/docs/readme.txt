   [big_fury]
  888888888    8888  888888888888  8888     888888888     8888      8888   888888888
8888888888888  8888  888888888888  8888   8888888888888   8888      8888 8888888888888
8888     8888              88888         8888       8888  8888      8888 8888     8888
888888888      8888      888888    8888 8888         8888 8888      8888 888888888
  8888888888   8888     88888      8888 8888         8888 8888      8888   88888888888
       888888  8888   88888        8888 8888         8888 8888      8888         888888
8888      8888 8888  88888         8888  8888       8888  8888      8888 8888      8888
8888888888888  8888 88888888888888 8888   8888888888888    888888888888  8888888888888
  8888888888   8888 88888888888888 8888     888888888       8888888888     8888888888

CDI4DC
======

Version : v0.3b (13 april 2007)
(C)reated by [big_fury]SiZiOUS
http://sbibuilder.shorturl.com/

I) What's that
==============
This proggy was written in order to replace the old (but good) Xeal's bin2boot.

It generates a *REAL* valid CDI selfboot file (e.g. a mountable CDI into virtual drives) from an ISO.

You can generate *BOTH* Audio/Data and Data/Data images.

The resulting CDI file is ready to burn, similar if you have used the Marcus method.

II) How to use it
=================

You need mkisofs from cdrtools.

II.1. Audio/Data images
-----------------------

Just generate your iso with the same command as the Marcus tutorial (please note the -G switch to insert
the IP.BIN in the iso).

mkisofs -C 0,11702 -V YOUR_VOLUME_NAME -G IP.BIN -joliet -rock -l -o YOUR_ISO.ISO YOUR_SOURCE_DIRECTORY

Then, you can use cdi4dc. Type this command :

cdi4dc YOUR_ISO.ISO YOUR_CDI.CDI

The result will be a valid CDI file.

II.2. Data/Data images
----------------------

mkisofs -V YOUR_VOLUME_NAME -G IP.BIN -joliet -rock -l -o YOUR_ISO.ISO YOUR_SOURCE_DIRECTORY

cdi4dc YOUR_ISO.ISO YOUR_CDI.CDI -d

III) Credits
============

This proggy's still dedicated to Ron even if today it isn't the 6 july !
Greetings goes to BlackAura, Fackue, Xeal, DeXT, Heiko Eissfeldt and Joerg Schilling (for libedc).

Bugs report : Fackue

SiZ! for Dreamcast-Scene 2007... The legend will never die...