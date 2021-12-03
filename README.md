# Experimental cross platform C++ library to control colors for Das Keyboard X50Q

Only tested on Linux, but theoretically Windows and Mac should work too. Based on [hidapi](https://github.com/libusb/hidapi). On Linux the hidraw backend requires a patched kernel in order for the kernel's HID driver to handle interfaces without input interrupt endpoints,
so for an unpatched kernel the libusb backend has to be used.

This has been written without any official documentation of the interfaces from the vendor and is purely based on observing the behavior of the Windows driver.
**It might brick your keyboard. Use at your own risk.**

## Requirements
- A relativly modern C++ compiler
- [libfmt](https://github.com/fmtlib/fmt)
- [hidapi](https://github.com/libusb/hidapi)

All of this should be available from most package managers.

## Compilation
Currently the library is header-only, but some examples can be compiled with `make`.

## Remarks
Changing a single key color requires to reset the color of all the keys, so
in order to change the color of only a single key, your program has to keep track of the color of all other keys.
There does not seem to be a way to query them.

This library only deals with low-level communication with the keyboard and does not do any such state tracking.

The library often uses arrays of 144 codes. The order does not correspond to any kind of common scancodes/keycodes/whatever
that I've seen before and sometimes feels rather random. Since the keyboard does not actually contain 144 keys it also has quite some holes in rather random positions.
Colors/effects/... written to these position seem to get ignored.
The `test` example demonstrates all available positions and the source contains a list of the codes in a slightly more sensible order.

## Examples
`single_color.cpp` and `rainbow.cpp` demonstrate the general interface of the library.
They might also be a useful on their own. (`single_color` sets the keyboard to a single color chosen by the user,
`rainbow` recreates the animated rainbow pattern the keyboard gets shipped with)
