#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont s_time_font;
static BitmapLayer *s_daytime_layer;
static GBitmap *s_daytime_bitmap;
static BitmapLayer *s_sunset_layer;
static GBitmap *s_sunset_bitmap;
static BitmapLayer *s_nighttime_layer;
static GBitmap *s_nighttime_bitmap;
static GBitmap *s_bitmap = NULL;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence = NULL;

static void timer_handler(void *context) {
  uint32_t next_delay;

  // Advance to the next APNG frame
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

    // Timer for that delay
    app_timer_register(next_delay, timer_handler, NULL);
  } else {
    // Start again
    gbitmap_sequence_restart(s_sequence);
  }
}

static void load_sequence() {
  // Free old data
  if(s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
    s_sequence = NULL;
  }
  if(s_bitmap) {
    gbitmap_destroy(s_bitmap);
    s_bitmap = NULL;
  }

  // Create sequence
  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_FIREWORK);

  // Create GBitmap
  s_bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);

  // Begin animation
  app_timer_register(1, timer_handler, NULL);
}


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  static char s_hour[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  strftime(s_hour, sizeof(s_hour), "%HH", tick_time);
  
  // Change depending on daytime/nighttime
  if (strcmp(s_hour, "06") < 0 || strcmp(s_hour, "20") > 0) {
    // NIGHTIME
    text_layer_set_text_color(s_time_layer, GColorWhite);
    layer_set_hidden(bitmap_layer_get_layer(s_sunset_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_daytime_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_nighttime_layer), false);
  } 
  else if (strcmp(s_hour, "10") < 0 || strcmp(s_hour, "17") > 0){
    // SUNSET
    text_layer_set_text_color(s_time_layer, GColorPastelYellow);  
    layer_set_hidden(bitmap_layer_get_layer(s_sunset_layer), false);
    layer_set_hidden(bitmap_layer_get_layer(s_daytime_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_nighttime_layer), true);
  } else {
    // DAYTIME
    text_layer_set_text_color(s_time_layer, GColorWhite); 
    layer_set_hidden(bitmap_layer_get_layer(s_sunset_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_daytime_layer), false);
    layer_set_hidden(bitmap_layer_get_layer(s_nighttime_layer), true);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
 update_time();
}

static void main_window_load(Window *window) {
  // Get information about the window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Crete the textlayer with specific bounds
  s_time_layer = text_layer_create(
    GRect(0, 110, bounds.size.w, 50));
  
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RAMS_BOLD_42));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorPastelYellow);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    
  // Create nighttime layer (blue)
  s_nighttime_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NIGHTTIME);
  s_nighttime_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_nighttime_layer, s_nighttime_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_nighttime_layer));
  
  // Create sunset layer (orange)
  s_sunset_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BIG_WALLPAPER);
  s_sunset_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_sunset_layer, s_sunset_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_sunset_layer));
  
  // Create daytime layer (green)
  s_daytime_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DAYTIME);
  s_daytime_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_daytime_layer, s_daytime_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_daytime_layer));
  
  // Create fireworks
  //s_bitmap_layer = bitmap_layer_create(bounds);
  //layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  //load_sequence();
  
  // Add time layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  // unload GFont
  fonts_unload_custom_font(s_time_font);
  // Dstroy GBitmap and BitmapLayer
  gbitmap_destroy(s_sunset_bitmap);
  bitmap_layer_destroy(s_sunset_layer);
  gbitmap_destroy(s_nighttime_bitmap);
  bitmap_layer_destroy(s_nighttime_layer);
  gbitmap_destroy(s_daytime_bitmap);
  bitmap_layer_destroy(s_daytime_layer);
  // Destroy fireworks
  bitmap_layer_destroy(s_bitmap_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
    
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is dusplayed from the start
  update_time();
  
  // Set background color
  window_set_background_color(s_main_window, GColorBlack);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}