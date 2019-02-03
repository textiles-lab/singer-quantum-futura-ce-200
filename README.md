# Singer Quantum Futura CE 200

The Singer Quantum Futura CE 200 is an computer-controlled single-needle embroidery machine from the early 2000's.
Today, they are often available second hand for around two hundred dollars.

These machines are controlled by a host computer that connects over USB;
unfortunately, the software that comes with these machines requires Windows XP, and does not work on Linux, MacOS, or (reportedly) modern versions of Windows.
(Though we have had success running the software inside a Windows XP virtual machine.)

This git repository holds documentation about the USB communication protocol used by the machine and a small libusb-based command line utility that can send paths to the machine from Linux, MacOS, and -- likely -- Windows hosts.

## The `send-path` Utility

The `send-path` utility (in the `send-path` folder) can be used to send a path to the embroidery machine.
A path is a list of needle-down locations, expressed in integer machine coordinates.
For the large embroidery hoop, these coordinates range from (0xfeb8,0xfe1b) at the lower left to (0xffff,0xffff) at the upper right, for a total sewing area size of 328x485 machine units.

The `send-path` utility reads (and sends to the machine) a whitespace-separated lists of pairs of integers in this range. For example, this will draw a box of side length 8 using five needle-downs:
```
0xff00 0xff00
0xff08 0xff00
0xff08 0xff08
0xff00 0xff08
0xff00 0xff00
```
(Note that values can be in decimal `123`, octal `0123`, or hex `0x123`.)

Owing to limitations of the control protocol, paths must contain at least two needle-downs and subsequent positions may differ by no more than 28 (0x1c) machine units in either coordinate.

Once send-path has sent the path to the machine, it will wait forever, dumping information about machine status codes.

## Communication Protocol

We were able to use usbpcap to record the communications of the control software and the embroidery machine. Several captured traces are available in the `captures` folder.

Notes and experiments are in the `experiments` folder.

Generally, the communication protocol seems to work like this:
 - the host performs a handshake with the machine
 - the host initiates a path transfer
 - the host polls the machine to see if the path is done sewing yet

More detailed documentation of these TODO, see `experiments/usb-bird-bit.cpp` or `send-path/send-path.hpp`.

