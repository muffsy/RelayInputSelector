# Muffsy Relay Input Selector 4

Arduino Code, Patched IRremote.h, Eagle project files

- https://www.muffsy.com/muffsy-relay-input-selector-4.html
- https://www.tindie.com/products/skrodahl/muffsy-relay-input-selector-kit/

The Muffsy Relay Input Selector is a simple, reliable, high quality, DIY stereo input selector that's controlled by an ESP-32 and is slightly larger than a Post-IT note.

This kit is meant to be paired with a preamplifier circuit, such as the Muffsy BSTRD Class-A Tube Preamp, Pass Labs B1 and many others.

This project is open source, licensed under the BSD 3-Clause "New" or "Revised" License

A note on the IRremote.h library:
The IR receiver can now be turned on and off, using:
  irrecv.enableIRIn(true);
  irrecv.enableIRIn(false);
instead of the library's default
  irrecv.enableIRIn();
  
The reason for this is that any attempt to write to nvram while the IR receiver is turned on will send the ESP32 into a reboot loop. The code for the Muffsy Relay Input Selector turns off the IR receiver right before writing to nvram, and turns the IR receiver on again when the nvram write operation has ended.
