/*
 * The Hypna Go Go
 * https://github.com/sowbug/hypnagogo/
 * Hardware at https://github.com/sowbug/hypnagogo-hw/
 *
 * Inspired by http://www.instructables.com/id/The-Lucid-Dream-Machine/
 * by gmoon (Doug Garmon) and guyfrom7up (Brian _)
 *
 */

// Button between pin 7 and ground
// Left LED on pin 2
// Right LED on pin 3
//
#if !defined(OLD_HARDWARE)
// New ISP-capable hardware
#define BUTTON _BV(PB2)
#define BUTTON_DD _BV(DDB2)
#define BUTTON_PCINT _BV(PCINT2)
#define LED_RIGHT _BV(PB4)
#define LED_RIGHT_DD _BV(DDB4)
#else
// Old boneheaded hardware
#define BUTTON _BV(PB4)
#define BUTTON_DD _BV(DDB4)
#define BUTTON_PCINT _BV(PCINT4)
#define LED_RIGHT _BV(PB2)
#define LED_RIGHT_DD _BV(DDB2)
#endif

#define LED_LEFT _BV(PB3)
#define LED_LEFT_DD _BV(DDB3)
#define LEDS (LED_LEFT | LED_RIGHT)

static void set_fast_timer() {
  TCCR0B = _BV(CS00);
}

static void set_slow_timer() {
  TCCR0B = _BV(CS02) | _BV(CS00);
}

static void enable_timer_prr() {
  PRR = _BV(PRADC);  // Timer on, ADC off.
}

static void disable_timer_prr() {
  PRR = _BV(PRADC) | _BV(PRTIM0);  // All peripherals off.
}

static void enable_timer_interrupt() {
  TIMSK0 = _BV(TOIE0);
}

static void enable_pin_interrupts() {
  GIMSK = _BV(PCIE);  // Enable pin-change interrupts
  PCMSK = BUTTON_PCINT;  // Mask off all but the button pin for interrupts
}

static void set_port_state() {
  DDRB &= ~BUTTON_DD; // set button to input
  PORTB |= BUTTON; // enable button's pull-up resistor
  DDRB |= LED_LEFT_DD | LED_RIGHT_DD; // set LEDs to output
}

static void leds_on() {
  PORTB |= (LEDS);
}

static void leds_off() {
  PORTB &= ~(LEDS);
}
