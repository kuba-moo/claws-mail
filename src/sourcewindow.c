/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "defs.h"

#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktext.h>
#include <gtk/gtkstyle.h>
#include <stdio.h>
#include <stdlib.h>

#include "intl.h"
#include "sourcewindow.h"
#include "utils.h"
#include "gtkutils.h"
#include "prefs_common.h"

static void source_window_size_alloc_cb	(GtkWidget	*widget,
					 GtkAllocation	*allocation);
static void source_window_destroy_cb	(GtkWidget	*widget,
					 SourceWindow	*sourcewin);
static gboolean key_pressed		(GtkWidget	*widget,
					 GdkEventKey	*event,
					 SourceWindow	*sourcewin);

static void source_window_init()
{
}

SourceWindow *source_window_create(void)
{
	SourceWindow *sourcewin;
	GtkWidget *window;
	GtkWidget *scrolledwin;
	GtkWidget *text;
	static PangoFontDescription *font_desc = NULL;

	debug_print("Creating source window...\n");
	sourcewin = g_new0(SourceWindow, 1);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Source of the message"));
	gtk_window_set_wmclass(GTK_WINDOW(window), "source_window", "Sylpheed");
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
	gtk_widget_set_size_request(window, prefs_common.sourcewin_width,
				    prefs_common.sourcewin_height);
	g_signal_connect(G_OBJECT(window), "size_allocate",
			 G_CALLBACK(source_window_size_alloc_cb),
			 sourcewin);
	g_signal_connect(G_OBJECT(window), "destroy",
			 G_CALLBACK(source_window_destroy_cb),
			 sourcewin);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			 G_CALLBACK(key_pressed), sourcewin);
	gtk_widget_realize(window);

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(window), scrolledwin);
	gtk_widget_show(scrolledwin);

	text = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
	if (!font_desc && prefs_common.textfont)
		font_desc = pango_font_description_from_string
					(prefs_common.textfont);
	if (font_desc)
		gtk_widget_modify_font(text, font_desc);
	gtk_container_add(GTK_CONTAINER(scrolledwin), text);
	gtk_widget_show(text);

	sourcewin->window = window;
	sourcewin->scrolledwin = scrolledwin;
	sourcewin->text = text;

	source_window_init();

	return sourcewin;
}

void source_window_show(SourceWindow *sourcewin)
{
	gtk_widget_show_all(sourcewin->window);
}

void source_window_destroy(SourceWindow *sourcewin)
{
	g_free(sourcewin);
}

void source_window_show_msg(SourceWindow *sourcewin, MsgInfo *msginfo)
{
	gchar *file;
	gchar *title;
	FILE *fp;
	gchar buf[BUFFSIZE];

	g_return_if_fail(msginfo != NULL);

	file = procmsg_get_message_file(msginfo);
	g_return_if_fail(file != NULL);

	if ((fp = fopen(file, "rb")) == NULL) {
		FILE_OP_ERROR(file, "fopen");
		g_free(file);
		return;
	}

	debug_print("Displaying the source of %s ...\n", file);

	title = g_strdup_printf(_("%s - Source"), file);
	gtk_window_set_title(GTK_WINDOW(sourcewin->window), title);
	g_free(title);
	g_free(file);

	while (fgets(buf, sizeof(buf), fp) != NULL)
		source_window_append(sourcewin, buf);

	fclose(fp);
}

void source_window_append(SourceWindow *sourcewin, const gchar *str)
{
	GtkTextView *text = GTK_TEXT_VIEW(sourcewin->text);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(text);
	GtkTextIter iter;
	gchar *out;
	gint len;

	len = strlen(str) + 1;
	Xalloca(out, len, return);
	
	conv_localetodisp(out, len, str);
	if (!g_utf8_validate(out, -1, NULL)) {
		gchar *buf;
		gint buflen;
		const gchar *src_codeset, *dest_codeset;
		src_codeset = conv_get_current_charset_str();
		dest_codeset = CS_UTF_8;
		buf = conv_codeset_strdup(out, src_codeset, dest_codeset);
		gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
		gtk_text_buffer_insert(buffer, &iter, buf, -1);
		g_free(buf);
	} else {
		gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
		gtk_text_buffer_insert(buffer, &iter, out, -1);
	}
}

static void source_window_size_alloc_cb(GtkWidget *widget,
					GtkAllocation *allocation)
{
	g_return_if_fail(allocation != NULL);

	prefs_common.sourcewin_width  = allocation->width;
	prefs_common.sourcewin_height = allocation->height;
}

static void source_window_destroy_cb(GtkWidget *widget,
				     SourceWindow *sourcewin)
{
	source_window_destroy(sourcewin);
}

static gboolean key_pressed(GtkWidget *widget, GdkEventKey *event,
			    SourceWindow *sourcewin)
{

	if (!event || !sourcewin) return FALSE;
	
	switch (event->keyval) {
	case GDK_A:
	case GDK_a:
		if ((event->state & GDK_CONTROL_MASK) != 0)
			gtk_editable_select_region(GTK_EDITABLE(sourcewin->text), 0, -1);
		break;
	case GDK_W:
	case GDK_w:
		if ((event->state & GDK_CONTROL_MASK) != 0)
			gtk_widget_destroy(sourcewin->window);
		break;
	case GDK_Escape:
		gtk_widget_destroy(sourcewin->window);
		break;
	}

	return FALSE;
}
