/*
 
 Text Watch 2
 A 'better' version of the default 'Text Watch' watchface by Pebble Tech.
 Tap/shake the watch for date and weather info.
 Added ‘o’ prefix for single-digit minutes, so “two eight” appears as “two o’ eight”.
 
 https://github.com/keelanc/text_watch_2
 
 Copyright (C) 2013 Keelan Chu For
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 You may contact the author at kkchufor@gmail.com
 
 */

#include <pebble.h>
#include "time_as_words.h"
#include <ctype.h> // for tolower()

static Window *window;
static TextLayer *hour_layer;
static TextLayer *tens_layer;
static TextLayer *ones_layer;
static char current_hour[] = "eleven";
static char current_tens[] = "thirteen";
static char previous_tens[] = "thirteen";
static char current_ones[] = "clock";
static char current_hour_dummy[] = "eleven";
static char current_tens_dummy[] = "thirteen";
static char current_ones_dummy[] = "clock";
static char day_str[] = "xxx";
static char mon_str[] = "xxx";
static char date_str[] = "xx";
static char weather_str[] = "eleven";
static char center_str[] = "thirteen";
static char temp_str[] = "clock";
static char temp_f_str[] = "clock";
static char temp_c_str[] = "clock";
static char city_str[] = "1234";
#define STAGGER_STR 100
#define STAGGER_IN 500
#define WAIT_TIME 1500
#define ConstantGRect(x, y, w, h) {{(x), (y)}, {(w), (h)}}  // borrowed from drop_zone
// hour, tens, ones frames off screen to the right, center (default), left
static GRect r_rect_hour = ConstantGRect(168, 20, 144, 168 - 20);
static GRect r_rect_tens = ConstantGRect(168, 55, 144, 168 - 55);
static GRect r_rect_ones = ConstantGRect(168, 90, 144, 168 - 90);
static GRect c_rect_hour = ConstantGRect(0, 20, 144, 168 - 20);
static GRect c_rect_tens = ConstantGRect(0, 55, 144, 168 - 55);
static GRect c_rect_ones = ConstantGRect(0, 90, 144, 168 - 90);
static GRect l_rect_hour = ConstantGRect(-168, 20, 144, 168 - 20);
static GRect l_rect_tens = ConstantGRect(-168, 55, 144, 168 - 55);
static GRect l_rect_ones = ConstantGRect(-168, 90, 144, 168 - 90);
static PropertyAnimation *hour_anim = NULL;
static PropertyAnimation *tens_anim = NULL;
static PropertyAnimation *ones_anim = NULL;
static bool introComplete = false;
static bool noInterrupts = false;
static bool timerCallback2 = false;
static AppTimer *timer_handle;
//Reserve global memory to store an AppSync struct and your Dictionary.
static AppSync sync;
static uint8_t sync_buffer[64];
enum WeatherKey {
    WEATHER_ICON_KEY = 0x0,         // TUPLE_CSTRING
    WEATHER_TEMPERATURE_F_KEY = 0x1,// TUPLE_CSTRING
    WEATHER_TEMPERATURE_C_KEY = 0x2,// TUPLE_CSTRING
    WEATHER_CITY_KEY = 0x3,         // TUPLE_CSTRING
    TEMP_FORMAT_KEY = 0x4,          // TUPLE_INT
};
static int config = 2;
#define CONFIG_TEMP_F 2

static void temp_format(void) {
    if(config & CONFIG_TEMP_F) {    // if set to Fahrenheit
        strcpy(temp_str, temp_f_str);
    }
    else {
        strcpy(temp_str, temp_c_str);
    }
}

static void more_info(struct tm *tick_time) {
    strftime(day_str, sizeof(day_str), "%a", tick_time);
    strftime(mon_str, sizeof(mon_str), "%b", tick_time);
    strftime(date_str, sizeof(date_str), "%d", tick_time);
}

static void destroy_property_animation(PropertyAnimation **prop_animation) {
    if (*prop_animation == NULL) {
        return;
    }
    
    if (animation_is_scheduled((Animation*) *prop_animation)) {
        animation_unschedule((Animation*) *prop_animation);
    }
    
    property_animation_destroy(*prop_animation);
    *prop_animation = NULL;
}

static void animation_stopped(Animation *animation, bool finished, void *data) {
//    snprintf(test_str, sizeof(test_str), "%d", test_int);
//    text_layer_set_text(minute_layer, finished ? test_str : "abrupt finish");
    if (timerCallback2) {
        noInterrupts = false;
        timerCallback2 = false;
    }
}

static void animation_started(Animation *animation, void *data) {
    // current_hour and current minute need to be updated in this callback in order to function properly
    strcpy(current_hour_dummy, current_hour);
    strcpy(current_tens_dummy, current_tens);
    strcpy(current_ones_dummy, current_ones);
    text_layer_set_text(hour_layer, current_hour_dummy);
    text_layer_set_text(tens_layer, current_tens_dummy);
    text_layer_set_text(ones_layer, current_ones_dummy);
}

static void animate_out(TextLayer *tlayer, PropertyAnimation *anim, GRect r_rect, GRect c_rect, GRect l_rect, uint32_t delay) {
    Layer *layer = text_layer_get_layer(tlayer);
    destroy_property_animation(&anim);
    anim = property_animation_create_layer_frame(layer, NULL, &l_rect);
	animation_set_duration((Animation*) anim, 400);
	animation_set_curve((Animation*) anim, AnimationCurveEaseIn);
    animation_set_delay((Animation*) anim, delay);
//    animation_set_handlers((Animation*) anim, (AnimationHandlers) {
//        .stopped = (AnimationStoppedHandler) animation_stopped
//    }, NULL /* callback data */);
	animation_schedule((Animation*) anim);
}

static void animate_in(TextLayer *tlayer, PropertyAnimation *anim, GRect r_rect, GRect c_rect, GRect l_rect, uint32_t delay) {
    Layer *layer = text_layer_get_layer(tlayer);
    anim = property_animation_create_layer_frame(layer, &r_rect, &c_rect);
	animation_set_duration((Animation*) anim, 400);
	animation_set_curve((Animation*) anim, AnimationCurveEaseOut);
    animation_set_delay((Animation*) anim, delay);
    animation_set_handlers((Animation*) anim, (AnimationHandlers) {
        .started = (AnimationStartedHandler) animation_started,
        .stopped = (AnimationStoppedHandler) animation_stopped
    }, NULL /* callback data */);
	animation_schedule((Animation*) anim);
}

static void animation_out_in() {
    animate_out(hour_layer, hour_anim, r_rect_hour, c_rect_hour, l_rect_hour, 0);
    animate_out(tens_layer, tens_anim, r_rect_tens, c_rect_tens, l_rect_tens, STAGGER_STR);
    animate_out(ones_layer, ones_anim, r_rect_ones, c_rect_ones, l_rect_ones, STAGGER_STR*2);
    // string updated in animation callback
    animate_in(hour_layer, hour_anim, r_rect_hour, c_rect_hour, l_rect_hour, STAGGER_IN);
    animate_in(tens_layer, tens_anim, r_rect_tens, c_rect_tens, l_rect_tens, STAGGER_IN + STAGGER_STR);
    animate_in(ones_layer, ones_anim, r_rect_ones, c_rect_ones, l_rect_ones, STAGGER_IN + STAGGER_STR*2);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	
    if (!noInterrupts) { // system can only call handle_tick if interrupts are allowed
        strcpy(previous_tens, current_tens);
        time_as_words(tick_time->tm_hour, tick_time->tm_min, current_hour, current_tens, current_ones);
    
        if (introComplete) {
            if ((units_changed & HOUR_UNIT) == HOUR_UNIT) {
                animate_out(hour_layer, hour_anim, r_rect_hour, c_rect_hour, l_rect_hour, 0);
                // string updated in animation callback
                animate_in(hour_layer, hour_anim, r_rect_hour, c_rect_hour, l_rect_hour, 600);
                
                more_info(tick_time); // update date every hour
            }
            if (strcmp(previous_tens, current_tens) != 0) {
                animate_out(tens_layer, tens_anim, r_rect_tens, c_rect_tens, l_rect_tens, 150);
                // string updated in animation callback
                animate_in(tens_layer, tens_anim, r_rect_tens, c_rect_tens, l_rect_tens, 600+150);
            }
            animate_out(ones_layer, ones_anim, r_rect_ones, c_rect_ones, l_rect_ones, 150+150);
            // string updated in animation callback
            animate_in(ones_layer, ones_anim, r_rect_ones, c_rect_ones, l_rect_ones, 600+150+150);
        }
        else {
            // intro animation
            animate_in(hour_layer, hour_anim, r_rect_hour, c_rect_hour, l_rect_hour, 0);
            animate_in(tens_layer, tens_anim, r_rect_tens, c_rect_tens, l_rect_tens, 150);
            animate_in(ones_layer, ones_anim, r_rect_ones, c_rect_ones, l_rect_ones, 150+150);
            introComplete = true;
            
            more_info(tick_time); // update date on app load
        }
    }
}

//static void timer_callback(void *context) {
//    time_t now = time(NULL);
//    struct tm * tick_time = localtime(&now);
//    noInterrupts = false;
//    handle_tick(tick_time, HOUR_UNIT);
//}

static void timer_callback_2(void *context) {
    //noInterrupts = false;
    timerCallback2 = true;
    //handle_tick(tick_time, HOUR_UNIT);
    time_t now = time(NULL);
    struct tm * tick_time = localtime(&now);
    time_as_words(tick_time->tm_hour, tick_time->tm_min, current_hour, current_tens, current_ones);
    
    animation_out_in();
}

static void timer_callback_1(void *context) {
    temp_format();
    strcpy(current_hour, weather_str);
    strcpy(current_tens, center_str);
    strcpy(current_ones, temp_str);
    
    // use '&' only if sync_error_callback hasn't been called
    // explicitly setting center_str in sync_error_callback isn't enough.
    strcpy(current_tens, strcmp(current_ones, "data ")==0 ? "weathr" : "&");
    
    animation_out_in();
    
    timer_handle = app_timer_register(WAIT_TIME + STAGGER_STR*2, timer_callback_2, NULL);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
    if (!noInterrupts) { // prevent calling accel_tap_handler multiple times mid-animation
        noInterrupts = true;
        
        strcpy(current_hour, day_str);
        strcpy(current_tens, mon_str);
        strcpy(current_ones, date_str);
        //    strcpy(current_hour, weather_str);
        //    strcpy(current_tens, temp_str);
        //    strcpy(current_ones, city_str);
        //    strcpy(current_hour, weather_str);
        //    strcpy(current_tens, center_str);
        //    strcpy(current_ones, temp_str);
        
        animation_out_in();
        
        timer_handle = app_timer_register(WAIT_TIME + STAGGER_STR*2, timer_callback_1, NULL);
    }
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
    strcpy(weather_str, "no    ");
    strcpy(center_str, "weathr");
    strcpy(temp_str, "data ");
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    // process the first and subsequent update
    switch (key) {
        case WEATHER_ICON_KEY:
            strcpy(weather_str, new_tuple->value->cstring);
            // make all lowercase
            weather_str[0] = tolower((unsigned char)weather_str[0]);
            break;
            
        case WEATHER_TEMPERATURE_F_KEY:
            strcpy(temp_f_str, new_tuple->value->cstring);
            strcat(temp_f_str, "\u00B0F");
            break;
        
        case WEATHER_TEMPERATURE_C_KEY:
            strcpy(temp_c_str, new_tuple->value->cstring);
            strcat(temp_c_str, "\u00B0C");
            break;
        
        case WEATHER_CITY_KEY:
            strncpy(city_str, new_tuple->value->cstring, 4);
            //strcpy(city_str, new_tuple->value->cstring);
            // make all lowercase
            city_str[0] = tolower((unsigned char)city_str[0]);
            break;
        
        case TEMP_FORMAT_KEY:
            config = (int)(new_tuple->value->uint8);
            break;
    }
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);

    window_set_background_color(window, GColorBlack);

    hour_layer = text_layer_create(r_rect_hour);
    tens_layer = text_layer_create(r_rect_tens);
    ones_layer = text_layer_create(r_rect_ones);
    text_layer_set_font(hour_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_font(tens_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    text_layer_set_font(ones_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    text_layer_set_text_color(hour_layer, GColorWhite);
    text_layer_set_text_color(tens_layer, GColorWhite);
    text_layer_set_text_color(ones_layer, GColorWhite);
    text_layer_set_background_color(hour_layer, GColorClear);
    text_layer_set_background_color(tens_layer, GColorClear);
    text_layer_set_background_color(ones_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(hour_layer));
    layer_add_child(window_layer, text_layer_get_layer(tens_layer));
    layer_add_child(window_layer, text_layer_get_layer(ones_layer));
    
    // prepare the initial values of your data
    Tuplet initial_values[] = {
        TupletCString(WEATHER_ICON_KEY, "no    "),
        TupletCString(WEATHER_TEMPERATURE_F_KEY, "data "),
        TupletCString(WEATHER_TEMPERATURE_C_KEY, "data "),
        TupletCString(WEATHER_CITY_KEY, "1234"),
        TupletInteger(TEMP_FORMAT_KEY, (uint8_t) 2),
    };
    // initialize the syncronization
    app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
        sync_tuple_changed_callback, sync_error_callback, NULL);
    //send_cmd();
}

static void window_unload(Window *window) {
    app_sync_deinit(&sync);
    text_layer_destroy(hour_layer);
    text_layer_destroy(tens_layer);
    text_layer_destroy(ones_layer);
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
    });
    
    // to sync watch fields
    const int inbound_size = 64;
    const int outbound_size = 64;
    app_message_open(inbound_size, outbound_size);
    
    const bool animated = true;
    window_stack_push(window, animated);
    
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
    accel_tap_service_subscribe(&accel_tap_handler);
}

static void deinit(void) {
    destroy_property_animation(&hour_anim);
    destroy_property_animation(&tens_anim);
    destroy_property_animation(&ones_anim);
    window_destroy(window);
    accel_tap_service_unsubscribe();
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
