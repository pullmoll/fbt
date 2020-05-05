# Frame Buffer Tool (fbt)

A simple tool to display things on a Raspberry Pi (TFT) frame buffer
device. It expects /dev/fb1 to be the device to use and currently
works on a Raspberry Pi Zero W with a 128x128 pixel TFT shield.

It uses the [GD library](https://libgd.github.io/) to handle loading
images, scaling them (down) to the TFT size.

The fbt.c code then copies the data from the GD image to the frame
buffer which is memory mapped into the program's address space.

There's also code to directly write glyphs into the frame buffer.
A very basic and simple fb_printf() uses an integrated 6x8 font.

Usage: fbt [OPTIONS] <image.ext> […]
Where [OPTIONS] is one or more of:
-u        Up scale small images to the TFT size.
-v        Be verbose.
-fb=<dev> Use frame buffer device <dev> (e.g. /dev/fb2)

