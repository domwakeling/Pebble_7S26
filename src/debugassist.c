#include <pebble.h>
#include "debugassist.h"

/* upper case of western calendar */
char *day_in_caps_from_int(int day) {	
	
	switch (day) {
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
