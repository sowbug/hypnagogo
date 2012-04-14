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

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <inttypes.h>
#include <util/delay.h>

// START MCU-SPECIFIC -------------------------------

// For ATtiny13:
//
// Button between pin 7 and ground
// Left LED on pin 2
// Right LED on pin 3
//
#define BUTTON _BV(PB2)
#define BUTTON_DD _BV(DDB2)
#define LEDS (LED_LEFT | LED_RIGHT)
#define LED_LEFT _BV(PB3)
#define LED_LEFT_DD _BV(DDB3)
#define LED_RIGHT _BV(PB4)
#define LED_RIGHT_DD _BV(DDB4)

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
  PCMSK = _BV(PCINT2);  // Mask off all but the button pin for interrupts
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

// END MCU-SPECIFIC -------------------------------

/* ramping fx increments */
#define PWM_VAL 4
#define TRANS_VAL 6

/* the overall pulse width and delay between pulses */
#define MACRO_WIDTH 1500
#define MACRO_GAP 250

// We use a sort of fixed-point decimal to let us compute durations with
// reasonable precision without bringing in FPU code that wouldn't fit in
// a small flash program space.
//
// "+ 8" = 8-bit timer overflow = 8 bits = 256 cycles = Hz / 256
// "- 8" = x256 for precision
// For slow: "10" = CS02 | CS00 = clkIO / 1024 Hz
// For fast: "0"  = CS00 = clkIO, no prescaling
//
#define SLOW_HZ_X_256 (F_CPU >> (10 + 8 - 8))  // 1171
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

enum {
  STATE_POWER_DOWN,
  STATE_INIT,
  STATE_IDLE,
  STATE_DREAM
};

static uint16_t interrupt_count;
static uint16_t interrupts_left;
static uint8_t current_led;
static uint8_t flash_count;
static volatile uint8_t button_was_pressed;
static volatile uint8_t quick_induction_count;
static volatile uint8_t state;

static void reset_idle_interrupts_left() {
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

  // Hack: reset the button-pressed indicator. On the breadboard version
  // of this circuit, I was going straight into nap mode after powering on
  // with the button. I debounced and checked states, but nothing worked. I
  // see nothing like this on the real hardware. I'm wondering whether the
  // breadboard is noisy, or perhaps the ISP was sending ambiguous signals.
  //////////

  ///////////////////////was_button_pressed();
}

static void reset_dream_interrupts_left() {
  interrupts_left = MACRO_WIDTH + MACRO_GAP;
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

  for (int i = 0; i < (MSEC_TO_WAIT / WAIT_QUANTUM_MSEC); ++i) {
    if (!is_button_pressed()) {
      return 0;
    }
    _delay_ms(WAIT_QUANTUM_MSEC);
  }
  wait_for_button_up();
  return 1;
}

static void handle_button_INIT() {
}

static void handle_button_IDLE() {
  start_nap_mode();
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

  start_POWER_DOWN();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();

  // If we reach this point, the user pressed the button.
  enable_timer_prr();
}

static void start_sleep_cycle() {
  enable_timer_interrupt();

  set_slow_timer();

  interrupt_count = START_INTERRUPT_COUNT;
  quick_induction_count = 0;
  start_INIT();

  set_sleep_mode(SLEEP_MODE_IDLE);
}

int main(void) {
  set_port_state();
  button_was_pressed = 0;
  sei();
  enable_pin_interrupts();
  state = -1;  // Ensure that the power-down flash cycle will happen

  while (1) {
    leds_off();
    power_down();
    start_sleep_cycle();
    while (state != STATE_POWER_DOWN && quick_induction_count < 8) {
      sleep_mode();
    }
  }
  return 0;
}
