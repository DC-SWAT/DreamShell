Parallax for KOS ##version##
(c)2002 Dan Potter


What is it?
-----------

Parallax is my answer to wanting a nice, simple API with which to write
mostly-2D games for KOS. Brian Peek and I both needed something like this
which is faster (and cuts around much of the properness of things like KGL)
for our projects, and so we sat down and came up with a basic list of 
requirements and specs, and I got to coding.


Basic requirements
------------------

These are the basic requirements we shared:

- Emphasis on 2D usage, but allow for 3D to be integrated as well without
  having to fall back to using parts of KGL or whatnot.

- Speed is an essential priority for things like submitting sprites and
  vertices; thus anything in the main code path needs to be inlined or
  made into macros if at all possible, and even use DR where possible.
  
- Everything should be based around supporting and enhancing the native
  libraries rather than replacing them. Thus Parallax can be used with the
  PVR API or inside a KGL program as if it were all straight PVR calls.

- Provide typedefs and macros for common DC anachronisms so that if a port
  is done of the library to another platform later, porting any client code
  that uses it is a lot easier.

- All the basic functionality should be C-based so that it can be easily
  plugged into various C++ high-level libraries or used from C programs by
  other programmers.

More specific goals / features can be seen in the document "specs.txt".


License
-------

Parallax is licensed under the same license as KOS (BSD-style). See
README.KOS for more specific information.

