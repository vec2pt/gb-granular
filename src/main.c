#include <gb/gb.h>
#include <gbdk/console.h>
#include <gbdk/font.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Include
#include "grains.h"

// Include assets
#include "assets/logo.h"
#include "assets/tiles.h"

// -----------------------------------------------------------------------------
// Variables / Constants
// -----------------------------------------------------------------------------

#define VERSION "v0.9.0"
#define DISPLAY_LOGO false

// Play / Stop
bool play = false;

// Corsor
enum { CURSOR_RIGHT, CURSOR_LEFT, CURSOR_UP, CURSOR_DOWN };
uint8_t cursor_position = 0;
uint8_t cursor_position_previous = 1;

// https://gbdev.io/pandocs/Audio.html#frequency
#define PERIOD_VALUE_MIN 0
#define PERIOD_VALUE_MAX 0x7FF
uint16_t period_value = 1792;

// Grains
#define GRAIN_FULL_SIZE 144
#define NR_OF_GRAINS 16
uint8_t grain_index = 0;
uint8_t grain_index_previous = NR_OF_GRAINS - 1;

// Grain size to be played back (option to trim the grain)
uint8_t grain_size = GRAIN_FULL_SIZE >> 4;
uint8_t grain_size_part = 0;

// Loop
bool loop = false;

// Loop performant delay
#define DELAY_MIN 1
#define DELAY_MAX 4
uint8_t loop_delay = DELAY_MIN;
uint8_t loop_delay_counter = 0;

// Helper variable for printing hex values
const char hex[] = "0123456789ABCDEF";

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
// UI
// -----------------------------------------------------------------------------

// Cell states
enum {
  UI_CELL_ENABLED = 0x60,
  UI_CELL_ACTIVE = 0x64,
  UI_CELL_DISABLED = 0x68,
  UI_CELL_DISABLED_ACTIVE = 0x6C,
};

// Cells location coordinates
// clang-format off
const uint8_t ui_cells_location[NR_OF_GRAINS][2] = {
    {3, 1},  {7, 1},  {11, 1},  {15, 1},
    {3, 4},  {7, 4},  {11, 4},  {15, 4},
    {3, 7},  {7, 7},  {11, 7},  {15, 7},
    {3, 10}, {7, 10}, {11, 10}, {15, 10},
};
// clang-format on

// Enable all cells at launch
bool disabled_cells[NR_OF_GRAINS] = {false};

void ui_draw_cell(const uint8_t i) {
  uint8_t grid_x, grid_y;
  uint8_t cell;
  if (i == grain_index && !disabled_cells[i])
    cell = UI_CELL_ACTIVE;
  else if (i == grain_index && disabled_cells[i])
    cell = UI_CELL_DISABLED_ACTIVE;
  else if (disabled_cells[i])
    cell = UI_CELL_DISABLED;
  else
    cell = UI_CELL_ENABLED;

  grid_x = ui_cells_location[i][0];
  grid_y = ui_cells_location[i][1];

  set_tile_xy(grid_x, grid_y, cell);
  set_tile_xy(grid_x + 1, grid_y, cell + 1);
  set_tile_xy(grid_x, grid_y + 1, cell + 2);
  set_tile_xy(grid_x + 1, grid_y + 1, cell + 3);
}

void ui_draw_cursor(void) {
  uint8_t grid_x, grid_y;
  // Remove old cursor
  grid_x = ui_cells_location[cursor_position_previous][0] - 1;
  grid_y = ui_cells_location[cursor_position_previous][1] - 1;
  set_tile_xy(grid_x, grid_y, 0x00);
  set_tile_xy(grid_x + 1, grid_y, 0x00);
  set_tile_xy(grid_x, grid_y + 1, 0x00);
  set_tile_xy(grid_x + 3, grid_y + 2, 0x00);
  set_tile_xy(grid_x + 2, grid_y + 3, 0x00);
  set_tile_xy(grid_x + 3, grid_y + 3, 0x00);
  ui_draw_cell(cursor_position_previous);

  // Drawing a new cursor
  grid_x = ui_cells_location[cursor_position][0] - 1;
  grid_y = ui_cells_location[cursor_position][1] - 1;
  set_tile_xy(grid_x, grid_y, 0x73);
  set_tile_xy(grid_x + 1, grid_y, 0x74);
  set_tile_xy(grid_x, grid_y + 1, 0x75);
  set_tile_xy(grid_x + 3, grid_y + 2, 0x76);
  set_tile_xy(grid_x + 2, grid_y + 3, 0x77);
  set_tile_xy(grid_x + 3, grid_y + 3, 0x78);
  ui_draw_cell(cursor_position);
}

void ui_update(void) {
  ui_draw_cell(grain_index);
  ui_draw_cell(grain_index_previous);
  ui_draw_cursor();

  (play) ? set_tile_xy(17, 1, 0x71) : set_tile_xy(17, 1, 0x70);
  (loop) ? set_tile_xy(17, 2, 0x72) : set_tile_xy(17, 2, 0x0);

  // WIP
  // TODO: Rebuild it
  gotoxy(0, 13);
  printf("grain: %d ", grain_index);
  gotoxy(0, 14);
  printf("grain_size: %d", grain_size);
  gotoxy(0, 15);
  printf("pv:%d", period_value);
  gotoxy(0, 16);
  printf("ld:%d", loop_delay);
  gotoxy(5, 16);
  printf("ld_c:%d", loop_delay_counter);
}

void ui_draw(void) {
  uint8_t grid_x, grid_y;
  for (uint8_t i = 0; i < 16; i++) {
    ui_draw_cell(i);

    grid_x = ui_cells_location[i][0] - 1;
    grid_y = ui_cells_location[i][1] + 1;
    gotoxy(grid_x, grid_y);
    printf("%c", hex[0x000Fu & (i)]);
  }

  // gotoxy(0, 17);
  gotoxy(2, 17);
  printf(VERSION);

  ui_update();
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

void play_isr(void) {
  if (!play)
    return;

  NR30_REG = 0;

  for (uint8_t c = 0; c < 16; c++)
    AUD3WAVE[c] =
        grains[grain_index * GRAIN_FULL_SIZE + 16 * grain_size_part + c];

  NR30_REG = 0x80;
  NR31_REG = 0xFE;
  NR32_REG = 0x20;
  NR33_REG = period_value & 0xFF;
  NR34_REG = 0xC0 | (period_value >> 8);

  // Update grain_size_part
  (grain_size_part == grain_size - 1) ? grain_size_part = 0 : grain_size_part++;
}

void move_cursor(uint8_t cursor_direction) {
  cursor_position_previous = cursor_position;
  switch (cursor_direction) {
  case CURSOR_RIGHT:
    cursor_position =
        (cursor_position - cursor_position % 4) + (cursor_position + 1) % 4;
    break;
  case CURSOR_LEFT:
    if (cursor_position == 0)
      cursor_position = 3;
    else
      cursor_position =
          (cursor_position - cursor_position % 4) + (cursor_position - 1) % 4;
    break;
  case CURSOR_UP:
    cursor_position -= 4;
    cursor_position %= NR_OF_GRAINS;
    break;
  case CURSOR_DOWN:
    cursor_position += 4;
    cursor_position %= NR_OF_GRAINS;
    break;
  }
}

void play_next_grain(void) {
  grain_index_previous = grain_index;

  uint8_t grain_index_candidate = grain_index + 1;
  while (disabled_cells[grain_index_candidate] &&
         grain_index_candidate != grain_index) {
    grain_index_candidate++;
    grain_index_candidate %= NR_OF_GRAINS;
  }
  grain_index = grain_index_candidate;
}

void increase_grain_size(void) {
  if (grain_size != GRAIN_FULL_SIZE >> 4) {
    grain_size++;
    grain_size_part = 0;
  }
}

void decrease_grain_size(void) {
  if (grain_size != 1) {
    grain_size--;
    grain_size_part = 0;
  }
}

void granular_reset() {
  grain_size_part = 0;
  loop = false;
  // grain_index = 0; // Or not to reset?
  loop_delay_counter = 0;
}

void display_logo(void) {
  gotoxy(6, 4);
  printf("Granular");

  // Logo
  set_bkg_tiles(5, 5, logo_WIDTH / 8, logo_HEIGHT / 8, logo_map);

  gotoxy(5, 13);
  printf("by  vec2pt");

  vsync();
  delay(2200);
  cls();
}

void setup(void) {
  NR52_REG = 0x80;
  NR51_REG = 0x44;
  NR50_REG = 0x77;

  __critical {
    TMA_REG = 0xC0, TAC_REG = 0x07;
    add_TIM(play_isr);
    set_interrupts(VBL_IFLAG | TIM_IFLAG);
  }

  // Font
  font_init();
  font_load(font_ibm);

  // Tiles
  set_bkg_data(0x60, tiles_TILE_COUNT, tiles_tiles);
#if DISPLAY_LOGO
  set_bkg_data(0x70, logo_TILE_COUNT, logo_tiles);
  show_logo();
#endif
}

void main(void) {
  setup();
  ui_draw();

  while (1) {
    update_keys();
    ui_update();

    // Play / Pause
    if (key_ticked(J_START)) {
      play = !play;
      if (play) {
        grain_index_previous = grain_index;
        grain_index = cursor_position;
      }
      if (play && key_pressed(J_SELECT))
        loop = true;
      if (!play)
        granular_reset();
    }

    // TODO: Add controls for loop_delay
    if (loop) {
      loop_delay_counter++;
      if (loop_delay_counter == loop_delay) {
        play_next_grain();
        loop_delay_counter = 0;
      }
    }

    // Cursor movements
    if (key_ticked(J_RIGHT))
      move_cursor(CURSOR_RIGHT);
    else if (key_ticked(J_LEFT))
      move_cursor(CURSOR_LEFT);
    else if (key_ticked(J_UP))
      move_cursor(CURSOR_UP);
    else if (key_ticked(J_DOWN))
      move_cursor(CURSOR_DOWN);

    // Enable / Disable cell
    if (key_ticked(J_A))
      disabled_cells[cursor_position] = !disabled_cells[cursor_position];

    // TODO
    // if (key_pressed(J_SELECT)) {
    //   if (key_pressed(J_RIGHT))
    //     next_grain();
    //   if (key_pressed(J_LEFT))
    //     previous_grain();
    // }

    // Grain size
    // if (key_pressed(J_A)) {
    //   if (key_ticked(J_UP))
    //     increase_grain_size();
    //   if (key_ticked(J_DOWN))
    //     decrease_grain_size();
    // }

    // Period value
    // if (!key_pressed(J_A)) {
    //   if (key_pressed(J_UP))
    //     if (period_value + 1 <= PERIOD_VALUE_MAX)
    //       period_value++;
    //   if (key_pressed(J_DOWN))
    //     if (period_value >= PERIOD_VALUE_MIN + 1)
    //       period_value--;
    // }

    vsync();
  }
}
