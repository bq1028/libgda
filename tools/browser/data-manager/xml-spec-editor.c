/*
 * Copyright (C) 2010 The GNOME Foundation.
 *
 * AUTHORS:
 *      Vivien Malerba <malerba@gnome-db.org>
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib/gi18n-lib.h>
#include <string.h>
#include "xml-spec-editor.h"
#include "data-source.h"
#include <libgda/libgda.h>
#include "../support.h"

#ifdef HAVE_GTKSOURCEVIEW
#ifdef GTK_DISABLE_SINGLE_INCLUDES
#undef GTK_DISABLE_SINGLE_INCLUDES
#endif

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include <gtksourceview/gtksourcestylescheme.h>
#endif

struct _XmlSpecEditorPrivate {
	DataSourceManager *mgr;
	
	/* warnings */
	GtkWidget  *info;
	GtkWidget  *info_label;

	/* XML view */
	gboolean xml_view_up_to_date;
	guint signal_editor_changed_id; /* timout ID to signal editor changed */
	GtkWidget *text;
	GtkTextBuffer *buffer;
	GtkWidget *help;
};

static void xml_spec_editor_class_init (XmlSpecEditorClass *klass);
static void xml_spec_editor_init       (XmlSpecEditor *sped, XmlSpecEditorClass *klass);
static void xml_spec_editor_dispose    (GObject *object);

static GObjectClass *parent_class = NULL;

/*
 * XmlSpecEditor class implementation
 */
static void
xml_spec_editor_class_init (XmlSpecEditorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = xml_spec_editor_dispose;
}


static void
xml_spec_editor_init (XmlSpecEditor *sped, XmlSpecEditorClass *klass)
{
	g_return_if_fail (IS_XML_SPEC_EDITOR (sped));

	/* allocate private structure */
	sped->priv = g_new0 (XmlSpecEditorPrivate, 1);
	sped->priv->signal_editor_changed_id = 0;
}

GType
xml_spec_editor_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		static const GTypeInfo info = {
			sizeof (XmlSpecEditorClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xml_spec_editor_class_init,
			NULL,
			NULL,
			sizeof (XmlSpecEditor),
			0,
			(GInstanceInitFunc) xml_spec_editor_init
		};
		type = g_type_register_static (GTK_TYPE_VBOX, "XmlSpecEditor", &info, 0);
	}
	return type;
}

static void
xml_spec_editor_dispose (GObject *object)
{
	XmlSpecEditor *sped = (XmlSpecEditor*) object;
	if (sped->priv) {
		if (sped->priv->signal_editor_changed_id)
			g_source_remove (sped->priv->signal_editor_changed_id);
		if (sped->priv->mgr)
			g_object_unref (sped->priv->mgr);

		g_free (sped->priv);
		sped->priv = NULL;
	}
	parent_class->dispose (object);
}

static gboolean
signal_editor_changed (XmlSpecEditor *sped)
{
	/* modify the DataSourceManager */
	data_source_manager_remove_all (sped->priv->mgr);

	/* create new DataSource objects */
	GError *lerror = NULL;
	gchar *xml;
	xmlDocPtr doc = NULL;
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter (sped->priv->buffer, &start);
	gtk_text_buffer_get_end_iter (sped->priv->buffer, &end);
	xml = gtk_text_buffer_get_text (sped->priv->buffer, &start, &end, FALSE);
	if (xml) {
		doc = xmlParseDoc (BAD_CAST xml);
		g_free (xml);
	}

	if (!doc) {
		TO_IMPLEMENT;
		g_set_error (&lerror, 0, 0,
			     _("Error parsing XML specifications"));
		goto out;
	}

	xmlNodePtr node;
	node = xmlDocGetRootElement (doc);
	if (!node) {
		/* nothing to do => finished */
		xmlFreeDoc (doc);
		goto out;
	}

	BrowserConnection *bcnc;
	GdaSet *params;
	
	params = data_source_manager_get_params (sped->priv->mgr);
	bcnc = data_source_manager_get_browser_cnc (sped->priv->mgr);
	for (node = node->children; node; node = node->next) {
		if (!strcmp ((gchar*) node->name, "table") ||
		    !strcmp ((gchar*) node->name, "query")) {
			DataSource *source;
			source = data_source_new_from_xml_node (bcnc, node, &lerror);
			if (!source) {
				data_source_manager_remove_all (sped->priv->mgr);
				TO_IMPLEMENT;
				goto out;
			}
			
			data_source_set_params (source, params);
			data_source_manager_add_source (sped->priv->mgr, source);
			g_object_unref (source);
		}
	}
	xmlFreeDoc (doc);

 out:
	if (lerror) {
		if (! sped->priv->info) {
#if GTK_CHECK_VERSION (2,18,0)
			sped->priv->info = gtk_info_bar_new ();
			gtk_box_pack_start (GTK_BOX (sped), sped->priv->info, FALSE, FALSE, 0);
			sped->priv->info_label = gtk_label_new ("");
			gtk_misc_set_alignment (GTK_MISC (sped->priv->info_label), 0., -1);
			gtk_label_set_ellipsize (GTK_LABEL (sped->priv->info_label), PANGO_ELLIPSIZE_END);
			gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (sped->priv->info))),
					   sped->priv->info_label);
#else
			sped->priv->info = gtk_label_new ("");
			sped->priv->info_label = sped->priv->info;
#endif
		}
		gchar *str;
		str = g_strdup_printf (_("Error: %s"), lerror->message);
		g_clear_error (&lerror);
		gtk_label_set_text (GTK_LABEL (sped->priv->info_label), str);
		g_free (str);
	}
	else if (sped->priv->info) {
		gtk_widget_hide (sped->priv->info);
	}

	/* remove timeout */
	sped->priv->signal_editor_changed_id = 0;
	return FALSE;
}

static void
editor_changed_cb (GtkTextBuffer *buffer, XmlSpecEditor *sped)
{
	if (sped->priv->signal_editor_changed_id)
		g_source_remove (sped->priv->signal_editor_changed_id);
	sped->priv->signal_editor_changed_id = g_timeout_add_seconds (1, (GSourceFunc) signal_editor_changed, sped);
}

/**
 * xml_spec_editor_new
 *
 * Returns: the newly created editor.
 */
GtkWidget *
xml_spec_editor_new (DataSourceManager *mgr)
{
	XmlSpecEditor *sped;
	GtkWidget *sw, *label;
	gchar *str;

	g_return_val_if_fail (IS_DATA_SOURCE_MANAGER (mgr), NULL);

	sped = g_object_new (XML_SPEC_EDITOR_TYPE, NULL);
	sped->priv->mgr = g_object_ref (mgr);

	/* XML editor */
	label = gtk_label_new ("");
	str = g_strdup_printf ("<b>%s</b>", _("SQL code to execute:"));
	gtk_label_set_markup (GTK_LABEL (label), str);
	g_free (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0., -1);
	gtk_box_pack_start (GTK_BOX (sped), label, FALSE, FALSE, 0);

	sw = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_OUT);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	gtk_box_pack_start (GTK_BOX (sped), sw, TRUE, TRUE, 0);

#ifdef HAVE_GTKSOURCEVIEW
        sped->priv->text = gtk_source_view_new ();
        gtk_source_buffer_set_highlight_syntax (GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (sped->priv->text))),
						TRUE);

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (sped->priv->text))),
					gtk_source_language_manager_get_language (gtk_source_language_manager_get_default (),
										  "xml"));
#else
        sped->priv->text = gtk_text_view_new ();
#endif
	gtk_container_add (GTK_CONTAINER (sw), sped->priv->text);	
	sped->priv->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (sped->priv->text));

	g_signal_connect (sped->priv->buffer, "changed",
			  G_CALLBACK (editor_changed_cb), sped);
	gtk_widget_show_all (sw);

	/* warning, not shown */
	sped->priv->info = NULL;

	return (GtkWidget*) sped;
}

/**
 * xml_spec_editor_set_xml_text
 */
void
xml_spec_editor_set_xml_text (XmlSpecEditor *sped, const gchar *xml)
{
	g_return_if_fail (IS_XML_SPEC_EDITOR (sped));

	g_signal_handlers_block_by_func (sped->priv->buffer,
					 G_CALLBACK (editor_changed_cb), sped);
	gtk_text_buffer_set_text (sped->priv->buffer, xml, -1);
	signal_editor_changed (sped);
	g_signal_handlers_unblock_by_func (sped->priv->buffer,
					   G_CALLBACK (editor_changed_cb), sped);
}

/**
 * xml_spec_editor_get_xml_text
 */
gchar *
xml_spec_editor_get_xml_text (XmlSpecEditor *sped)
{
	GtkTextIter start, end;
	g_return_val_if_fail (IS_XML_SPEC_EDITOR (sped), NULL);
	gtk_text_buffer_get_start_iter (sped->priv->buffer, &start);
	gtk_text_buffer_get_end_iter (sped->priv->buffer, &end);

	return gtk_text_buffer_get_text (sped->priv->buffer, &start, &end, FALSE);
}