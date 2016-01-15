#include <pebble.h>

static Window *s_main_window;
static Layer *s_fg_layer;
static Layer *s_bg_layer;

static uint8_t s_hour;
static uint8_t s_minute;
static bool s_phase;

static void update_bg_display(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer); 
  int thickness = bounds.size.w / 2;

  if (s_phase) {
    graphics_context_set_fill_color(ctx, GColorBlack);      
  } else {
    graphics_context_set_fill_color(ctx, GColorWhite);  
  }
  
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, thickness, 0, TRIG_MAX_ANGLE);
   
}

static void update_fg_display(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer); 
  uint16_t angle_hour = (s_hour % 12) * (TRIG_MAX_ANGLE / 12);
  uint16_t angle_minute = s_minute * (TRIG_MAX_ANGLE / 60);
  int16_t angle_diff = angle_minute - angle_hour;
  int thickness = bounds.size.w / 2;

  if (s_phase) {
    graphics_context_set_fill_color(ctx, GColorWhite);      
  } else {
    graphics_context_set_fill_color(ctx, GColorBlack);  
  }
  
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Time: %i:%i", s_hour, s_minute);
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Angles: %i %i (%i) Phase: %i", angle_hour, angle_minute, angle_diff, s_phase);
  if ( abs(angle_diff) < 50 ) {
    // draw full circle when hands share the same approx position to prevent aliased, non-vertical line
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, thickness, 0, TRIG_MAX_ANGLE);
  } else if ( angle_minute > angle_hour ) {
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, thickness, angle_hour, angle_minute);
  } else {
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, thickness, angle_hour, TRIG_MAX_ANGLE);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, thickness, 0, angle_minute);
  }
}

static void update_time(struct tm *tick_time) {
  s_hour = tick_time->tm_hour;
  s_minute = tick_time->tm_sec;

uint16_t angle_hour = s_hour % 12 * (TRIG_MAX_ANGLE / 12);
  uint16_t angle_minute = s_minute * (TRIG_MAX_ANGLE / 60);
  uint16_t angle_min = TRIG_MAX_ANGLE / 60;
  int16_t angle_diff = angle_minute - angle_hour;
  
  if ( angle_diff <= angle_min && angle_diff > 0  ) {
    s_phase = !s_phase;
  }
  
  layer_mark_dirty(s_fg_layer);
  layer_mark_dirty(s_bg_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void main_window_load(Window *window) {
  // Get window details
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_bg_layer = layer_create(bounds);
  layer_add_child(window_layer, s_bg_layer);
  layer_set_update_proc(s_bg_layer, update_bg_display);
  
  s_fg_layer = layer_create(bounds);
  layer_add_child(window_layer, s_fg_layer);
  layer_set_update_proc(s_fg_layer, update_fg_display);
}

static void main_window_unload(Window *window) {
  // destroy all the things
  layer_destroy(s_fg_layer);
  layer_destroy(s_bg_layer);
}

static void init() {
  // Create main Window element
  s_main_window = window_create();
  
  // Set handlers to manage Window elements
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show Window on watch (animated = true)
  s_phase = 0;
  window_set_background_color(s_main_window, GColorClear);  
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  time_t start_time = time(NULL);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Display initial time
  update_time(localtime(&start_time));
}

static void deinit() {
  window_destroy(s_main_window);
  tick_timer_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}