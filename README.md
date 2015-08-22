Sequitur
======================
![sequitur](https://cloud.githubusercontent.com/assets/2175324/3618522/edbceb22-0dea-11e4-9d05-fadac8860e5d.png)  

Sequitur is a BeOS/Haiku-native MIDI sequencer with a MIDI processing add-on architecture. It allows you
to record, compose, store, and play back music from your computer. Sequitur is designed for people
who like to tinker with their music.  It facilitates rapid, dynamic, and radical processing of your
performance.

This version (2.1.2) of the source compiles and runs under Haiku, without any so-far-detected limitations.
It is essentially identical in behaviour to the last 2.1 BeOS version, except that the FilePanels are a bit
smarter (they filter to show only MIDI files, and preserve the working directory).  It now saves files
with MimeType 'audio/midi', but recognizes 'audio/x-midi' and 'audio/mid' as well.

Features:

 * An add-on architecture for processing MIDI and performing MIDI effects. Change a track's velocity,
   apply echo, add chorus, change it's key signature, etc. All the tools to write your own add-ons are
   included.
 * MIDI processing is integrated with editing: Paint with echo effects, paint chords in key, etc.
 * Easily rearrange songs with familiar phrase operations.
 * Customize your interface with skinning.
 * Achieve special effects with tool properties: Turn on gradual delete and slowly rub out MIDI events.
 * Full support of Be's advanced MIDI services in R5: All R5-compatible software is automatically available.
 * Record, edit, and playback note, program change, control change, and pitch bend MIDI data.
 * Read and write standard MIDI files.

LICENSE: Public Domain
> All code here is free of any copyright, regardless of what might be written on any headers. 
> - Eric Hackborn

[Eric noted that the code is pretty awful -- he was still learning C++ through it all. (Programmer beware).]

----------------------

Fixes needed for this version were mainly the restoration of some missing code for Load/Store,
and some updates for Haiku compatibility -- in the area of static destructors (causing an exit crash)
and a disparity in the archiving of bitmaps.

This set of sources is complete for building Sequitur, but has been trimmed of ~5MB of unused
ARP code.  This is still available in the Haiku Archives if desired.

To build the release version of the app, go to the 'Sequitur' subdirectory and type 'make release'.
'make' alone will build the (30MB + 34MB library) debug version with symbols.   (interpose a
'make clean' if you switch between them...)

                                                                              -- Pete Goodeve --
                                                                                August 2015

