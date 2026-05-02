#include <pebble.h>
#include <string.h>

#define STORAGE_KEY_TIME_COLOR      0
#define STORAGE_KEY_DENSITY         1
#define STORAGE_KEY_CELL_SIZE       2
#define STORAGE_KEY_TOUCH_INTENSITY 3
#define STORAGE_KEY_TOUCH_DURATION  4
#define STORAGE_KEY_AMBIENT_TIMEOUT 5
#define STORAGE_KEY_ASCII_COLOR     6
#define STORAGE_KEY_FACE_BG_COLOR   7

#define DEFAULT_TIME_COLOR    0xFFFFFF
#define DEFAULT_ASCII_COLOR   0x555555
#define DEFAULT_FACE_BG_COLOR 0x000000

#define MAX_COLS 32
#define MAX_ROWS 32
#define FRAME_MS 100
#define FRAMES_PER_SEC (1000 / FRAME_MS)

#define MAX_ATTRACTORS 12

#define DIGIT_W 3
#define DIGIT_H 5
#define DIGIT_GAP_X 1
#define DIGIT_GAP_Y 1
#define BLOCK_W (DIGIT_W * 2 + DIGIT_GAP_X)
#define BLOCK_H (DIGIT_H * 2 + DIGIT_GAP_Y)

static const char *DENSITY_RAMPS[] = {
  " .-+*#@",
  " .,oO@",
  " /\\|+*#",
  " :;io08",
  " <>=#%@",
};
#define DENSITY_RAMP_COUNT (sizeof(DENSITY_RAMPS) / sizeof(DENSITY_RAMPS[0]))

typedef struct {
  int cell_w;
  int cell_h;
  const char *font_key;
} CellSize;

static const CellSize CELL_SIZES[] = {
  {  8, 10, FONT_KEY_GOTHIC_09 },
  { 10, 12, FONT_KEY_GOTHIC_14 },
  { 14, 16, FONT_KEY_GOTHIC_18 },
};
#define CELL_SIZE_COUNT (sizeof(CELL_SIZES) / sizeof(CELL_SIZES[0]))

static const int TOUCH_INTENSITY_MAX[] = { 400, 700, 1000 };
#define TOUCH_INTENSITY_COUNT 3

static const int TOUCH_DURATION_SECS[] = { 1, 3, 6, 10 };
#define TOUCH_DURATION_COUNT 4

static const int AMBIENT_TIMEOUT_SECS[] = { 0, 30, 60, 300, 600 };
#define AMBIENT_TIMEOUT_COUNT 5

#define ATTRACTOR_RADIUS 64
#define ATTRACTOR_RADIUS_SQ (ATTRACTOR_RADIUS * ATTRACTOR_RADIUS)
#define ATTRACTOR_INITIAL 1000

static const uint8_t DIGIT_FONT[10][DIGIT_H] = {
  {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
  {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
  {0b110, 0b001, 0b010, 0b100, 0b111}, // 2
  {0b111, 0b001, 0b011, 0b001, 0b111}, // 3
  {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
  {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
  {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
  {0b111, 0b001, 0b010, 0b010, 0b010}, // 7
  {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
};

typedef struct {
  int16_t x;
  int16_t y;
  uint16_t strength;
} Attractor;

typedef struct {
  int time_color_rgb;
  int ascii_color_rgb;
  int face_bg_color_rgb;
  int density_index;
  int cell_size_index;
  int touch_intensity_index;
  int touch_duration_index;
  int ambient_timeout_index;
} Settings;

static Window *s_window;
static Layer *s_field_layer;
static AppTimer *s_anim_timer;
static uint32_t s_tick;
static uint32_t s_last_input_tick;
static bool s_animating;

static int8_t s_sin_table[256];
static Attractor s_attractors[MAX_ATTRACTORS];

static int8_t s_digits[4];
static GColor s_time_color;
static GColor s_ascii_color;
static GColor s_face_bg_color;
static Settings s_settings;

#if PBL_TOUCH
static int16_t s_last_touch_x;
static int16_t s_last_touch_y;
#endif

static GColor gcolor_from_rgb(int rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  return GColorFromRGB(r, g, b);
}

static int clamp_int(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static int read_color_or_default(uint32_t key, int fallback) {
  if (!persist_exists(key)) return fallback;
  return persist_read_int(key) & 0xFFFFFF;
}

static void load_settings(void) {
  s_settings.time_color_rgb = read_color_or_default(STORAGE_KEY_TIME_COLOR, DEFAULT_TIME_COLOR);
  s_settings.ascii_color_rgb = read_color_or_default(STORAGE_KEY_ASCII_COLOR, DEFAULT_ASCII_COLOR);
  s_settings.face_bg_color_rgb = read_color_or_default(STORAGE_KEY_FACE_BG_COLOR, DEFAULT_FACE_BG_COLOR);
  s_settings.density_index = clamp_int(persist_read_int(STORAGE_KEY_DENSITY), 0, DENSITY_RAMP_COUNT - 1);
  s_settings.cell_size_index = clamp_int(persist_read_int(STORAGE_KEY_CELL_SIZE), 0, CELL_SIZE_COUNT - 1);
  s_settings.touch_intensity_index = clamp_int(persist_read_int(STORAGE_KEY_TOUCH_INTENSITY), 0, TOUCH_INTENSITY_COUNT - 1);
  s_settings.touch_duration_index = clamp_int(persist_read_int(STORAGE_KEY_TOUCH_DURATION), 0, TOUCH_DURATION_COUNT - 1);
  s_settings.ambient_timeout_index = clamp_int(persist_read_int(STORAGE_KEY_AMBIENT_TIMEOUT), 0, AMBIENT_TIMEOUT_COUNT - 1);

  if (!persist_exists(STORAGE_KEY_CELL_SIZE)) s_settings.cell_size_index = 1;
  if (!persist_exists(STORAGE_KEY_TOUCH_INTENSITY)) s_settings.touch_intensity_index = 1;
  if (!persist_exists(STORAGE_KEY_TOUCH_DURATION)) s_settings.touch_duration_index = 2;
  if (!persist_exists(STORAGE_KEY_AMBIENT_TIMEOUT)) s_settings.ambient_timeout_index = 0;

  s_time_color = gcolor_from_rgb(s_settings.time_color_rgb);
  s_ascii_color = gcolor_from_rgb(s_settings.ascii_color_rgb);
  s_face_bg_color = gcolor_from_rgb(s_settings.face_bg_color_rgb);
}

static void build_sin_table(void) {
  for (int i = 0; i < 256; i++) {
    s_sin_table[i] = (int8_t)(sin_lookup(i * (TRIG_MAX_ANGLE / 256)) >> 8);
  }
}

static inline int8_t fast_sin(uint8_t a) { return s_sin_table[a]; }
static inline int8_t fast_cos(uint8_t a) { return s_sin_table[(uint8_t)(a + 64)]; }

static int field_brightness(int col, int row, uint32_t t) {
  uint8_t a = (uint8_t)(col * 14 + (t * 3));
  uint8_t b = (uint8_t)(row * 12 - (t * 2));
  uint8_t c = (uint8_t)((col + row) * 9 + (t * 4));
  uint8_t d = (uint8_t)((col - row) * 11 + (t * 1));

  int v = fast_sin(a) + fast_cos(b) + fast_sin(c) + fast_cos(d);
  v = v + 512;
  if (v < 0) v = 0;
  if (v > 1023) v = 1023;
  return v;
}

static int attractor_bump(int px, int py) {
  int total = 0;
  int max_bump = TOUCH_INTENSITY_MAX[s_settings.touch_intensity_index];
  for (int i = 0; i < MAX_ATTRACTORS; i++) {
    uint16_t s = s_attractors[i].strength;
    if (s == 0) continue;
    int dx = px - s_attractors[i].x;
    int dy = py - s_attractors[i].y;
    int d2 = dx * dx + dy * dy;
    if (d2 >= ATTRACTOR_RADIUS_SQ) continue;
    int falloff = ((ATTRACTOR_RADIUS_SQ - d2) * (int)s) / ATTRACTOR_RADIUS_SQ;
    int bump = (falloff * max_bump) / ATTRACTOR_INITIAL;
    total += bump;
  }
  return total;
}

static int attractor_decay_amount(void) {
  int lifetime = TOUCH_DURATION_SECS[s_settings.touch_duration_index];
  int frames = lifetime * FRAMES_PER_SEC;
  if (frames <= 0) frames = 1;
  int decay = ATTRACTOR_INITIAL / frames;
  if (decay < 1) decay = 1;
  return decay;
}

static void decay_attractors(void) {
  int amount = attractor_decay_amount();
  for (int i = 0; i < MAX_ATTRACTORS; i++) {
    if (s_attractors[i].strength > amount) {
      s_attractors[i].strength -= amount;
    } else {
      s_attractors[i].strength = 0;
    }
  }
}

static bool any_attractors_active(void) {
  for (int i = 0; i < MAX_ATTRACTORS; i++) {
    if (s_attractors[i].strength > 0) return true;
  }
  return false;
}

static void anim_timer_cb(void *data);

static void start_animation(void) {
  if (s_animating) return;
  s_animating = true;
  s_anim_timer = app_timer_register(FRAME_MS, anim_timer_cb, NULL);
}

static void register_input(void) {
  s_last_input_tick = s_tick;
  start_animation();
}

static void spawn_attractor(int16_t x, int16_t y) {
  int slot = 0;
  uint16_t lowest = s_attractors[0].strength;
  for (int i = 1; i < MAX_ATTRACTORS; i++) {
    if (s_attractors[i].strength < lowest) {
      lowest = s_attractors[i].strength;
      slot = i;
    }
  }
  s_attractors[slot].x = x;
  s_attractors[slot].y = y;
  s_attractors[slot].strength = ATTRACTOR_INITIAL;
  register_input();
}

#if PBL_TOUCH
static void touch_handler(const TouchEvent *event, void *context) {
  if (event->type != TouchEvent_Touchdown &&
      event->type != TouchEvent_PositionUpdate) return;

  if (event->type == TouchEvent_PositionUpdate) {
    int dx = event->x - s_last_touch_x;
    int dy = event->y - s_last_touch_y;
    if (dx * dx + dy * dy < 16) return;
  }
  s_last_touch_x = event->x;
  s_last_touch_y = event->y;
  spawn_attractor(event->x, event->y);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "touch %d,%d t=%d", event->x, event->y, (int)event->type);
}
#endif

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  GRect b = layer_get_bounds(s_field_layer);
  int16_t x = b.size.w / 2 + (axis == ACCEL_AXIS_X ? direction * 30 : 0);
  int16_t y = b.size.h / 2 + (axis == ACCEL_AXIS_Y ? direction * 30 : 0);
  spawn_attractor(x, y);
}

static int digit_cell_lit(int col, int row, int cols, int rows) {
  int ox = (cols - BLOCK_W) / 2;
  int oy = (rows - BLOCK_H) / 2;

  int cx = col - ox;
  int cy = row - oy;
  if (cx < 0 || cx >= BLOCK_W || cy < 0 || cy >= BLOCK_H) return 0;

  int line, dy;
  if (cy < DIGIT_H) {
    line = 0; dy = cy;
  } else if (cy >= DIGIT_H + DIGIT_GAP_Y) {
    line = 1; dy = cy - DIGIT_H - DIGIT_GAP_Y;
  } else {
    return 0;
  }

  int side, dx;
  if (cx < DIGIT_W) {
    side = 0; dx = cx;
  } else if (cx >= DIGIT_W + DIGIT_GAP_X) {
    side = 1; dx = cx - DIGIT_W - DIGIT_GAP_X;
  } else {
    return 0;
  }

  int digit = s_digits[line * 2 + side];
  if (digit < 0 || digit > 9) return 0;

  uint8_t row_bits = DIGIT_FONT[digit][dy];
  return (row_bits >> (DIGIT_W - 1 - dx)) & 1;
}

static void field_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const CellSize *cs = &CELL_SIZES[s_settings.cell_size_index];
  int cell_w = cs->cell_w;
  int cell_h = cs->cell_h;
  int cols = bounds.size.w / cell_w;
  int rows = bounds.size.h / cell_h;
  if (cols > MAX_COLS) cols = MAX_COLS;
  if (rows > MAX_ROWS) rows = MAX_ROWS;

  GFont font = fonts_get_system_font(cs->font_key);
  const char *ramp = DENSITY_RAMPS[s_settings.density_index];
  int ramp_len = (int)strlen(ramp);
  if (ramp_len < 2) ramp_len = 2;

  graphics_context_set_fill_color(ctx, s_face_bg_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  for (int row = 0; row < rows; row++) {
    int py = row * cell_h + cell_h / 2;
    for (int col = 0; col < cols; col++) {
      int px = col * cell_w + cell_w / 2;
      int lit = digit_cell_lit(col, row, cols, rows);

      char glyph_char;
      GColor color;

      if (lit) {
        glyph_char = ramp[ramp_len - 1];
        color = s_time_color;
      } else {
        int v = field_brightness(col, row, s_tick) + attractor_bump(px, py);
        if (v > 1023) v = 1023;
        int idx = (v * ramp_len) / 1024;
        if (idx <= 0) continue;
        if (idx >= ramp_len) idx = ramp_len - 1;
        glyph_char = ramp[idx];
        color = s_ascii_color;
      }

      char glyph[2] = { glyph_char, 0 };
      GRect cell = GRect(col * cell_w, row * cell_h - 2, cell_w + 2, cell_h + 4);
      graphics_context_set_text_color(ctx, color);
      graphics_draw_text(ctx, glyph, font, cell,
                         GTextOverflowModeWordWrap,
                         GTextAlignmentCenter, NULL);
    }
  }
}

static void anim_timer_cb(void *data) {
  s_tick++;
  decay_attractors();
  layer_mark_dirty(s_field_layer);

  int timeout = AMBIENT_TIMEOUT_SECS[s_settings.ambient_timeout_index];
  bool should_sleep = false;
  if (timeout > 0 && !any_attractors_active()) {
    uint32_t timeout_ticks = (uint32_t)(timeout * FRAMES_PER_SEC);
    if (s_tick - s_last_input_tick > timeout_ticks) {
      should_sleep = true;
    }
  }

  if (should_sleep) {
    s_animating = false;
    s_anim_timer = NULL;
  } else {
    s_anim_timer = app_timer_register(FRAME_MS, anim_timer_cb, NULL);
  }
}

static void update_time_digits(struct tm *t) {
  int hour = clock_is_24h_style() ? t->tm_hour : (t->tm_hour % 12);
  if (!clock_is_24h_style() && hour == 0) hour = 12;
  s_digits[0] = hour / 10;
  s_digits[1] = hour % 10;
  s_digits[2] = t->tm_min / 10;
  s_digits[3] = t->tm_min % 10;
  if (s_field_layer) layer_mark_dirty(s_field_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time_digits(tick_time);
}

static void apply_setting_int(uint32_t storage_key, int *target, int value, int max) {
  int v = clamp_int(value, 0, max - 1);
  *target = v;
  persist_write_int(storage_key, v);
}

static void apply_setting_color(uint32_t storage_key, int *target, GColor *cached, int value) {
  int rgb = value & 0xFFFFFF;
  *target = rgb;
  *cached = gcolor_from_rgb(rgb);
  persist_write_int(storage_key, rgb);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t;
  bool changed = false;

  if ((t = dict_find(iter, MESSAGE_KEY_TIME_COLOR))) {
    apply_setting_color(STORAGE_KEY_TIME_COLOR, &s_settings.time_color_rgb, &s_time_color, (int)t->value->int32);
    changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_ASCII_COLOR))) {
    apply_setting_color(STORAGE_KEY_ASCII_COLOR, &s_settings.ascii_color_rgb, &s_ascii_color, (int)t->value->int32);
    changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_FACE_BG_COLOR))) {
    apply_setting_color(STORAGE_KEY_FACE_BG_COLOR, &s_settings.face_bg_color_rgb, &s_face_bg_color, (int)t->value->int32);
    changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DENSITY_INDEX))) {
    apply_setting_int(STORAGE_KEY_DENSITY, &s_settings.density_index, (int)t->value->int32, DENSITY_RAMP_COUNT);
    changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_CELL_SIZE_INDEX))) {
    apply_setting_int(STORAGE_KEY_CELL_SIZE, &s_settings.cell_size_index, (int)t->value->int32, CELL_SIZE_COUNT);
    changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_TOUCH_INTENSITY))) {
    apply_setting_int(STORAGE_KEY_TOUCH_INTENSITY, &s_settings.touch_intensity_index, (int)t->value->int32, TOUCH_INTENSITY_COUNT);
    changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_TOUCH_DURATION))) {
    apply_setting_int(STORAGE_KEY_TOUCH_DURATION, &s_settings.touch_duration_index, (int)t->value->int32, TOUCH_DURATION_COUNT);
    changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_AMBIENT_TIMEOUT))) {
    apply_setting_int(STORAGE_KEY_AMBIENT_TIMEOUT, &s_settings.ambient_timeout_index, (int)t->value->int32, AMBIENT_TIMEOUT_COUNT);
    changed = true;
  }

  if (changed) {
    register_input();
    if (s_field_layer) layer_mark_dirty(s_field_layer);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_field_layer = layer_create(bounds);
  layer_set_update_proc(s_field_layer, field_update_proc);
  layer_add_child(root, s_field_layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time_digits(t);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

#if PBL_TOUCH
  touch_service_subscribe(touch_handler, NULL);
#endif

  accel_tap_service_subscribe(accel_tap_handler);

  s_last_input_tick = 0;
  s_animating = false;
  start_animation();
}

static void window_unload(Window *window) {
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }
  s_animating = false;
#if PBL_TOUCH
  touch_service_unsubscribe();
#endif
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  layer_destroy(s_field_layer);
}

static void init(void) {
  load_settings();
  build_sin_table();

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(256, 64);

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
