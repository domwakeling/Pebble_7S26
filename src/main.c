#include <pebble.h>

//#define DCW_DEBUG			// delete the comment qualifier to include debug options at compile
#ifdef DCW_DEBUG
	#include "debugassist.h"
#endif

/*****************************************************/
/******************* DEFINE KEYS *********************/	
/*****************************************************/
	
#define KEY_DIAL_COLOUR 0
#define PERSIST_DIAL_COLOUR 0


/*****************************************************/
/***************** DEFINE CONSTANTS ******************/	
/*****************************************************/

/* screen info */
#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
#define BACKGROUND_COLOUR GColorOrange

/* centre of rotation */
#define ROT_CENTRE_X 71
#define ROT_CENTRE_Y 83

/* top "circle" */
#define CENTRE_DIA 6
#define CIRCLE_STROKE_COLOUR GColorWhite
#define CIRCLE_FILL_COLOUR GColorBlack

/* hour hand info */	
#define HR_ROT_X 10
#define HR_ROT_Y 39

/* minute hand info */	
#define MIN_ROT_X 5
#define MIN_ROT_Y 66
	
/* seconds hand info */
#define SEC_ROT_X 4
#define SEC_ROT_Y 64

/*****************************************************/
/*********** DEFINE VARIABLES & 'OBJECTS' ************/	
/*****************************************************/

/* windows & layers */
static Window *main_window;
Layer *hand_group_layer;
Layer *circle_layer;
BitmapLayer *dial_layer;
RotBitmapLayer *minute_layer, *hour_layer, *second_layer;
TextLayer *day_text_layer, *date_text_layer;

/* paths, fonts & bitmaps */
GBitmap *minute_hand_map_o, *hour_hand_map_o, *second_hand_map_o, *dial_map_o;
GBitmap *minute_hand_map_b, *hour_hand_map_b, *second_hand_map_b, *dial_map_b;
GFont eurostile_font;

/* variables */
char date_buff[] = "00";
char day_buff[] = "ABCD";
bool hands_drawn = false;
int hours, minutes, seconds, hours_display;
int hours_angle = -1;			// set to -1 as default so we draw on load
int minutes_angle = -1;		// set to -1 as default so we draw on load
bool dial_is_black;


/*****************************************************/
/**************** LAYER UPDATE PROCS *****************/	
/*****************************************************/

void circle_update_proc(Layer *l, GContext *ctx) {
	
	// if DCW_DEBUG is defined we will draw cross-hairs and two circles, otherwise the "pommel"
	#ifdef DCW_DEBUG
		graphics_context_set_fill_color(ctx, GColorWhite);
		graphics_context_set_stroke_color(ctx, GColorWhite);
		graphics_draw_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), 63);
		graphics_draw_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), 22);
		graphics_draw_line(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y - 4), GPoint(ROT_CENTRE_X, ROT_CENTRE_Y + 4));
		graphics_draw_line(ctx, GPoint(ROT_CENTRE_X - 4, ROT_CENTRE_Y), GPoint(ROT_CENTRE_X + 4, ROT_CENTRE_Y));
	#else
		if(dial_is_black) {
			graphics_context_set_fill_color(ctx, CIRCLE_STROKE_COLOUR);
			graphics_context_set_stroke_color(ctx, CIRCLE_FILL_COLOUR);
			graphics_fill_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), CENTRE_DIA + 1);
			graphics_draw_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), CENTRE_DIA + 1);
		} else {
			graphics_context_set_fill_color(ctx, CIRCLE_FILL_COLOUR);
			graphics_context_set_stroke_color(ctx, CIRCLE_STROKE_COLOUR);
			graphics_fill_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), CENTRE_DIA);
		}
		graphics_draw_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), 1);
	#endif
}

void dummy_update_proc(Layer *l, GContext *ctx) {
}

void rotated_map_update(RotBitmapLayer *l, int deg) {
	if(!hands_drawn) {
		GRect r;
		r = layer_get_frame((Layer *)l);
		r.origin.x = ROT_CENTRE_X - r.size.w/2;
		r.origin.y = ROT_CENTRE_Y - r.size.h/2;
		layer_set_frame((Layer *)l, r);
	}
	int32_t rot_angle = deg * TRIG_MAX_ANGLE / 360;
	rot_bitmap_layer_set_angle(l, rot_angle);
}


/*****************************************************/
/********** TICK HANDLER HELPER FUNCTIONS ************/	
/*****************************************************/

/* get angle in degrees of the minute hand */
int get_minutes_angle() {
	int theta = (int) ( (float)minutes * (360/60.0) + (float)seconds * (360/(60*60.0)) );
	return theta;
}

/* get angle in degrees of the hours hand */
int get_hours_angle() {
	int theta = (int) ( (float)hours * (360/12.0) + (float)minutes * (360/(12*60.0)) );
	return theta;
}

/* return day (as char) as int */
int day_char_as_int(char *day) {
	int temp_num = day[0] - (char)'0';
	return temp_num;
}

/* upper case of western calendar */
char *day_in_caps(char *day) {	
	
	switch (day_char_as_int(day)) {
		case 0:
			return "SUN ";
			break;
		case 1:
			return "MON ";
			break;
		case 2:
			return "TUE ";
			break;
		case 3:
			return "WED ";
			break;
		case 4:
			return "THU ";
			break;
		case 5:
			return "FRI ";
			break;
		case 6:
			return "SAT ";
			break;
		default:
			return " ERR";
			break;
	}
}

/*****************************************************/
/********** HELPERS FOR B&W / DRAWING LAYERS *********/	
/*****************************************************/

/* helper to create all the gbitmaps */
void load_bitmaps() {
	dial_map_b = gbitmap_create_with_resource(RESOURCE_ID_DIAL_B);
	second_hand_map_b = gbitmap_create_with_resource(RESOURCE_ID_SECOND_HAND_B);
	minute_hand_map_b = gbitmap_create_with_resource(RESOURCE_ID_MINUTE_HAND_B);
	hour_hand_map_b = gbitmap_create_with_resource(RESOURCE_ID_HOUR_HAND_B);
	dial_map_o = gbitmap_create_with_resource(RESOURCE_ID_DIAL);
	hour_hand_map_o = gbitmap_create_with_resource(RESOURCE_ID_HOUR_HAND);
	minute_hand_map_o = gbitmap_create_with_resource(RESOURCE_ID_MINUTE_HAND);
	second_hand_map_o = gbitmap_create_with_resource(RESOURCE_ID_SECOND_HAND);
}

/* helper to destroy all the gbitmaps */
void destroy_bitmaps() {
	gbitmap_destroy(dial_map_b);
	gbitmap_destroy(dial_map_o);
	gbitmap_destroy(second_hand_map_b);
	gbitmap_destroy(second_hand_map_o);
	gbitmap_destroy(minute_hand_map_b);
	gbitmap_destroy(minute_hand_map_o);
	gbitmap_destroy(hour_hand_map_b);
	gbitmap_destroy(hour_hand_map_o);
}

/* helpers to get the correct bitmap for dial colour */
GBitmap* active_dial_map() {
	if(dial_is_black) {
		return dial_map_b;
	} else {
		return dial_map_o;
	}
}

GBitmap* active_hour_map() {
	if(dial_is_black) {
		return hour_hand_map_b;
	} else {
		return hour_hand_map_o;
	}
}

GBitmap* active_minute_map() {
	if(dial_is_black) {
		return minute_hand_map_b;
	} else {
		return minute_hand_map_o;
	}
}

GBitmap* active_second_map() {
	if(dial_is_black) {
		return second_hand_map_b;
	} else {
		return second_hand_map_o;
	}
}

/* helper routine to set up the dial and hands, bool whether we need to destroy rotbitmaplayers */
void initialise_dial_and_hands(bool destroy_required) {
	
	strcpy(date_buff, "00");
	strcpy(day_buff, "ABCD");
	hours_angle = -1;			// set to -1 as default so we draw on load
	minutes_angle = -1;		// set to -1 as default so we draw on load
	
	// if destroy_required true, destroy the hand rotbitmaplayers
	if(destroy_required) {
		rot_bitmap_layer_destroy(hour_layer);
		rot_bitmap_layer_destroy(minute_layer);
		rot_bitmap_layer_destroy(second_layer);
	}
	
	// set the dial layer
	bitmap_layer_set_bitmap(dial_layer, active_dial_map());
	
	// set up the rotbitmaplayers
	hour_layer = rot_bitmap_layer_create(active_hour_map());
	minute_layer = rot_bitmap_layer_create(active_minute_map());
	second_layer = rot_bitmap_layer_create(active_second_map());
	rot_bitmap_set_compositing_mode(hour_layer, GCompOpSet);
	rot_bitmap_set_compositing_mode(minute_layer, GCompOpSet);
	rot_bitmap_set_compositing_mode(second_layer, GCompOpSet);
	rot_bitmap_set_src_ic(minute_layer, GPoint(MIN_ROT_X, MIN_ROT_Y));
	rot_bitmap_set_src_ic(second_layer, GPoint(SEC_ROT_X, SEC_ROT_Y));
	rot_bitmap_set_src_ic(hour_layer, GPoint(HR_ROT_X, HR_ROT_Y));
	
	// add the rotbitmaplayers
	layer_add_child(hand_group_layer, (Layer *)hour_layer);
	layer_add_child(hand_group_layer, (Layer *)minute_layer);
	layer_add_child(hand_group_layer, (Layer *)second_layer);
}


/*****************************************************/
/************ UPDATE TIME & TICK HANDLER *************/	
/*****************************************************/

/* update the hands etc */
void update_time() {

	// store current time
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	// get time into storage
	hours = (int)tick_time->tm_hour;
	minutes = (int)tick_time->tm_min;
	seconds = (int)tick_time->tm_sec;
	
	// deal with 24-hour clock for calculation purposes
	if( hours >= 12) {
		hours_display = hours - 12;
	} else {
		hours_display = hours;
	}
	
	// deal with leap seconds!
	if(seconds > 59) seconds = 59;
	
	// update date if either it's not displaying OR this is a new day
	if(strcmp(date_buff, "00") == 0 || (hours == 0 && minutes == 0 && seconds == 0)) {
		
		// set the colour
		if(dial_is_black) {
			text_layer_set_text_color(date_text_layer, GColorWhite);
		} else {
			text_layer_set_text_color(date_text_layer, GColorBlack);
		}
		
		// set the date to show
		strftime(date_buff, sizeof("00"), "%e", tick_time);
		
		// check if first character is a string, if so swap order
		if(date_buff[0] == ' ') {
			char temp_char = date_buff[1];
			date_buff[0] = temp_char;
			date_buff[1] = ' ';
		}
				
		// show the date
		text_layer_set_text(date_text_layer, date_buff);
		
		// set the day to show - stage 1, get the day of week as a char
		char *temp_buff = "0";
		strftime(temp_buff, sizeof(temp_buff), "%w", tick_time);

		// set the day to show - stage 2, set date as string
		strcpy(day_buff, day_in_caps(temp_buff));
		
		// set the day to show - stage 3, change text colour if necessary and display
		text_layer_set_text(day_text_layer, day_buff);
		if(strcmp(day_buff, "SUN ") == 0) {
			text_layer_set_text_color(day_text_layer, GColorRed);
		} else if(strcmp(day_buff, "SAT ") == 0 && !dial_is_black) {
			text_layer_set_text_color(day_text_layer, GColorBlue);
		} else if(dial_is_black){
			text_layer_set_text_color(day_text_layer, GColorWhite);
		} else {
			text_layer_set_text_color(day_text_layer, GColorBlack);
		}		
	}
	
	// debugging option for date and day - #define DCW_DEBUG to use
	#ifdef DCW_DEBUG
		snprintf(date_buff, sizeof(date_buff), "%d", seconds);
		text_layer_set_text(date_text_layer, date_buff);
		strcpy(day_buff, day_in_caps_from_int(seconds%7));
		text_layer_set_text(day_text_layer, day_buff);
		if(strcmp(day_buff, "SUN ") == 0) {
			text_layer_set_text_color(day_text_layer, GColorRed);
		} else if(strcmp(day_buff, "SAT ") == 0 && !dial_is_black) {
			text_layer_set_text_color(day_text_layer, GColorBlue);
		} else if(dial_is_black){
			text_layer_set_text_color(day_text_layer, GColorWhite);
		} else {
			text_layer_set_text_color(day_text_layer, GColorBlack);
		}	
	#endif
		
	// check whether we need to re-draw hours hand
	if( hours_angle == -1 || hours_angle != get_hours_angle() ) {
		hours_angle = get_hours_angle();
		rotated_map_update(hour_layer, hours_angle);	
	}
	
	// check whether we need to re-draw minutes hand
	if( minutes_angle == -1 || minutes_angle != get_minutes_angle() ) {
		minutes_angle = get_minutes_angle();
		rotated_map_update(minute_layer, minutes_angle);
	}
	
	// draw seconds hand
	rotated_map_update(second_layer, seconds * 360 / 60);
	
	// debug option to hide hours and minute hands - #define DCW_DEBUG to use
	#ifdef DCW_DEBUG
		layer_set_hidden((Layer*)minute_layer, true);
		layer_set_hidden((Layer*)hour_layer, true);
	#endif
	
	hands_drawn = true;
}

/* tick handler - call update_time() */
void tick_handler(struct tm * tick_time, TimeUnits units_changed) {
	update_time();
}


/*****************************************************/
/********* COMMUNICATION HANDLER CALLBACKS ***********/	
/*****************************************************/

/*inbox received callback, deal with successful receipt of message from phone */
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

	// define our temporary buffer first
	char colour_buffer[7];
	
	// read first item from the dictionary
	Tuple *t = dict_read_first(iterator);
	
	// there's only going to be one key at the moment ...
	while(t != NULL) {
		
		switch(t->key) {
			
			// if it's the bluetooth-showing item
			case KEY_DIAL_COLOUR:
				// use persist_log_bool to log value and use the return value (1 true, 0 false, -1 error)
				snprintf(colour_buffer, sizeof(colour_buffer), "%s", t->value->cstring );
				APP_LOG(APP_LOG_LEVEL_INFO, "Colour received: %s", colour_buffer);
				if(strcmp(colour_buffer, "orange") == 0) {
					dial_is_black = false;
				} else {
					dial_is_black = true;
				}
				persist_write_bool(PERSIST_DIAL_COLOUR, dial_is_black);
				initialise_dial_and_hands(true);
				break;		
		}
		
		// Look for next item
    t = dict_read_next(iterator);
	}
}

/* inbox dropped callback - logs a message that there's been an error */
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped");
}


/*****************************************************/
/*************** MAIN WINDOW HANDLERS ****************/	
/*****************************************************/

static void main_window_load(Window *w) {
	
	window_set_background_color(w, BACKGROUND_COLOUR);
	
	// get a layer for the window_root for easy reference
	Layer *w_layer = window_get_root_layer(w);
	
	// init our general layers
	GRect r = GRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	hand_group_layer = layer_create(r);
	circle_layer = layer_create(r);
	
	// set update procs
	layer_set_update_proc(circle_layer, circle_update_proc);
	layer_set_update_proc(hand_group_layer, dummy_update_proc);
	
	// load the gbitmaps
	load_bitmaps();
	
	// set the bitmap layers up
	dial_layer = bitmap_layer_create(r);
	
	// load the custom text
	eurostile_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_EUROSTILE_12));
	
	// create the text layers
	date_text_layer = text_layer_create(GRect(107,76,30,30));
	day_text_layer = text_layer_create(GRect(95,76,50,30)); 
	
	// set their initial format
	text_layer_set_background_color(date_text_layer, GColorClear);
	text_layer_set_background_color(day_text_layer, GColorClear);
	text_layer_set_text_color(date_text_layer, GColorBlack);
	text_layer_set_text_color(day_text_layer, GColorBlack);
	text_layer_set_text_alignment(date_text_layer, GTextAlignmentRight);
	text_layer_set_text_alignment(day_text_layer, GTextAlignmentLeft);
	text_layer_set_font(date_text_layer, eurostile_font);
	text_layer_set_font(day_text_layer, eurostile_font);		
		
	// add layers to window
	layer_add_child(w_layer, (Layer *)dial_layer);
	layer_add_child(w_layer, text_layer_get_layer(day_text_layer));
	layer_add_child(w_layer, text_layer_get_layer(date_text_layer));
	layer_add_child(w_layer, hand_group_layer);
	layer_add_child(w_layer, circle_layer);
	
	// read persistant state for black dial and set accordingly
	if(persist_exists(PERSIST_DIAL_COLOUR)) {
		dial_is_black = persist_read_bool(PERSIST_DIAL_COLOUR);
	} else {
		dial_is_black = false;
		persist_write_bool(PERSIST_DIAL_COLOUR, dial_is_black);
	}
	
	// set the dial and create/set up the hands
	initialise_dial_and_hands(false);
	
	update_time();
}

static void main_window_unload(Window *w) {
	// destroy layers
	bitmap_layer_destroy(dial_layer);
	rot_bitmap_layer_destroy(hour_layer);
	rot_bitmap_layer_destroy(second_layer);
	rot_bitmap_layer_destroy(minute_layer);
	layer_destroy(hand_group_layer);
	layer_destroy(circle_layer);
	
	// destroy gpaths & gbitmaps
	destroy_bitmaps();
	
	// destroy text layers
	text_layer_destroy(date_text_layer);
	text_layer_destroy(day_text_layer);
}


/*****************************************************/
/*************** INIT, DEINIT & MAIN *****************/	
/*****************************************************/

static void init() {
	// create main Window element and assign to pointer
	main_window = window_create();
	
	// subscribe to tick timer
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
	
	// set handlers to manage the elements inside the Window
	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});	
	
	// register appMessage handlers
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	
	// open AppMessage
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	// show the Window on the watch, with animated = true
	window_stack_push(main_window, true);
}


static void deinit() {
	window_destroy(main_window);
	tick_timer_service_unsubscribe();
}


int main(void) {
	init();
	app_event_loop();
	deinit();
}