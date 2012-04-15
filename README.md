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

Features
========

* Tuned for a typical 8- or 9-hour nighttime sleep, with long separations between inductions early in the night, and rapid inductions near morning.
* Nap mode, with quick inductions in an 80-minute program.
* Auto-shutoff at program end.
* Manual shutoff by holding down button for two seconds.
* Extremely low power consumption. A 40mAh CR1220 should last months or years.

Operation
=========

When the battery is first installed, the device is in the power-off state. Tap the button to turn it on. You'll see steady lights for about two seconds. The 9-hour sleep cycle has begun. Place the board over your eyes. I use a sleep mask to hold mine in place.

The sleep cycle starts with an idle state of about 3.5 hours. When idle ends, dream induction begins. During induction, the LEDs alternate gently blinking for about 30 seconds. The device then returns to idle, but the idle period each time is halved: 1.75 hours, then 52 minutes, then 26, and so on down to a minimum of 8 minutes separating each induction. After 16 consecutive eight-minute inductions, the device powers off.

For example:

1. Go to sleep at 10:30pm. Turn on the device.
2. The first induction will happen around 2am.
3. The second induction will happen one hour and 45 minutes later, at 3:45am.
4. At 5:16am, the device will begin inductions every 8 minutes.
5. At about 7:30am, the device shuts off.

This pattern corresponds to a typical human's REM-sleep schedule (short and sparse at the beginning of sleep, long and frequent toward morning).

Nap mode starts with a button tap while the device is idling (i.e., powered on but not blinking). The only difference is that it starts with an immediate induction followed by a 16-minute idle time, and it powers off after only eight consecutive 8-minute inductions, for a total cycle of about 80 minutes.

Hold the button down for two seconds to turn the device off. You'll see a quick blink sequence to confirm shutdown.

To Do
=====

* Run the 'tiny at an even slower speed. 128KHz ought to be fine, and it'll use even less power.
* Experiment with different light patterns.

Disclaimer
==========

As stated above, this project hasn't yet done anything for me. And if for any
reason you have problems wearing a circuit board on your head at night or
lights flashing in your eyes, you should probably avoid this project.
