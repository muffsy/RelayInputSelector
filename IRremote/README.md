# IRremote Arduino Library - v. 2.4.0b - Patched for ESP-32

This library enables you to send and receive using infra-red signals on an Arduino.

Tutorials and more information will be made available on [the official homepage](http://z3t0.github.io/Arduino-IRremote/).

## Changes made:
The IR receiver can now be turned on and off, using "irrecv.enableIRIn(true);" and "irrecv.enableIRIn(false);" instead of the library's default "irrecv.enableIRIn();".

The reason for this is that any attempt to write to nvram while the IR receiver is turned on will send the ESP-32 into a reboot loop. The code for the Muffsy Relay Input Selector turns off the IR receiver right before writing to nvram, and turns the IR receiver on again when the nvram write operation has ended.

SEND_PIN is not defined for ESP-32, and it would prevent any project using IRremote.h from compiling. This has been fixed using better if-statement.

None of the changes to the IRremote library should affect other platforms, and it adds the ability to turn the IR receiver on an off.

## Copyright
Copyright 2009-2012 Ken Shirriff
