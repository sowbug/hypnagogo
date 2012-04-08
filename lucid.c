/*********************************************
lucid13.c
V 0.2b

Lucid dreaming device

Software by gmoon (Doug Garmon)
Hardware by guyfrom7up (Brian _)

* Chip type : ATtiny13
*********************************************/
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
uint16_t flash_count;

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
  flash_count = 120;  // This is a guess.
  set_fast_timer();
  reset_fast_interrupts_left();
}

static void reset_slow_interrupts_left(int long_delay) {
  if (long_delay) {
    interrupts_left = 3.8 * 60 * 60 * SLOW_HZ;  // 3.8 hours
  } else {
    uint16_t MIN_DELAY_COUNT = 4 * 60 * SLOW_HZ;  // 4 minutes
    static uint16_t short_delay_count = 30 * 60 * SLOW_HZ;  // 30 minutes
    interrupts_left = short_delay_count;
    short_delay_count = short_delay_count / 2;
    if (short_delay_count < MIN_DELAY_COUNT) {
      short_delay_count = MIN_DELAY_COUNT;
}
  }
}

static void switch_to_WAITING(int long_delay) {
  mode = MODE_WAITING;
  set_slow_timer();
  reset_slow_interrupts_left(long_delay);
}

static void leds_on() {
  PORTB |= (LEDS);
}

static void leds_off() {
  PORTB &= ~(LEDS);
}

// IRQ vector
ISR (TIM0_OVF_vect) {
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
          switch_to_WAITING(FALSE);  // back to waiting, but for a short time
        }
      }
    }
    break;

  case MODE_WAITING:
    if (interrupts_left == 0)
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
  uint8_t button_pressed;

  DDRB &= ~_BV(DDB4); // clear bit, input fire-button
  PORTB |= _BV(BUTTON); // set bit, enable pull-up resistor

  DDRB |= _BV(DDB3) | _BV(DDB2); // set output

  // Check if button is pressed when powering up...
  if (!(PINB & _BV(BUTTON)))
    button_pressed = 1;

  // init the IRQ
  irqinit();

  // turn on LED,
  // delay before checking user input again
  interrupts_left = SLOW_HZ * 2;
  while (interrupts_left > 0) {
    leds_on();
  }
  leds_off();

  // button still pressed? Enter immediate mode...
  if (!(PINB & _BV(BUTTON)) && button_pressed) {
    switch_to_DREAM();
  } else {
    switch_to_WAITING(TRUE);  // 1 = 3.8-hour delay
  }

  // place the CPU into idle mode
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();

  // infinite loop--the IRQ does all the work...
  while (TRUE) {
  }

  return 0;
}
