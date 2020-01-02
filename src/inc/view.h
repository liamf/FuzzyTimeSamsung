#ifndef __view_H__
#define __view_H__

#include <tizen.h>
#include <watch_app.h>
#include <watch_app_efl.h>
#include <Elementary.h>
#include <dlog.h>

typedef struct watchfacedata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *naviframe;

	Evas_Object *screenGrid;

	Evas_Object *topTimeRow;
	Evas_Object *midTimeRow;
	Evas_Object *bottomTimeRow;
	Evas_Object *ampm;

	Evas_Object *secondBarTop;
	Evas_Object *secondBarBottom;

	Evas_Object *hrTime;
	Evas_Object *battery;
	Evas_Object *steps;
	Evas_Object *date;

	int width;
	int height;

} watchfacedata_s;


void create_watch_face(watchfacedata_s *, int, int);
void update_watch_face(watchfacedata_s *, watch_time_h, int);

static void view_set_second(watchfacedata_s *, int );

static void formatLine( char *, char *, bool);
#endif
