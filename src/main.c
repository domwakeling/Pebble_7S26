#include <pebble.h>
#include "gpathcentrerotate.h"

/*****************************************************/
/***************** DEFINE CONSTANTS ******************/	
/*****************************************************/

/* screen info */
#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
#define BACKGROUND_COLOUR GColorOrange

/* centre of rotation */
#define ROT_CENTRE_X 72
#define ROT_CENTRE_Y 84

/* top "circle" */
#define CENTRE_DIA 6
#define CIRCLE_STROKE_COLOUR GColorWhite
#define CIRCLE_FILL_COLOUR GColorBlack

/* hour hand info */	
#define HR_ROT_X 10
#define HR_ROT_Y 37

/* minute hand info */	
#define MIN_ROT_X 5
#define MIN_ROT_Y 66
	
	
/* seconds hand info */
#define SEC_ROT_X 5
#define SEC_ROT_Y 64

/*****************************************************/
/*********** DEFINE VARIABLES & 'OBJECTS' ************/	
/*****************************************************/

/* windows & layers */
static Window *main_window;
Layer *circle_layer;
BitmapLayer *dial_layer;
RotBitmapLayer *minute_layer, *hour_layer, *second_layer;
TextLayer *day_text_layer, *date_text_layer;


/* paths, fonts & bitmaps */
GBitmap *minute_hand_map, *hour_hand_map, *second_hand_map, *dial_map;
GFont eurostile_font;

/* variables */
char date_buff[] = "00";
char day_buff[] = "ABCD";
bool drawing_hands = false;
int hours, minutes, seconds, hours_display;
int hours_angle = -1;			// set to -1 as default so we draw on load
int minutes_angle = -1;		// set to -1 as default so we draw on load

int temp_angle = 0;


/*****************************************************/
/**************** LAYER UPDATE PROCS *****************/	
/*****************************************************/

void circle_update_proc(Layer *l, GContext *ctx) {
	graphics_context_set_fill_color(ctx, CIRCLE_FILL_COLOUR);
	graphics_context_set_stroke_color(ctx, CIRCLE_STROKE_COLOUR);
	graphics_fill_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), CENTRE_DIA);
	graphics_draw_circle(ctx, GPoint(ROT_CENTRE_X, ROT_CENTRE_Y), 1);
}

void rotated_map_update(RotBitmapLayer *l, int deg) {
	GRect r;
	int32_t rot_angle = deg * TRIG_MAX_ANGLE / 360;
	r = layer_get_frame((Layer *)l);
	r.origin.x = (SCREEN_WIDTH - r.size.w)/2;
	r.origin.y = (SCREEN_HEIGHT - r.size.h)/2;
	layer_set_frame((Layer *)l, r);
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
			return " SUN";
			break;
		case 1:
			return "MON ";
			break;
		case 2:
			return " TUE";
			break;
		case 3:
			return "WED ";
			break;
		case 4:
			return " THU";
			break;
		case 5:
			return " FRI";
			break;
		case 6:
			return " SAT";
			break;
		default:
			return " ERR";
			break;
	}
}

/*****************************************************/
/******************* TICK HANDLER ********************/	
/*****************************************************/

/* tick handler - update the hands */
void tick_handler(struct tm * tick_time, TimeUnits units_changed) {

	// as soon as tick_handler starts, set the hands to be visible
	drawing_hands = true;
	
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
		
		// set the date to show
		strftime(date_buff, sizeof("00"), "%e", tick_time);
		text_layer_set_text(date_text_layer, date_buff);
		
		// set the day to show - stage 1, get the day of week as a char
		char *temp_buff = "0";
		strftime(temp_buff, sizeof(temp_buff), "%w", tick_time);

		// set the day to show - stage 3, set date as string
		strcpy(day_buff, day_in_caps(temp_buff));
		text_layer_set_text(day_text_layer, day_buff);
		
		// set the day to show - stage 2, change text colour if necessary
		if(strcmp(day_buff, " SUN") == 0) {
			text_layer_set_text_color(day_text_layer, GColorRed);
		} else if(strcmp(day_buff, " SAT") == 0) {
			text_layer_set_text_color(day_text_layer, GColorDukeBlue);
		} else {
			text_layer_set_text_color(day_text_layer, GColorBlack);
		}
		
	}
	
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
	
	
	// un-hide the rotbitmap hands first time we draw - stops them showing in random position on load
	bool hands_hidden = layer_get_hidden((Layer *)minute_layer);
	if(hands_hidden) {
		layer_set_hidden((Layer *)minute_layer, false);
		layer_set_hidden((Layer *)hour_layer, false);
		layer_set_hidden((Layer *)second_layer, false);
	}
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
	circle_layer = layer_create(r);
	
	// set update procs
	layer_set_update_proc(circle_layer, circle_update_proc);
		
	// init our gpaths
	
	// init our bitmaps
	dial_map = gbitmap_create_with_resource(RESOURCE_ID_DIAL);
	minute_hand_map = gbitmap_create_with_resource(RESOURCE_ID_MINUTE_HAND);
	hour_hand_map = gbitmap_create_with_resource(RESOURCE_ID_HOUR_HAND);
	second_hand_map = gbitmap_create_with_resource(RESOURCE_ID_SECOND_HAND);
	
	// create rotbitmaplayers
	minute_layer = rot_bitmap_layer_create(minute_hand_map);
	hour_layer = rot_bitmap_layer_create(hour_hand_map);
	second_layer = rot_bitmap_layer_create(second_hand_map);
	
	// set rotation points
	rot_bitmap_set_src_ic(minute_layer, GPoint(MIN_ROT_X, MIN_ROT_Y));
	rot_bitmap_set_src_ic(hour_layer, GPoint(HR_ROT_X, HR_ROT_Y));
	rot_bitmap_set_src_ic(second_layer, GPoint(SEC_ROT_X, SEC_ROT_Y));
	
	// set compositing modes
	rot_bitmap_set_compositing_mode(minute_layer, GCompOpSet);
	rot_bitmap_set_compositing_mode(hour_layer, GCompOpSet);
	rot_bitmap_set_compositing_mode(second_layer, GCompOpSet);	
	
	// create background bitmaplayer
	dial_layer = bitmap_layer_create(r);
	bitmap_layer_set_bitmap(dial_layer, dial_map);
		
	// load the custom text
	eurostile_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_EUROSTILE_11));
	
	// create the text layers
	date_text_layer = text_layer_create(GRect(108,77,30,30));
	day_text_layer = text_layer_create(GRect(97,77,50,30)); 
	
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
	layer_add_child(w_layer, (Layer *)hour_layer);
	layer_add_child(w_layer, (Layer *)minute_layer);
	layer_add_child(w_layer, (Layer *)second_layer);
	layer_add_child(w_layer, circle_layer);
	
	// hide the hand layers until we've placed them!
	layer_set_hidden((Layer *)minute_layer, true);
	layer_set_hidden((Layer *)hour_layer, true);
	layer_set_hidden((Layer *)second_layer, true);
}

static void main_window_unload(Window *w) {
	// destroy layers
	rot_bitmap_layer_destroy(hour_layer);
	rot_bitmap_layer_destroy(second_layer);
	layer_destroy(circle_layer);
	rot_bitmap_layer_destroy(minute_layer);
	bitmap_layer_destroy(dial_layer);
	
	// destroy gpaths & gbitmaps
	gbitmap_destroy(minute_hand_map);
	gbitmap_destroy(hour_hand_map);
	gbitmap_destroy(second_hand_map);
	gbitmap_destroy(dial_map);
	
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