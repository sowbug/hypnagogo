Dream Machine: Firmware and Circuit Design for Lucid-Dreaming Goggles
=====================================================================

This is adapted from an [Instructables
project](http://www.instructables.com/id/The-Lucid-Dream-Machine/). With a
simple ATtiny13-based circuit, this firmware helps nudge your dreaming brain
into an *Inception*-like state of awareness. At least, that's the theory. So
far, I have slept through the whole experience and remembered nothing.

See [the firmware](https://github.com/sowbug/dream-machine) and
[the hardware](https://github.com/sowbug/dream-machine-hw), and have a look
at an untested [Mouser cart](http://www.mouser.com/ProjectManager/ProjectDetail.aspx?AccessID=e0a3dbcca8).

Ingredients
===========

* ATtiny13 or 13A, set to the default 1.2MHz internal oscillator.

* Two through-hole LEDs, preferably red because that's supposed to get through
  your closed eyelids more easily.

* At least two resistors in the 50-250 ohm range, low enough that you can see
  the lights in the dark, but not so high that your battery dies quickly.

* A battery and a holder for it. I used a CR1220.

* A momentary normally-closed switch.

Operation
=========

When the circuit starts up, it's in the power-off state. Tap the button to
turn it on and start the approximately 9-hour sleep cycle. Hold down for two
seconds to turn back off again. You'll see a quick blink sequence before
shutdown.

At the start of the sleep cycle, the device lights both LEDs for about two
seconds just to let you know the sleep cycle began. It then turns them off and
goes to a waiting state for about 3.5 hours. It wakes up and goes into the
dream-induction state, pulsing each LED, alternating back and forth at about
0.5Hz, for about 30 seconds. It then returns to the waiting state for half the
previous time (3.5 ÷ 2 = 1.75 hours). This pattern repeats, halving the
waiting time each time through, with at least 4 minutes separating each
dream-induction phase.

After 15 consecutive dream inductions at the minimum 4-minute wait time, the
device shuts itself off.

Assuming about 8 hours of sleep starting at 10:30pm, this program will first
blink at about 2am, then will continue blinking more frequently with a lot of
blinking during the final hour of sleep. This pattern corresponds to a typical
human's REM-sleep schedule (short and sparse at the beginning of sleep, long
and frequent toward morning).

To see the LEDs blink immediately, tap the button. This ends the current
waiting period, sets the next wait state to four times the minimum duration
(16 minutes), and goes straight to the next dream-induction cycle. This is
good for testing or for taking naps.

To Do
=====

* Run the 'tiny at an even slower speed. 128KHz ought to be fine, and it'll use even less power.
* Experiment with different light patterns.

Disclaimer
==========

As stated above, this project hasn't yet done anything for me. And if for any
reason you have problems wearing a circuit board on your head at night or
lights flashing in your eyes, you should probably avoid this project.
