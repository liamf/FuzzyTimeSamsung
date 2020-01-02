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

	/* Add our three columns to the display ( 3 vertical boxes) */
    face->leftColumn  = elm_box_add(face->naviframe);
    evas_object_move(face->leftColumn, width*0.25, height * 0.20);
    evas_object_show(face->leftColumn);

	face->midColumn   = elm_box_add(face->naviframe);
    evas_object_show(face->midColumn);

	face->rightColumn = elm_box_add(face->naviframe);
	evas_object_show(face->rightColumn);

	// Add our time row labels
	face->topTimeRow = elm_label_add(face->leftColumn);
	view_add_timelabel(face->leftColumn, face->topTimeRow);

	face->midTimeRow = elm_label_add(face->leftColumn);
	view_add_timelabel(face->leftColumn, face->midTimeRow);

	face->bottomTimeRow = elm_label_add(face->leftColumn);
	view_add_timelabel(face->leftColumn, face->bottomTimeRow);

	// Add our Second progress bar
	face->secondBarTop = evas_object_rectangle_add(face->midColumn);
	face->secondBarBottom = evas_object_rectangle_add(face->midColumn);
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

	if (watch_time == NULL)
		return;

	watch_time_get_hour24(watch_time, &hour24);
	watch_time_get_minute(watch_time, &minute);
	watch_time_get_second(watch_time, &second);

	fuzzy_time(hour24, minute, topLine, midLine, bottomLine);

	formatLine(formattedLine, topLine);
	elm_object_text_set(face->topTimeRow, formattedLine);

	formatLine(formattedLine, midLine);
	elm_object_text_set(face->midTimeRow, formattedLine);

	formatLine(formattedLine, bottomLine);
	elm_object_text_set(face->bottomTimeRow, formattedLine);

	view_set_second(face, second);
}

static void view_set_second(watchfacedata_s *face, int second)
{
	evas_object_move(face->secondBarTop, face->width*0.65, face->height*0.1);
	evas_object_resize(face->secondBarTop,5,face->height*0.8);
	evas_object_color_set(face->secondBarTop, 25,1,255,190);
}

static void view_add_timelabel(Evas_Object *box, Evas_Object *label)
{
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(label, EVAS_HINT_FILL, 1.0);
	evas_object_show(label);
	elm_box_pack_end(box, label);
}


static void formatLine( char *formatted, char *raw)
{
	snprintf(formatted, TEXTBUFSIZE, "<font=TizenSans font_weight=semibold font_size=55 align=right>%s</font>",raw);
}
