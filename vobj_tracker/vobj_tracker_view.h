/*  This file is part of Polyora, a multi-target tracking library.
    Copyright (C) 2010 Julien Pilet

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <http://www.gnu.org/licenses/>.

    To contact the author of this program, please send an e-mail to:
    julien.pilet(at)calodox.org
*/
#ifndef VSVIEW_H
#define VSVIEW_H

#include <videosource.h>
#include <opencv/cv.h>
#include <qapplication.h>

#include <QKeyEvent>
#include <QTimerEvent>
#include <QTime>

#include "glbox.h"
#include "ipltexture.h"
#include <polyora/polyora.h>

#include "../script/script.h"

class vobj_tracker_view : public GLBox {
Q_OBJECT

public:
	vobj_tracker_view(QWidget *parent, const char *name, VideoSource *vs);
	~vobj_tracker_view();
	VideoSource *vs;

	void saveCurrentFrame(const char *filename);
	
protected slots:
	/*! One iteration of the main loop, processes a frame */
	void timerEvent( QTimerEvent *);

	/*! reacts to keyboard events */
	void keyPressEvent(QKeyEvent *k);
	void mousePressEvent ( QMouseEvent * event );
	void mouseReleaseEvent ( QMouseEvent * event );
	void mouseDoubleClickEvent ( QMouseEvent * event );

	/*! display everything in an opengl viewport */
	void paintGL();

	void show_help();

private:
	// idle timer
	int timer;
	bool filter;
	bool viewScores;

public:
	bool record;
	bool record_movie;
	bool auto_index;
	bool learning;

	bool draw_color_flag, draw_matches_flag, draw_instance_flag;
	enum {
		DRAW_COLOR = 1,
		DRAW_MATCHES = 2,
		DRAW_INSTANCE = 4,
		DRAW_TRACKS = 8,
		DRAW_DARK =16,
		DRAW_KEYPOINTS=32,
		DRAW_SCRIPT=64,
	};
	unsigned draw_flags;
private:

	int nbLev;

	IplImage *im, *im2;

	vobj_tracker *tracker;
	visual_database database;

	// fps counter
	QTime qtime;
	int frameCnt;
	int frameno;


public:
	const char *tree_fn, *clusters_fn, *descriptors_fn;
	const char *visual_db_fn;
	float threshold;
	int query_flags;
private:

	std::list<IplTexture> icon_texture_used, icon_texture_available;

	IplTexture entry_tex;
	QTime add_frame_time;

	int nb_missing_frames;
	pyr_keypoint *selected_kpt;

	bool selectKeypoint (QMouseEvent *event);
	void show_track(pyr_keypoint *k);
	void show_tracks();
	void createTracker();
	void draw_keypoints(pyr_frame *frame);
	void draw_all_tracks(pyr_frame *frame);
	void draw_selected_track();
	void draw_icon(point2d *c, CvArr *image, float w, float h, float max_width, int margin_x=0, int margin_y=0);
	void draw_entry_image(visual_object *entry);
	void draw_matches(vobj_frame *frame);
	void add_current_frame_to_db(const char *name, int flags);

	typedef std::map<unsigned, std::pair<double, unsigned> > fps_stat_map;
	fps_stat_map fps_stat;
	void update_fps_stat(float fr_ms, pyr_frame *pframe);

	void summary();
	void homography_augment(visual_object *obj, float H[3][3]);
	void draw_instances(vobj_frame *frame);
	GLuint get_texture_for_obj(visual_object *obj);

	QWidget *help_window;

	Script script_engine;
	QTime time_since_started;
};

#endif

