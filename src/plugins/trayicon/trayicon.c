/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "common/sylpheed.h"
#include "common/version.h"
#include "plugin.h"
#include "utils.h"
#include "hooks.h"
#include "folder.h"
#include "mainwindow.h"
#include "gtkutils.h"
#include "intl.h"

#include "eggtrayicon.h"
#include "newmail.xpm"
#include "unreadmail.xpm"
#include "nomail.xpm"

static guint hook_id;

static GdkPixmap *newmail_pixmap;
static GdkPixmap *newmail_bitmap;
static GdkPixmap *unreadmail_pixmap;
static GdkPixmap *unreadmail_bitmap;
static GdkPixmap *nomail_pixmap;
static GdkPixmap *nomail_bitmap;

static EggTrayIcon *trayicon;
static GtkWidget *eventbox;
static GtkWidget *image;
static GtkTooltips *tooltips;
guint destroy_signal_id;

typedef enum
{
	TRAYICON_NEW,
	TRAYICON_UNREAD,
	TRAYICON_UNREADMARKED,
	TRAYICON_NOTHING,
} TrayIconType;

/* static gboolean mainwin_hidden = FALSE; */

static void set_trayicon_pixmap(TrayIconType icontype)
{
	GdkPixmap *pixmap = NULL;
	GdkBitmap *bitmap = NULL;

	switch(icontype) {
	case TRAYICON_NEW:
		pixmap = newmail_pixmap;
		bitmap = newmail_bitmap;
		break;
	case TRAYICON_UNREAD:
	case TRAYICON_UNREADMARKED:
		pixmap = unreadmail_pixmap;
		bitmap = unreadmail_bitmap;
		break;
	default:
		pixmap = nomail_pixmap;
		bitmap = nomail_bitmap;
		break;
	}

	gtk_image_set_from_pixmap(GTK_IMAGE(image), pixmap, bitmap);
	gtk_widget_shape_combine_mask(GTK_WIDGET(trayicon), bitmap, GTK_WIDGET(image)->allocation.x, GTK_WIDGET(image)->allocation.y);
}

static void update(void)
{
	gint new, unread, unreadmarked, total;
	gchar *buf;

	folder_count_total_msgs(&new, &unread, &unreadmarked, &total);
	buf = g_strdup_printf("New %d, Unread: %d, Total: %d", new, unread, total);

        gtk_tooltips_set_tip(tooltips, eventbox, buf, "");
	g_free(buf);

	set_trayicon_pixmap(new > 0 ? TRAYICON_NEW : (unread > 0 ? TRAYICON_UNREAD : TRAYICON_NOTHING));
}

static gboolean folder_item_update_hook(gpointer source, gpointer data)
{
	update();

	return FALSE;
}

static gboolean click_cb(GtkWidget * widget,
		         GdkEventButton * event, gpointer user_data)
{
	MainWindow *mainwin;

	mainwin = mainwindow_get_mainwindow();
	if (GTK_WIDGET_VISIBLE(GTK_WIDGET(mainwin->window))) {
		main_window_hide(mainwin);
	} else {
		main_window_show(mainwin);
        }
	return TRUE;
}

static void create_trayicon(void);

static void destroy_cb(GtkWidget *widget, gpointer *data)
{
	debug_print("Widget destroyed\n");

	create_trayicon();
}

static void create_trayicon()
{
#if 0
	GtkPacker *packer;
#endif

        trayicon = egg_tray_icon_new("Sylpheed-Claws");
//        trayicon = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(GTK_WIDGET(trayicon));
	gtk_window_set_default_size(GTK_WINDOW(trayicon), 16, 16);
        gtk_container_set_border_width(GTK_CONTAINER(trayicon), 0);

        PIXMAP_CREATE(GTK_WIDGET(trayicon), nomail_pixmap, nomail_bitmap, nomail_xpm);
        PIXMAP_CREATE(GTK_WIDGET(trayicon), unreadmail_pixmap, unreadmail_bitmap, unreadmail_xpm);
        PIXMAP_CREATE(GTK_WIDGET(trayicon), newmail_pixmap, newmail_bitmap, newmail_xpm);

        eventbox = gtk_event_box_new();
        gtk_container_set_border_width(GTK_CONTAINER(eventbox), 0);
        gtk_container_add(GTK_CONTAINER(trayicon), GTK_WIDGET(eventbox));

#warning FIXME_GTK2
#if 0
	packer = GTK_PACKER(gtk_packer_new());
        gtk_container_add(GTK_CONTAINER(eventbox), GTK_WIDGET(packer));
        gtk_container_set_border_width(GTK_CONTAINER(packer), 0);

        image = gtk_image_new_from_pixmap(nomail_pixmap, nomail_bitmap);
        gtk_packer_add_defaults(GTK_PACKER(packer), GTK_WIDGET(image), GTK_SIDE_TOP, GTK_ANCHOR_CENTER, GTK_PACK_EXPAND);
#else
        image = gtk_image_new_from_pixmap(nomail_pixmap, nomail_bitmap);
        gtk_container_add(GTK_CONTAINER(eventbox), image);
#endif

	destroy_signal_id =
	g_signal_connect(G_OBJECT(trayicon), "destroy",
                     	 G_CALLBACK(destroy_cb), NULL);
	g_signal_connect(G_OBJECT(eventbox), "button-press-event",
		    	 G_CALLBACK(click_cb), NULL);

        tooltips = gtk_tooltips_new();
        gtk_tooltips_set_delay(tooltips, 1000);
        gtk_tooltips_enable(tooltips);

        gtk_widget_show_all(GTK_WIDGET(trayicon));

	update();
}

int plugin_init(gchar **error)
{
	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your sylpheed version is newer than the version the plugin was built with");
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 3, 86))) {
		*error = g_strdup("Your sylpheed version is too old");
		return -1;
	}

	hook_id = hooks_register_hook (FOLDER_ITEM_UPDATE_HOOKLIST, folder_item_update_hook, NULL);
	if (hook_id == -1) {
		*error = g_strdup("Failed to register folder item update hook");
		return -1;
	}

	create_trayicon();

        return 0;
}

void plugin_done(void)
{
	g_signal_handler_disconnect(G_OBJECT(trayicon), destroy_signal_id);

	gtk_widget_destroy(GTK_WIDGET(trayicon));
	hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_id);

	while (gtk_events_pending())
		gtk_main_iteration();		
}

const gchar *plugin_name(void)
{
	return _("Trayicon");
}

const gchar *plugin_desc(void)
{
	return _("This plugin places a mailbox icon in the system tray that "
	         "indicates if you have new or unread mail.\n"
	         "\n"
	         "The mailbox is empty if you have no unread mail, otherwise "
	         "it contains a letter. A tooltip shows new, unread and total "
	         "number of messages.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}
