/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2001 Hiroyuki Yamamoto
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

#ifndef __SSL_CERTWINDOW_H__
#define __SSL_CERTWINDOW_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if USE_SSL

#include <openssl/ssl.h>
#include <openssl/objects.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "../ssl_certificate.h"

GtkWidget *cert_presenter(SSLCertificate *cert);
void sslcertwindow_show_cert(SSLCertificate *cert);
gboolean sslcertwindow_ask_new_cert(SSLCertificate *cert);
gboolean sslcertwindow_ask_changed_cert(SSLCertificate *old_cert, SSLCertificate *new_cert);

#endif /* USE_SSL */
#endif /* __SSL_CERTWINDOW_H__ */
