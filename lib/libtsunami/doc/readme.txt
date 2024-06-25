Tsunami for KOS ##version##
Copyright (C) 2002 Megan Potter


What is it?
-----------

Tsunami builds upon Parallax to provide a "scene graph" library in C++.
Most of the code is pulled directly from Feet of Fury's code base or
designed around the same concepts will replace the corresponding pieces
in FoF.

While working on Feet of Fury, I quickly discovered that there was no way
I could manage all the things happening at once in any reasonable manner
without some way to allow each visual aspect of the screen to "control
itself".

Tsunami provides a high level way to describe what a scene looks like, and
then you simply call into it to have it manage everything each frame. Each
scene is composed of one or more "drawables" (one of which is the whole scene
container). Each of these drawables has a screen position and other
attributes that describe its state. Additionally, each drawable can contain
sub-drawables which can be positioned, rotated, etc, relative to its
parents. Furthermore, each drawable knows how to draw itself in position
and how to handle any intrinsic animations. In this way it is possible to
build up some fairly complex scenes without much effort.

Needless to say, these drawables are all subclasses of a root Drawable
class. Drawable has a virtual method called for each list, and a "next
frame" method called per frame to advance any animation counters. Note
that you, the programmer, control both the top and bottom end of the
library. So the only lists that are called on each object are the ones
that you call on the scene graph, and each of your drawables (or the ones
in the library) can choose to respond to each list or not.

Generic animation objects can also be attached to drawables to provide
additional animation unrelated to the object itself. For example, one
animation I have moves the position of a drawable towards some target
with a logarithmic speed curve. Another one fades the alpha blending value
of the object to some new value. Of course, all of these can be done at
once and in tandem with the object's intrinsic animation(s), if any.

Triggers provide a simple mechanism to have a multi-legged animation. The
current trigger mechanism allows for you to do things like switch out an
animation or signal that a drawable is dead in the scene upon the completion
of another animation, but I'm also planning to add triggers based on time
and other events (like controller inputs).

Environmental objects provide a hook into or out of the system for things
that aren't directly drawn but are part of the overall look and feel of
the scene -- such as background music, rendering background color,
controller inputs, etc.

A simple reference counting base class ties all of the resource management
together, since many things can be "owned" by more than one object (such as
generic animations and triggers). Additionally, auto_ptr types are used
with the reference counted objects pervasively, dodging the bullet that
things like CComPtr setup in the MS world (i.e. mixing of raw pointers and
auto_ptrs, causing the programmer to have to defacto manually reference
count anyway).
