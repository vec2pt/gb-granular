#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Include
#include "grains.h"

// -----------------------------------------------------------------------------
// Variables / Constants
// -----------------------------------------------------------------------------

#define PERIOD_VALUE_MIN 0
#define PERIOD_VALUE_MAX 0x7FF
uint16_t period_value = 1792;

// Grains
#define GRAIN_FULL_SIZE 144
#define NR_OF_GRAINS 16
uint8_t grain_index = 0;

// Grain size
uint8_t grain_size = GRAIN_FULL_SIZE >> 4;
uint8_t grain_pt = 0;

// Loop
bool loop = false;
// uint8_t loop_size = NR_OF_GRAINS;
uint8_t loop_size = 4;

// Loop performant delay
#define DELAY_MIN 1
#define DELAY_MAX 4
// uint8_t loop_delay = DELAY_MIN;
uint8_t loop_delay = 2;
uint8_t loop_delay_pt = 0;

// -----------------------------------------------------------------------------
// Inputs
// -----------------------------------------------------------------------------

uint8_t previous_keys = 0, keys = 0;

void update_keys(void) {
  previous_keys = keys;
  keys = joypad();
}

uint8_t key_pressed(uint8_t aKey) { return keys & aKey; }
uint8_t key_ticked(uint8_t aKey) {
  return (keys & aKey) && !(previous_keys & aKey);
}
uint8_t key_released(uint8_t aKey) {
  return !(keys & aKey) && (previous_keys & aKey);
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

void play_isr(void) {
  NR30_REG = 0;

  for (uint8_t c = 0; c < 16; c++)
    AUD3WAVE[c] = grains[grain_index * GRAIN_FULL_SIZE + 16 * grain_pt + c];

  NR30_REG = 0x80;
  NR31_REG = 0xFE;
  NR32_REG = 0x20;
  NR33_REG = period_value & 0xFF;
  NR34_REG = 0xC0 | (period_value >> 8);

  // Update grain_pt
  (grain_pt == grain_size - 1) ? grain_pt = 0 : grain_pt++;
}

void next_grain(void) {
  uint8_t max_index = (!loop) ? NR_OF_GRAINS - 1 : loop_size - 1;
  (grain_index >= max_index) ? grain_index = 0 : grain_index++;
}

void previous_grain(void) {
  uint8_t max_index = (!loop) ? NR_OF_GRAINS - 1 : loop_size - 1;
  (grain_index <= 0) ? grain_index = max_index : grain_index--;
}

void increase_grain_size(void) {
  if (grain_size != GRAIN_FULL_SIZE >> 4) {
    grain_size++;
    grain_pt = 0;
    printf("%d", grain_size);
  }
}

void decrease_grain_size(void) {
  if (grain_size != 1) {
    grain_size--;
    grain_pt = 0;
    printf("%d", grain_size);
  }
}

void main(void) {
  NR52_REG = 0x80;
  NR51_REG = 0x44;
  NR50_REG = 0x77;

  __critical {
    TMA_REG = 0xC0, TAC_REG = 0x07;
    add_TIM(play_isr);
    set_interrupts(VBL_IFLAG | TIM_IFLAG);
  }

  while (1) {
    update_keys();

    // Loop
    // TODO: Add controls for loop_delay
    // TODO: Add controls for loop_size
    if (key_ticked(J_START)) {
      loop = !loop;
      if (loop)
        loop_delay_pt = 0;
    }

    if (loop) {
      loop_delay_pt++;
      printf("%d\n", loop_delay_pt);
      if (loop_delay_pt == loop_delay) {
        next_grain();
        loop_delay_pt = 0;
      }
    }

    // Grain size
    if (key_pressed(J_A)) {
      if (key_ticked(J_UP))
        increase_grain_size();
      if (key_ticked(J_DOWN))
        decrease_grain_size();
    }

    // Period value
    if (!key_pressed(J_A)) {
      if (key_pressed(J_UP))
        if (period_value + 1 <= PERIOD_VALUE_MAX)
          period_value++;
      if (key_pressed(J_DOWN))
        if (period_value >= PERIOD_VALUE_MIN + 1)
          period_value--;
    }

    // Sample switches
    if (key_ticked(J_RIGHT))
      next_grain();
    if (key_ticked(J_LEFT))
      previous_grain();
    if (key_pressed(J_SELECT)) {
      if (key_pressed(J_RIGHT))
        next_grain();
      if (key_pressed(J_LEFT))
        previous_grain();
    }

    vsync();
  }
}
