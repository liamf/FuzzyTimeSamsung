/*
 * Create base GUI and provide manipulation routines
 */
#include <tizen.h>
#include "fuzzytimesamsung.h"
#include "view.h"
#include "fuzzy_time.h"

#define TEXTBUFSIZE	256

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
	evas_object_move(face->topTimeRow, face->width*0.1, face->height*0.2);
	evas_object_resize(face->topTimeRow, face->width*0.5, face->height*0.2);
	evas_object_show(face->topTimeRow);

	face->midTimeRow = elm_label_add(face->naviframe);
	evas_object_move(face->midTimeRow, face->width*0.1, face->height*0.4);
	evas_object_resize(face->midTimeRow, face->width*0.5, face->height*0.2);
	evas_object_show(face->midTimeRow);

	face->bottomTimeRow = elm_label_add(face->naviframe);
	evas_object_move(face->bottomTimeRow, face->width*0.1, face->height*0.6);
	evas_object_resize(face->bottomTimeRow, face->width*0.5, face->height*0.2);
	evas_object_show(face->bottomTimeRow);

	face->ampm = elm_label_add(face->naviframe);
	evas_object_move(face->ampm, face->width*0.1, face->height*0.8);
	evas_object_resize(face->ampm, face->width*0.5, face->height*0.2);
	evas_object_show(face->ampm);

	// Add our Second progress bar
	face->secondBarTop = evas_object_rectangle_add(face->naviframe);
	face->secondBarBottom = evas_object_rectangle_add(face->naviframe);
	evas_object_color_set(face->secondBarTop, 0, 0, 0, 0);
	evas_object_color_set(face->secondBarBottom, 0, 0, 0, 0);
	evas_object_resize(face->secondBarTop, 0, 0);
	evas_object_resize(face->secondBarBottom, 0, 0);
	evas_object_show(face->secondBarTop);
	evas_object_show(face->secondBarBottom);

	ret = watch_time_get_current_time(&watch_time);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get current time. err = %d", ret);

	update_watch_face(face, watch_time, 0);
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

	view_set_second(face, second);
}

/* Animated seconds bar */
static void view_set_second(watchfacedata_s *face, int second)
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


static void formatLine( char *formatted, char *raw, bool hint)
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
