#include <pebble.h>

static Window *s_main_window;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static TextLayer *s_time_layer;
static Layer *s_battery_bar_layer;
static int s_battery_level = 100;

static void battery_bar_update_proc(Layer *layer, GContext *ctx) {
  // Outer rectangle
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(0, 0, 36, 11));
  // Nub on right
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(36, 3, 3, 5), 0, GCornerNone);
  // Fill based on charge level
  int fill_w = (int)(34.0f * s_battery_level / 100.0f);
  if (fill_w > 0) {
    graphics_fill_rect(ctx, GRect(1, 1, fill_w, 9), 0, GCornerNone);
  }
}

static void battery_handler(BatteryChargeState charge) {
  s_battery_level = charge.charge_percent;
  layer_mark_dirty(s_battery_bar_layer);
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char buffer[] = "00:00";
  if (clock_is_24h_style()) {
    strftime(buffer, sizeof(buffer), "%H:%M", tick_time);
  } else {
    strftime(buffer, sizeof(buffer), "%I:%M", tick_time);
  }
  text_layer_set_text(s_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); // 144x168

  // Background
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  bitmap_layer_set_alignment(s_background_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  // Time — right-aligned over iPod screen area (x=72, y=52, w=70, h=36)
  // LECO font matches the tall condensed numerals in the illustration
  s_time_layer = text_layer_create(GRect(72, 52, 70, 36));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Battery bar — over the battery icon in the illustration (x=91, y=89, w=39, h=11)
  s_battery_bar_layer = layer_create(GRect(91, 89, 39, 11));
  layer_set_update_proc(s_battery_bar_layer, battery_bar_update_proc);
  layer_add_child(window_layer, s_battery_bar_layer);

  update_time();
  battery_handler(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_background_bitmap);
  text_layer_destroy(s_time_layer);
  layer_destroy(s_battery_bar_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  update_time();
}

static void deinit() {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
