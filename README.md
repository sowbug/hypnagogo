Lucid: Firmware for lucid-dreaming goggles
==========================================

This is adapted from an [Instructables project](http://www.instructables.com/id/The-Lucid-Dream-Machine/). With a
simple ATtiny13-based circuit, this firmware helps nudge your dreaming brain
into an *Inception*-like state of awareness. At least, that's the theory.
So far, I have slept through the whole experience and remembered nothing.

Ingredients
===========

* ATtiny13 or 13A, set to the default 1.2MHz internal oscillator.
* Two through-hole LEDs, preferably red because that's supposed to get through your closed eyelids more easily.
* At least two resistors in the 50-250 ohm range, low enough that you can see the lights in the dark, but not so high that your battery dies quickly.
* A battery and a holder for it. I used a CR1220.
* A momentary normally-closed switch (optional).
* A slider switch or a jumper with a shunt (optional).

The Circuit
===========

1. Tiny pin 2 to anode (positive) of one LED
1. Tiny pin 3 to one side of the momentary switch
1. Tiny pin 4 to ground
1. Tiny pin 7 to anode of the other LED
1. Tiny pin 8 to one side of the slider switch
1. For each LED, cathode (negative) to one end of a resistor
1. For each resistor, remaining end to ground
1. Remaining end of slider switch to battery's positive side
1. Battery's negative side to ground
1. Remaining end of the momentary switch to ground

In English, hook up the battery to the VCC/ground pins of the Tiny. Hook up the positive ends of the LEDs to pins 2 and 7 of the Tiny, putting in current-limiting resistors as appropriate. Insert a slider switch into the power circuitry if desired. Put a momentary switch between pin 3 and ground if desired.

Then compile the firmware (or use the .hex) and burn it to your tiny.

Operation
=========

When the circuit starts up, it lights both LEDs for about two seconds, then turns them off and goes to sleep for about 3.5 hours. It wakes up and pulses each LED, alternating back and forth at about 0.5Hz, for about 90 seconds, then goes back to sleep for half the previous sleep time (3.5 ÷ 2 = 1.75 hours). This cycle repeats, halving the sleep time each cycle, with at least 4 minutes separating each LED-pulsing phase.

Assuming about 8 hours of sleep starting at 10:30pm, this program will first blink at about 2am, then will continue blinking more frequently with a lot of blinking during the final hour of sleep. This pattern corresponds to a typical human's REM-sleep schedule (short and sparse at the beginning of sleep, long and frequent toward morning).

To see the LEDs blink immediately, hold down the button. This ends the current sleep period.

Disclaimer
==========

As stated above, this project hasn't yet done anything for me. And if for any reason you have problems wearing a circuit board on your head at night or lights flashing in your eyes, you should probably avoid this project.