/*
 * lucid13.c
 * forked from V 0.2b
 * Lucid dreaming device
 * Original project on http://www.instructables.com/id/The-Lucid-Dream-Machine/
 *
 * Software by gmoon (Doug Garmon)
 * Hardware by guyfrom7up (Brian _)
 *
 * Chip type : ATtiny13 or ATtiny 13A
 */
#define F_CPU (1200000UL) // 1.2 MHz default clock

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* ramping fx increments */
#define PWM_VAL 4
#define TRANS_VAL 6

/* the overall pulse width and delay between pulses */
#define MACRO_WIDTH 1500
#define MACRO_GAP 1500

// Don't use this in real calculations unless you have room for the FP
// library to be linked in
#define SLOW_HZ (4.57)

#define TRUE (1)
#define FALSE (!TRUE)

enum {
  MODE_WAITING,
  MODE_DREAM
};
uint8_t mode;

#define BUTTON PB4          // button, pin 3 to ground
#define LED_LEFT _BV(PB3)   // pin 2
#define LED_RIGHT _BV(PB2)  // pin 7
#define LEDS (LED_LEFT | LED_RIGHT)

volatile uint16_t interrupts_left;
uint8_t flash_count;

void set_fast_timer() {
  TCCR0B = _BV(CS00); // new prescaler
}

static void set_slow_timer() {
  TCCR0B = _BV(CS02) | _BV(CS00); // new prescaler
}

static void reset_fast_interrupts_left() {
  interrupts_left = MACRO_WIDTH + MACRO_GAP;
}

static void switch_to_DREAM() {
  mode = MODE_DREAM;
  flash_count = 88;  // This is a guess.
  set_fast_timer();
  reset_fast_interrupts_left();
}

/*
 Assuming bedtime of 10:30pm, will flash on this schedule:

 2:00:00 AM
 3:45:00 AM
 4:37:30 AM
 5:03:45 AM
 5:16:52 AM
 5:23:26 AM
 5:27:26 AM
 5:31:26 AM
 5:35:26 AM
 5:39:26 AM
 5:43:26 AM
 5:47:26 AM
 5:51:26 AM
 5:55:26 AM
 5:59:26 AM
 6:03:26 AM

*/
#define START_INTERRUPT_COUNT (57582)  // 3.5 * 60 * 60 * SLOW_HZ, 30 minutes
#define MIN_INTERRUPT_COUNT (1096)  // 4 * 60 * SLOW_HZ, 4 minutes
static uint16_t interrupt_count = START_INTERRUPT_COUNT;
static void reset_slow_interrupts_left() {
  interrupts_left = interrupt_count;
  interrupt_count = interrupt_count >> 1;
  if (interrupt_count < MIN_INTERRUPT_COUNT) {
    interrupt_count = MIN_INTERRUPT_COUNT;
  }
}

static void set_slow_interrupts_to_min() {
  interrupt_count = MIN_INTERRUPT_COUNT;
}

static void switch_to_WAITING() {
  mode = MODE_WAITING;
  set_slow_timer();
  reset_slow_interrupts_left();
}

static void leds_on() {
  PORTB |= (LEDS);
}

static void leds_off() {
  PORTB &= ~(LEDS);
}

int is_button_pressed() {
  return !(PINB & _BV(BUTTON));
}

ISR(TIM0_OVF_vect) {
  static uint8_t pwm;
  static uint16_t transition;
  static uint8_t current_led = LED_LEFT;

  --interrupts_left;

  switch (mode) {
  case MODE_DREAM:
    if (interrupts_left >= MACRO_WIDTH) {
      pwm += PWM_VAL;
      if (pwm > transition)
        PORTB &= ~(current_led);
      else
        PORTB |= current_led;

      if (!pwm)
        transition += TRANS_VAL;
    } else {
      // delay between pulses
      pwm = transition = 0;
      leds_off();
      if (interrupts_left == 0) {
        if (current_led == LED_LEFT) {
          current_led = LED_RIGHT;
        } else {
          current_led = LED_LEFT;
        }
        reset_fast_interrupts_left();
        if (--flash_count == 0) {
          switch_to_WAITING();
        }
      }
    }
    break;

  case MODE_WAITING:
    if (interrupts_left == 0 || is_button_pressed())
      switch_to_DREAM();
    break;
  }
}

// init the IRQ
void irqinit (void) {
  set_slow_timer();

  // Enable timer overflow irq
  TIMSK0 = _BV(TOIE0);
  sei(); // IRQ on
}

int main(void) {
  DDRB &= ~_BV(DDB4); // clear bit, input fire-button
  PORTB |= _BV(BUTTON); // set bit, enable pull-up resistor

  DDRB |= _BV(DDB3) | _BV(DDB2); // set output

  // init the IRQ
  irqinit();

  // delay before checking user input again
  leds_on();
  interrupts_left = 9; // SLOW_HZ * 2, about 2 seconds
  while (interrupts_left > 0) {
  }
  leds_off();

  if (is_button_pressed()) {
    set_slow_interrupts_to_min();
    switch_to_DREAM();
  } else {
    switch_to_WAITING();
  }

  // place the CPU into idle mode
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();

  // infinite loop--the IRQ does all the work...
  while (TRUE) {
  }

  return 0;
}
