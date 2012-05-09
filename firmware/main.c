/*
 * The Hypna Go Go
 * https://github.com/sowbug/hypnagogo/
 * Hardware at https://github.com/sowbug/hypnagogo-hw/
 *
 * Inspired by http://www.instructables.com/id/The-Lucid-Dream-Machine/
 * by gmoon (Doug Garmon) and guyfrom7up (Brian _)
 *
 */


#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <inttypes.h>
#include <util/delay.h>

#include "devices/attiny13.h"

// We use a sort of fixed-point decimal to let us compute durations with
// reasonable precision without bringing in FPU code that wouldn't fit in
// a small flash program space.
//
// "+ 8" = 8-bit timer overflow = 8 bits = 256 cycles = Hz / 256
// "- 8" = x256 for precision
// For slow: "10" = CS02 | CS00 = clkIO / 1024 Hz
// For fast: "0"  = CS00 = clkIO, no prescaling
//
#define SLOW_HZ_X_256 (F_CPU >> (10 + 8 - 8))
#define FAST_HZ_X_256 (F_CPU >> (0 + 8 - 8))

#ifdef PRODUCTION
// Normal cycle representing an approximately 8-hour sleep cycle
#define START_WAIT_DURATION_SECONDS (12600)  // 3.5 * 60 * 60, 3.5 hours
#define MIN_WAIT_DURATION_SECONDS (480)      // 8 * 60, 8 minutes
#define FLASH_COUNT (64)
#else
// Short durations for easier development
#define START_WAIT_DURATION_SECONDS (32)
#define MIN_WAIT_DURATION_SECONDS (4)
#define FLASH_COUNT (8)
#endif
#define START_INTERRUPT_COUNT                           \
  ((START_WAIT_DURATION_SECONDS * SLOW_HZ_X_256) >> 8)
#define MIN_INTERRUPT_COUNT                           \
  ((MIN_WAIT_DURATION_SECONDS * SLOW_HZ_X_256) >> 8)

// Overall pulse width and delay between pulses
// The "+ 10" is 1024, which is close to dividing by 1,000 for msec -> sec
#define PULSE_DURATION_MSEC (500)
#define PULSE_ON_MSEC (450)
#define PULSE_ON_CYCLES ((PULSE_ON_MSEC * FAST_HZ_X_256) >> (8 + 10))
#define PULSE_OFF_MSEC (PULSE_DURATION_MSEC - PULSE_ON_MSEC)
#define PULSE_OFF_CYCLES ((PULSE_OFF_MSEC * FAST_HZ_X_256) >> (8 + 10))

// This number isn't CPU-speed dependent, but it must be an even
// divisor of 256. It represents the granularity of the PWM's duty
// cycle.
#define PWM_SLICE (4)

// Each cycle should add this number to reach max desired brightness by the
// halfway point to the end of PULSE_ON_CYCLES cycles.
//#define ON_OFF_SLICE (PULSE_ON_CYCLES >> 8)  // 100%
#define ON_OFF_SLICE (PULSE_ON_CYCLES >> 10)  // 25% (?) brightness

#define SHORT_INDUCTIONS_BEFORE_POWER_DOWN (16)

enum {
  STATE_POWER_DOWN = 0,
  STATE_INIT,
  STATE_IDLE,
  STATE_DREAM
};

static uint16_t interrupt_count;
static uint16_t interrupts_left;
static uint8_t current_led;
static uint8_t flash_count;
static volatile uint8_t button_was_pressed;
static volatile uint8_t min_induction_count;
static volatile uint8_t short_inductions_before_power_down;
static volatile uint8_t state = -1;

static void reset_idle_interrupts_left() {
  interrupts_left = interrupt_count;
  if (interrupt_count > MIN_INTERRUPT_COUNT) {
    interrupt_count = interrupt_count >> 1;
  } else {
    interrupt_count = MIN_INTERRUPT_COUNT;
    min_induction_count++;
  }
}

static void reset_init_interrupts_left() {
  interrupts_left = 9;  // about 2 seconds
}

static void flash_leds(uint8_t count) {
  while (count--) {
    _delay_ms(50);
    leds_on();
    _delay_ms(5);
    leds_off();
  }
}

static void start_POWER_DOWN() {
  if (state != STATE_POWER_DOWN) {
    flash_leds(4);
  }
  state = STATE_POWER_DOWN;
}

static int is_button_pressed() {
  return !(PINB & BUTTON);
}

static void wait_for_button_up() {
  _delay_ms(25);  // Debounce.

  // Wait for user to let go of button.
  while (is_button_pressed()) {
    leds_on();
    _delay_ms(5);
    leds_off();
    _delay_ms(250);
  }
}

static void start_INIT() {
  leds_on();
  set_slow_timer();
  state = STATE_INIT;
  reset_init_interrupts_left();
}

static void start_IDLE() {
  leds_off();
  set_slow_timer();
  state = STATE_IDLE;
  reset_idle_interrupts_left();
}

static void reset_dream_interrupts_left() {
  interrupts_left = PULSE_ON_CYCLES + PULSE_OFF_CYCLES;
}

static void start_DREAM() {
  leds_off();
  set_fast_timer();
  state = STATE_DREAM;
  flash_count = FLASH_COUNT;
  current_led = LED_LEFT;
  reset_dream_interrupts_left();
}

// Allow the user to hold down the button for two seconds to power down.
static int power_down_pressed() {
#define MSEC_TO_WAIT (2000)
#define WAIT_QUANTUM_MSEC (100)

  int i = (MSEC_TO_WAIT / WAIT_QUANTUM_MSEC);
  do {
    if (!is_button_pressed()) {
      return 0;
    }
    _delay_ms(WAIT_QUANTUM_MSEC);
  } while (--i);
  wait_for_button_up();
  return 1;
}

static void handle_button_INIT() {
}

// Assumes we're already in the IDLE state.
static void start_nap() {
  interrupts_left = MIN_INTERRUPT_COUNT * 2;
  short_inductions_before_power_down = SHORT_INDUCTIONS_BEFORE_POWER_DOWN / 2;
}

static void handle_button_IDLE() {
  start_nap();
  start_DREAM();
}

static void handle_button_DREAM() {
}

ISR(PCINT0_vect) {
  // We care only about a button-down event.
  if (!is_button_pressed()) {
    return;
  }

  if (power_down_pressed()) {
    start_POWER_DOWN();
  } else {
    switch (state) {
    case STATE_INIT: handle_button_INIT(); break;
    case STATE_IDLE: handle_button_IDLE(); break;
    case STATE_DREAM: handle_button_DREAM(); break;
    }
  }
}

static void handle_end_INIT() {
  start_IDLE();
}

static void handle_end_IDLE() {
  start_DREAM();
}

static void handle_end_DREAM() {
  if (current_led == LED_LEFT) {
    current_led = LED_RIGHT;
  } else {
    current_led = LED_LEFT;
  }
  reset_dream_interrupts_left();
  if (--flash_count == 0) {
    start_IDLE();
  }
}

static void handle_work_INIT() {
}

static void handle_work_IDLE() {
}

static void handle_work_DREAM() {
  static uint8_t pwm_slices;
  static uint16_t on_off_slices;

  if (interrupts_left >= PULSE_OFF_CYCLES) {
    pwm_slices += PWM_SLICE;
    if (pwm_slices <= on_off_slices)
      PORTB |= current_led;
    else
      PORTB &= ~current_led;

    if (pwm_slices == 0)
      on_off_slices += ON_OFF_SLICE;
  } else {
    pwm_slices = on_off_slices = 0;
    leds_off();
  }
}

ISR(TIM0_OVF_vect) {
  if (interrupts_left-- == 0) {
    switch (state) {
    case STATE_INIT: handle_end_INIT(); break;
    case STATE_IDLE: handle_end_IDLE(); break;
    case STATE_DREAM: handle_end_DREAM(); break;
    }
    if (interrupts_left == 0) {
      // Programming error. Halt loudly.
      while (1) {
        flash_leds(255);
      }
    }
  } else {
    switch (state) {
    case STATE_INIT: handle_work_INIT(); break;
    case STATE_IDLE: handle_work_IDLE(); break;
    case STATE_DREAM: handle_work_DREAM(); break;
    }
  }
}

static void power_down() {
  disable_timer_prr();
  leds_off();

  start_POWER_DOWN();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();

  // If we reach this point, the user pressed the button.
  enable_timer_prr();
}

static void start_sleep_mode() {
  enable_timer_interrupt();

  set_slow_timer();

  interrupt_count = START_INTERRUPT_COUNT;
  min_induction_count = 0;
  short_inductions_before_power_down = SHORT_INDUCTIONS_BEFORE_POWER_DOWN;
  start_INIT();

  set_sleep_mode(SLEEP_MODE_IDLE);
}

int main(void) {
  set_port_state();
  sei();
  enable_pin_interrupts();
  state = -1;  // Ensure that the power-down flash cycle will happen

  while (1) {
    power_down();
    start_sleep_mode();
    while (state != STATE_POWER_DOWN &&
           min_induction_count < short_inductions_before_power_down) {
      sleep_mode();
    }
  }
  return 0;
}
