#!/usr/bin/perl

#
# List of benches to run
#

#########################################
# Decoder benches
#########################################

  # Raw command-line args passed to 'xvid_bench 9'
  # format: bitstream_name width height checksum
  # followed, possibly, by the CPU option to use.  

@Dec_Benches = (

  "test1.m4v 640 352 0x9fa4494d -sse2"
, "test1.m4v 640 352 0x9fa4494d -mmxext"
, "test1.m4v 640 352 0x9fa4494d -mmx"
, "test1.m4v 640 352 0x76c9cde2 -c"

, "qpel.m4v 352 288 0xc07eb687 -sse2"
, "qpel.m4v 352 288 0xc07eb687 -mmxext"
, "qpel.m4v 352 288 0xc07eb687 -mmx"
, "qpel.m4v 352 288 0x54e720e0 -c"

, "lowdelay.m4v 720 576 0xf2a3229d -sse2"
, "lowdelay.m4v 720 576 0xf2a3229d -mmxext"
, "lowdelay.m4v 720 576 0xf2a3229d -mmx"
, "lowdelay.m4v 720 576 0x5ea8e958 -c"

, "gmc1.m4v 640 272 0x94f12062 -sse2"
, "gmc1.m4v 640 272 0x94f12062 -mmxext"
, "gmc1.m4v 640 272 0x94f12062 -mmx"
, "gmc1.m4v 640 272 0x3b938c99 -c"

);

#########################################
