#include <pebble.h>
#include <stdio.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_days_layer;
static TextLayer *s_bt_layer, *s_date_layer;
static Layer *s_battery_layer;
static int display_since = 1;
static int s_battery_level;
// Persistent storage key
#define SETTINGS_KEY 1

// Define our settings struct
typedef struct ClaySettings {
  // format: 2017-03-13
  char selectedDate[10];
  int hide_battery;
  int hide_date;
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

// ### helper methods

char *subString(char *someString, int start, int len)
{
   char *new = malloc(sizeof(char)*len+1);
   strncpy(new, someString+start, len);
   new[len] = '\0';
   return new;
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Show the date
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a, %d.%m.", tick_time);
  text_layer_set_text(s_date_layer, date_buffer);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  // update days
  struct tm * timesince;
  int year=2015, month=7 ,day=9;
  if (strcmp(settings.selectedDate, "")){
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "update_time: using selected date '%s'", settings.selectedDate);
    year = atoi(subString(settings.selectedDate, 0, 4));
    month = atoi(subString(settings.selectedDate, 5, 2));
    day = atoi(subString(settings.selectedDate, 8, 2));
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "parsed: %i, %i, %i", year, month, day);
  }
  int dayssince, daystil;
  time_t cur_secs = mktime(tick_time);
  /* get current timeinfo and modify it to the user's choice */
  timesince = localtime ( &temp );
  timesince->tm_mon = month - 1;
  timesince->tm_mday = day;
  
  daystil = ((((mktime(timesince) - cur_secs) /60)/60)/24);
  if (daystil < 0){
    timesince->tm_year = timesince->tm_year + 1;
    daystil = ((((mktime(timesince) - cur_secs) /60)/60)/24);
  }
  
  timesince->tm_year = year - 1900;
  dayssince = ((((cur_secs - mktime(timesince)) /60)/60)/24);

  static char countText[100] = "";
  if (display_since && dayssince > 0){
    snprintf(countText, 100, "%d", dayssince);
	  strcat(countText, " days since");
  }else{
    if(daystil == 0){
      snprintf(countText, 100, "today is the day");
    }else{
      snprintf(countText, 100, "%d", daystil);
	    strcat(countText, " days til");
    }
  }
  text_layer_set_text(s_days_layer, countText);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar (total width = 114px)
  int width = 2+(s_battery_level * 13) / 100;

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  // Draw the outline
  graphics_draw_rect(ctx, GRect(0, 0, 15, bounds.size.h));
  graphics_draw_rect(ctx, GRect(14, 2, 2, 4));

  // Draw the bar
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);

}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(text_layer_get_layer(s_bt_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorWhite);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
 //text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_font(s_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_XKCD_42)));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create the TextLayer with specific bounds
  s_days_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(120, 120), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_days_layer, GColorWhite);
  text_layer_set_text_color(s_days_layer, GColorBlack);
  text_layer_set_text(s_days_layer, "xxx days since");
  //text_layer_set_font(s_days_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_font(s_days_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_XKCD_21)));
  text_layer_set_text_alignment(s_days_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_days_layer));

  // Create the TextLayer with specific bounds
  s_bt_layer = text_layer_create(
      GRect(0, 0, 30, 30));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_bt_layer, GColorWhite);
  text_layer_set_text_color(s_bt_layer, GColorRed);
  text_layer_set_text(s_bt_layer, "bt");
  text_layer_set_font(s_bt_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_XKCD_21)));
  text_layer_set_text_alignment(s_bt_layer, GTextAlignmentLeft);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_bt_layer));  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(120, 5, 20, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 0, 144, 15));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text(s_date_layer, "Sept 23");
  
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_days_layer);
  text_layer_destroy(s_bt_layer);
  layer_destroy(s_battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // alter days to display
  display_since = ! display_since;
  
  update_time();
}

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // get date, format: 2017-03-13
  Tuple *selectedDateInput = dict_find(iter, MESSAGE_KEY_SelectedDate);
  if(selectedDateInput) {
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "selectedDate=%s", selectedDateInput->value->cstring);
    
    strcpy(settings.selectedDate, selectedDateInput->value->cstring);
    update_time();
  }
  Tuple *displayBatteryInput = dict_find(iter, MESSAGE_KEY_hide_battery);
  if(displayBatteryInput) {
    settings.hide_battery = displayBatteryInput->value->int8;
    layer_set_hidden(s_battery_layer, settings.hide_battery);
  }
  Tuple *displayDateInput = dict_find(iter, MESSAGE_KEY_hide_date);
  if(displayDateInput) {
    settings.hide_date = displayDateInput->value->int8;
    layer_set_hidden(text_layer_get_layer(s_date_layer), settings.hide_date);
  }
  prv_save_settings();
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void init() {
  //load settings
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Open AppMessage connection
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
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