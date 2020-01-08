/*
 * Create base GUI and provide manipulation routines
 */
#include <stdlib.h>
#include <tizen.h>
#include <privacy_privilege_manager.h>
#include <sensor.h>
#include <device/battery.h>


#include "fuzzytimesamsung.h"
#include "view.h"
#include "fuzzy_time.h"

#define TEXTBUFSIZE	256

sensor_listener_h hrmSensorListener;	// Not sure if we need to keep these around
sensor_listener_h stepsSensorListener;

void hrm_sensor_callback(sensor_h, sensor_event_s *, void *);
void steps_sensor_callback(sensor_h, sensor_event_s *, void *);

void get_sensor_permissions();
void sensor_permission_response_callback(ppm_call_cause_e, ppm_request_result_e, const char *, void *);


void create_watch_face(watchfacedata_s *face, int width, int height)
{
	int ret;
	watch_time_h watch_time = NULL;

	/* Window */
	ret = watch_app_get_elm_win(&face->win);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get window. err = %d", ret);
		return;
	}

	evas_object_resize(face->win, width, height);

	face->width = width;
	face->height = height;
	face->beatsPerMinute = 0;
	face->steps = 0;
	face->privileged = 0;

	/* Conformant */
	face->conform = elm_conformant_add(face->win);
	evas_object_size_hint_weight_set(face->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(face->win, face->conform);
	evas_object_show(face->conform);

	/* Naviframe */
	face->naviframe = elm_naviframe_add(face->conform);
	evas_object_size_hint_weight_set(face->naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(face->conform, face->naviframe);
	evas_object_show(face->naviframe);

	// Add our time row labels
	face->topTimeRow = elm_label_add(face->naviframe);
	evas_object_move(face->topTimeRow, face->width*0.09, face->height*0.18);
	evas_object_resize(face->topTimeRow, face->width*0.5, face->height*0.2);
	evas_object_show(face->topTimeRow);

	face->midTimeRow = elm_label_add(face->naviframe);
	evas_object_move(face->midTimeRow, face->width*0.09, face->height*0.38);
	evas_object_resize(face->midTimeRow, face->width*0.5, face->height*0.2);
	evas_object_show(face->midTimeRow);

	face->bottomTimeRow = elm_label_add(face->naviframe);
	evas_object_move(face->bottomTimeRow, face->width*0.09, face->height*0.58);
	evas_object_resize(face->bottomTimeRow, face->width*0.5, face->height*0.2);
	evas_object_show(face->bottomTimeRow);

	face->ampm = elm_label_add(face->naviframe);
	evas_object_move(face->ampm, face->width*0.1, face->height*0.8);
	evas_object_resize(face->ampm, face->width*0.5, face->height*0.2);
	evas_object_show(face->ampm);

	// Add our Second progress bar : they'll be sized correctly by the watch update
	face->secondBarTop = evas_object_rectangle_add(face->naviframe);
	face->secondBarBottom = evas_object_rectangle_add(face->naviframe);
	evas_object_show(face->secondBarTop);
	evas_object_show(face->secondBarBottom);

	// Add the various display widgets
	add_display_widgets(face);

	ret = watch_time_get_current_time(&watch_time);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get current time. err = %d", ret);

	update_watch_face(face, watch_time, 0);

	update_display_widgets(face, watch_time);

	watch_time_delete(watch_time);

	/* Show window after base gui is set up */
	evas_object_show(face->win);
}


void update_watch_face(watchfacedata_s *face, watch_time_h watch_time, int ambient)
{
	int hour24, minute, second;

	char topLine[24];
	char midLine[24];
	char bottomLine[24];
	char formattedLine[TEXTBUFSIZE];
	int hint;
	int pm;

	if (watch_time == NULL)
		return;

	watch_time_get_hour24(watch_time, &hour24);
	watch_time_get_minute(watch_time, &minute);
	watch_time_get_second(watch_time, &second);

	if( hour24 == 0 && minute == 0 && second < 5 )
	{
		// Zero the steps counter
		face->stepsTakenToday = 0;
	}

	fuzzy_time(hour24, minute, topLine, midLine, bottomLine, &pm, &hint);

	formatLine(formattedLine, topLine, false);
	elm_object_text_set(face->topTimeRow, formattedLine);

	formatLine(formattedLine, midLine, (hint == 2));
	elm_object_text_set(face->midTimeRow, formattedLine);

	formatLine(formattedLine, bottomLine, (hint == 3));
	elm_object_text_set(face->bottomTimeRow, formattedLine);

	if( pm )
	{
		snprintf(formattedLine, TEXTBUFSIZE, "<color=#0000FFFF><font=TizenSans font_weight=semibold font_size=24 align=right>pm</font></color>");
	}
	else
	{
		snprintf(formattedLine, TEXTBUFSIZE, "<color=#0000FFFF><font=TizenSans font_weight=semibold font_size=24 align=right>am</font></color>");
	}
	elm_object_text_set(face->ampm, formattedLine);

	// Set the "second hand" progress bar
	view_set_second(face, second);

	// Update the widgets every 10 seconds
	// ... which is too often anyway ... not going to change that fast ...
	if( (second % 10) == 0 )
	{
		update_display_widgets(face, watch_time);
	}

}

// Create the label widgets for updated information from sensors
void add_display_widgets(watchfacedata_s *face)
{
	face->heartrate = elm_label_add(face->naviframe);
	evas_object_move(face->heartrate, face->width*0.7, face->height*0.22);
	evas_object_resize(face->heartrate, face->width*0.2, face->height*0.20);
	evas_object_show(face->heartrate);

	face->battery = elm_label_add(face->naviframe);
	evas_object_move(face->battery, face->width*0.7, face->height*0.35);
	evas_object_resize(face->battery, face->width*0.2, face->height*0.2);
	evas_object_show(face->battery);

	face->steps = elm_label_add(face->naviframe);
	evas_object_move(face->steps, face->width*0.7, face->height*0.48);
	evas_object_resize(face->steps, face->width*0.2, face->height*0.2);
	evas_object_show(face->steps);

	face->date = elm_label_add(face->naviframe);
	evas_object_move(face->date, face->width*0.7, face->height*0.70);
	evas_object_resize(face->date, face->width*0.2, face->height*0.2);
	evas_object_show(face->date);

}

// Create the sensor callbacks
void create_sensor_callbacks(watchfacedata_s *face)
{
	sensor_h sensor;
	int ret;

	// Seems that we have to request privileges from the user before setting this up
	// Otherwise ... app will not work correctly
	get_sensor_permissions();

	sensor_get_default_sensor(SENSOR_HRM, &sensor);
	ret = sensor_create_listener(sensor, &hrmSensorListener);
	if( ret != SENSOR_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get hrm sensor. err = %d", ret);

	ret = sensor_listener_set_event_cb(hrmSensorListener, 20000, hrm_sensor_callback, face);
	if( ret != SENSOR_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to set hrm sensor listener. err = %d", ret);

	sensor_listener_set_option(hrmSensorListener, SENSOR_OPTION_DEFAULT);
	sensor_listener_start(hrmSensorListener);

	sensor_get_default_sensor(SENSOR_HUMAN_PEDOMETER, &sensor);
	ret = sensor_create_listener(sensor, &stepsSensorListener);
	if( ret != SENSOR_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get steps sensor. err = %d", ret);

	ret = sensor_listener_set_event_cb(stepsSensorListener, 20000, steps_sensor_callback, face);
	if( ret != SENSOR_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to set steps sensor listener. err = %d", ret);

	sensor_listener_set_option(stepsSensorListener, SENSOR_OPTION_DEFAULT);
	sensor_listener_start(stepsSensorListener);
}

// Update them
void update_display_widgets(watchfacedata_s *face, watch_time_h watchtime)
{
	char formattedLine[TEXTBUFSIZE];
	int day;
	int month;
	int batteryCharge;
	sensor_event_s event;

	if( face->privileged == 0)
	{
		create_sensor_callbacks(face);
		face->privileged = 1;
	}

	event.values[0]=0;
	// Update the HRM now. Dont rely on the callback which seems a bit unreliable
	sensor_listener_read_data(hrmSensorListener, &event);
	if( (int)event.values[0] > 0)
	{
		face->beatsPerMinute = (int)event.values[0];
	}

	if( face->beatsPerMinute < 100 )
	{
		snprintf(formattedLine, TEXTBUFSIZE, "<font=TizenSans font_weight=medium font_size=24 align=left>%d bpm</font>",face->beatsPerMinute);
	}
	else
	{
		snprintf(formattedLine, TEXTBUFSIZE, "<font=TizenSans font_weight=medium font_size=24 align=left>%d pm</font>",face->beatsPerMinute);
	}
	elm_object_text_set(face->heartrate, formattedLine);

	event.values[0] = 0;
	// Update the step count now. Dont rely on the callback which seems a bit unreliable
	sensor_listener_read_data(stepsSensorListener, &event);
	if( (int)event.values[0] > 0)
	{
		face->stepsTakenToday = (int)event.values[0];
	}
	snprintf(formattedLine, TEXTBUFSIZE, "<font=TizenSans font_weight=medium font_size=24 align=left>%d<br/>steps</font>",face->stepsTakenToday);
	elm_object_text_set(face->steps, formattedLine);

	device_battery_get_percent(&batteryCharge);
	snprintf(formattedLine, TEXTBUFSIZE, "<font=TizenSans font_weight=medium font_size=24 align=left>%d %%</font>",batteryCharge);
	elm_object_text_set(face->battery, formattedLine);

	if( watchtime != NULL)
	{
		watch_time_get_day(watchtime, &day);
		watch_time_get_month(watchtime, &month);
		snprintf(formattedLine, TEXTBUFSIZE, "<color=#0000FFFF><font=TizenSans font_weight=bold font_size=24 align=left>%d %s</font></color>",day, MONTHS[month-1]);
		elm_object_text_set(face->date, formattedLine);
	}
}

/* Animated seconds bar */
void view_set_second(watchfacedata_s *face, int second)
{
	int barHeight;
	int topPart;

	barHeight = face->height * 0.8;
	topPart = (barHeight * second) / 60;

	evas_object_move(face->secondBarTop, face->width*0.65, face->height*0.1);
	evas_object_resize(face->secondBarTop,3,topPart);
	evas_object_color_set(face->secondBarTop, 100,0,255,180);

	evas_object_move(face->secondBarBottom, face->width*0.65, face->height*0.1 + topPart);
	evas_object_resize(face->secondBarBottom, 3, barHeight - topPart);
	evas_object_color_set(face->secondBarBottom, 0,0,255,180);
}

// Sensor handler callback : stash the value for later
// Note we get a callback only when the value changes ... not sure how to just read the current value
void hrm_sensor_callback(sensor_h sensor, sensor_event_s *event, void *user_data)
{
	sensor_type_e sensorType;
	sensor_get_type(sensor, &sensorType);

	watchfacedata_s *face = (watchfacedata_s *)user_data;

	if( sensorType == SENSOR_HRM)
	{
		if( (int)event->values[0] > 0)
		{
			face->beatsPerMinute = (int)event->values[0];
		}
	}
}

// Sensor handler callback : stash the value for later
void steps_sensor_callback(sensor_h sensor, sensor_event_s *event, void *user_data)
{
	sensor_type_e sensorType;
	sensor_get_type(sensor, &sensorType);

		watchfacedata_s *face = (watchfacedata_s *)user_data;

	if (sensorType == SENSOR_HUMAN_PEDOMETER)
	{
		if( (int)event->values[0] > 0)
		{
			face->stepsTakenToday = (int)event->values[0];
		}
	}

}

// This seems to be required ... even if the sensor priv is set manually in the app ...
void get_sensor_permissions()
{
    int ret;
	ppm_check_result_e result;

    ret = ppm_check_permission("http://tizen.org/privilege/healthinfo", &result);
    if( ret == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE)
    {
		switch( result)
		{
			case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
				dlog_print(DLOG_ERROR, LOG_TAG, "allowed sensor priv http://tizen.org/privilege/healthinfo");
				break;

			case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
				dlog_print(DLOG_ERROR, LOG_TAG, "denied sensor priv http://tizen.org/privilege/healthinfo");
				break;

			case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
				dlog_print(DLOG_ERROR, LOG_TAG, "asking for sensor priv http://tizen.org/privilege/healthinfo");
				ret = ppm_request_permission("http://tizen.org/privilege/healthinfo", sensor_permission_response_callback, NULL);
				break;
		}

	    ret = ppm_check_permission("http://tizen.org/privilege/display", &result);
	    if( ret == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE)
	    {
			switch( result)
			{
				case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
					dlog_print(DLOG_ERROR, LOG_TAG, "allowed sensor priv http://tizen.org/privilege/display");
					break;

				case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
					dlog_print(DLOG_ERROR, LOG_TAG, "denied sensor priv http://tizen.org/privilege/display");
					break;

				case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
					dlog_print(DLOG_ERROR, LOG_TAG, "asking for sensor priv http://tizen.org/privilege/display");
					ret = ppm_request_permission("http://tizen.org/privilege/healthinfo", sensor_permission_response_callback, NULL);
					break;
			}
	    }

    }
}

// sensor permission callback
void sensor_permission_response_callback(ppm_call_cause_e cause, ppm_request_result_e result, const char *priv , void *data)
{
    if (cause == PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ERROR) {
        /* Log and handle errors */
    	dlog_print(DLOG_ERROR, LOG_TAG, "callback for a manager error for privs");
        return;
    }

	switch (result) {
		case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER:
			/* Update UI and start accessing protected functionality */
			dlog_print(DLOG_ERROR, LOG_TAG, "allowed priv forever %s", priv);
			break;

		case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_FOREVER:
			/* Show a message and terminate the application */
			dlog_print(DLOG_ERROR, LOG_TAG, "denied priv forever %s", priv);
			break;

		case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_ONCE:
			/* Show a message with explanation */
			dlog_print(DLOG_ERROR, LOG_TAG, "allowed priv once %s", priv);
			break;
	}
}

// Format a main time line
void formatLine( char *formatted, char *raw, bool hint)
{
	if( hint)
	{
		snprintf(formatted, TEXTBUFSIZE, "<color=#0000FFFF><font=TizenSans font_weight=semibold font_size=56 align=right>%s</font></color>",raw);
	}
	else
	{
		snprintf(formatted, TEXTBUFSIZE, "<font=TizenSans font_weight=thin font_size=56 align=right>%s</font>",raw);
	}
}
