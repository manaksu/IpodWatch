#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static GBitmap *s_bg_bitmap;
static char s_time_buffer[6];
static int s_battery_percent = 100;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  if (clock_is_24h_style()) {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M", t);
  } else {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%I:%M", t);
  }
  layer_mark_dirty(s_canvas_layer);
}

static void battery_callback(BatteryChargeState state) {
  s_battery_percent = state.charge_percent;
  layer_mark_dirty(s_canvas_layer);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  graphics_draw_bitmap_in_rect(ctx, s_bg_bitmap, GRect(0, 0, 144, 168));

  // TIME — shifted down 10px from previous attempt
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx,
    s_time_buffer,
    fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
    GRect(24, 23, 80, 32),
    GTextOverflowModeWordWrap,
    GTextAlignmentRight,
    NULL);

  // BATTERY OUTLINE — shifted down 7px
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(55, 68, 36, 14));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(91, 72, 3, 6), 0, GCornerNone);
  int fill_w = (s_battery_percent * 32) / 100;
  if (fill_w > 0) {
    graphics_fill_rect(ctx, GRect(56, 69, fill_w, 12), 0, GCornerNone);
  }

  // BATTERY %
  static char batt_buf[8];
  snprintf(batt_buf, sizeof(batt_buf), "%d%%", s_battery_percent);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx,
    batt_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(55, 83, 40, 14),
    GTextOverflowModeWordWrap,
    GTextAlignmentLeft,
    NULL);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  s_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_canvas_layer = layer_create(layer_get_bounds(window_layer));
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  update_time();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  gbitmap_destroy(s_bg_bitmap);
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler)update_time);
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
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
