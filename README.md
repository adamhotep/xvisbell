xvisbell
========

This is a simple program that flashes a white window on the screen whenever the
X11 bell is rung.

The typical way to use this program is to run it in the background in your
.xsession/.xinitrc file. Works best when used with dwm & xmonad & friends.

Why would I use this?
----------------------

If you're like Rian Hunter (this tool's original author)
and you are constantly pressing ^G in Emacs, audio bells can
be annoying. Visual bells are less annoying.

How do I configure this?
------------------------

Edit xvisbell.cpp to change code or Makefile to change targets.

To compile on Debian/Ubuntu, you'll need something like this:
`sudo apt install libxkbfile-dev`


What is the upstream repository for this code?
------------------------

It was previously on Github but now lives at https://thelig.ht/code/xvisbell/
which was last synced here on 2025-12-06.
