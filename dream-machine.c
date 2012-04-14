/*
 * Dream Machine
 * https://github.com/sowbug/dream-machine/
 * Hardware at https://github.com/sowbug/dream-machine-hw/
 *
 * Based on http://www.instructables.com/id/The-Lucid-Dream-Machine/
 * by gmoon (Doug Garmon) and guyfrom7up (Brian _)
 *
 * Chip type: ATtiny13 or ATtiny 13A
 * Assuming default fuses:
 *   -U lfuse:w:0x6A:m
 *   -U hfuse:w:0xFF:m
 *   -U lock:w:0xFF:m
 */
#define F_CPU (1200000UL) // 1.2 MHz default clock

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

/* ramping fx increments */
#define PWM_VAL 4
#define TRANS_VAL 6

/* the overall pulse width and delay between pulses */
#define MACRO_WIDTH 1500
#define MACRO_GAP 250

#if 1
// Normal
#define START_INTERRUPT_COUNT (57678)  // 3.5 * 60 * 60 * SLOW_HZ, 30 minutes
#define MIN_INTERRUPT_COUNT (2197)  // 8 * 60 * SLOW_HZ, 8 minutes
#define FLASH_COUNT (64)
#else
// Debugging
#define START_INTERRUPT_COUNT (69)  // 15 seconds
#define MIN_INTERRUPT_COUNT (14)  // 3 seconds
#define FLASH_COUNT (4)
#endif

// Don't use this in real calculations unless you have room for the FP
// library to be linked in
// #define SLOW_HZ (4.57)

#define TRUE (1)
#define FALSE (!TRUE)

enum {
  STATE_POWER_DOWN,
  STATE_INIT,
  STATE_WAITING,
  STATE_DREAM
};

#define BUTTON PB2          // button, pin 7 to ground
#define LED_LEFT _BV(PB3)   // pin 2
#define LED_RIGHT _BV(PB4)  // pin 3
#define LEDS (LED_LEFT | LED_RIGHT)

static void set_fast_timer() {
  TCCR0B = _BV(CS00);
}

static void set_slow_timer() {
  TCCR0B = _BV(CS02) | _BV(CS00);
}

static void leds_on() {
  PORTB |= (LEDS);
}

static void leds_off() {
  PORTB &= ~(LEDS);
}

static volatile uint8_t state;
static uint16_t interrupts_left;
static uint8_t flash_count;
static uint16_t interrupt_count;
static uint8_t current_led;
static uint8_t button_was_pressed;
static uint8_t quick_induction_count;
static void reset_waiting_interrupts_left() {
  interrupts_left = interrupt_count;
  interrupt_count = interrupt_count >> 1;
  if (interrupt_count < MIN_INTERRUPT_COUNT) {
    interrupt_count = MIN_INTERRUPT_COUNT;
    quick_induction_count++;
  }
}

static void start_nap_mode() {
  interrupts_left = MIN_INTERRUPT_COUNT * 4;
}

static void reset_init_interrupts_left() {
  interrupts_left = 9;  // about 2 seconds
}

static void flash_leds(uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    _delay_ms(50);
    leds_on();
    _delay_ms(5);
    leds_off();
  }
}

static void switch_to_POWER_DOWN() {
  if (state != STATE_POWER_DOWN) {
    flash_leds(4);
  }
  state = STATE_POWER_DOWN;
}

static int is_button_pressed() {
  return !(PINB & _BV(BUTTON));
}

static int was_button_pressed() {
  uint8_t r = button_was_pressed;
  button_was_pressed = 0;
  return r;
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

static void switch_to_INIT() {
  leds_on();
  set_slow_timer();
  state = STATE_INIT;
  reset_init_interrupts_left();
}

static void switch_to_WAITING() {
  leds_off();
  set_slow_timer();
  state = STATE_WAITING;
  reset_waiting_interrupts_left();

  // Hack: reset the button-pressed indicator. On the breadboard version
  // of this circuit, I was going straight into nap mode after powering on
  // with the button. I debounced and checked states, but nothing worked. I
  // see nothing like this on the real hardware. I'm wondering whether the
  // breadboard is noisy, or perhaps the ISP was sending ambiguous signals.
  was_button_pressed();
}

static void reset_dream_interrupts_left() {
  interrupts_left = MACRO_WIDTH + MACRO_GAP;
}

static void switch_to_DREAM() {
  leds_off();
  set_fast_timer();
  state = STATE_DREAM;
  flash_count = FLASH_COUNT;
  current_led = LED_LEFT;
  reset_dream_interrupts_left();
}

ISR(PCINT0_vect) {
  // We care only about a button-down event.
  if (!is_button_pressed()) {
    return;
  }

  // Button is down. If we were powered down, let's wake up.
  if (state == STATE_POWER_DOWN) {
    wait_for_button_up();
    return;
  }

  int i;
  for (i = 0; i < 20; ++i) {
    if (!is_button_pressed()) {
      break;
    }
    _delay_ms(100);
  }
  if (i < 20) {
    // We're in some other state. Mark the button pressed and move on.
    button_was_pressed = 1;
  } else {
    // The user held the button down for 2 seconds.
    wait_for_button_up();
    switch_to_POWER_DOWN();
  }
}

ISR(TIM0_OVF_vect) {
  if (interrupts_left == 0) {
    switch (state) {
    case STATE_INIT:
      switch_to_WAITING();
      break;

    case STATE_DREAM:
      if (current_led == LED_LEFT) {
        current_led = LED_RIGHT;
      } else {
        current_led = LED_LEFT;
      }
      reset_dream_interrupts_left();
      if (--flash_count == 0) {
        switch_to_WAITING();
      }
      break;

    case STATE_WAITING:
      switch_to_DREAM();
      break;
    }
  } else {
    --interrupts_left;

    switch (state) {
    case STATE_INIT:
      break;

    case STATE_DREAM: {
      static uint8_t pwm;
      static uint16_t transition;

      if (interrupts_left >= MACRO_GAP) {
        pwm += PWM_VAL;
        if (pwm > transition)
          PORTB &= ~(current_led);
        else
          PORTB |= current_led;

        if (!pwm)
          transition += TRANS_VAL;
      } else {
        pwm = transition = 0;
        leds_off();
      }
    }
      break;

    case STATE_WAITING:
      if (was_button_pressed()) {
        start_nap_mode();
        switch_to_DREAM();
      }
      break;
    }
  }
}

static void enable_timer_prr() {
  PRR = 0;  // No peripherals
}

static void disable_timer_prr() {
  PRR = _BV(PRTIM0);  // Just the timer
}

static void power_down() {
  disable_timer_prr();

  switch_to_POWER_DOWN();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();

  // If we reach this point, the user pressed the button.
  enable_timer_prr();
}

static void start_sleep_cycle() {
  TIMSK0 = _BV(TOIE0);  // Enable timer overflow irq

  set_slow_timer();

  interrupt_count = START_INTERRUPT_COUNT;
  quick_induction_count = 0;
  switch_to_INIT();

  set_sleep_mode(SLEEP_MODE_IDLE);
}

int main(void) {
  DDRB &= ~_BV(DDB2); // set button to input
  PORTB |= _BV(BUTTON); // enable button's pull-up resistor
  DDRB |= _BV(DDB3) | _BV(DDB4); // set LEDs to output

  button_was_pressed = 0;

  sei();

  GIMSK = _BV(PCIE);  // Enable pin-change interrupts
  PCMSK = _BV(PCINT2);  // Mask off all but the button pin for interrupts

  state = -1;  // Ensure that the power-down flash cycle will happen
  while (1) {
    leds_off();
    power_down();
    start_sleep_cycle();
    while (state != STATE_POWER_DOWN && quick_induction_count < 15) {
      sleep_mode();
    }
  }
  return 0;
}
