/*
 * Copyright (C) 2016 Adrien Plazas.
 *
 * Contact: Adrien Plazas <kekun.plazas@laposte.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "totem-episode-view.h"

#include <net/grl-net.h>
#include <string.h>

typedef struct _TotemEpisodeViewPrivate
{
  GrlMedia *media;

  GtkLabel *episode_number_label;
  GtkLabel *episode_title_label;

  GtkButton *episode_viewed_button;

  GtkComboBoxText *subtitles_combo;

  GtkButton *watch_now_button;

  GtkRevealer *revealer;
} TotemEpisodeViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (TotemEpisodeView, totem_episode_view, GTK_TYPE_BOX);

/* -------------------------------------------------------------------------- *
 * Internal / Helpers
 * -------------------------------------------------------------------------- */

static void
totem_episode_view_update (TotemEpisodeView *self)
{
  GrlMedia *media;
  gint episode_number;
  gchar *episode_number_string;
  const gchar *episode_title;

  media = self->priv->media;

  episode_number = grl_media_get_episode (media);
  episode_number_string = g_strdup_printf ("%d", episode_number);
  gtk_label_set_text (self->priv->episode_number_label, episode_number_string);
  g_free (episode_number_string);

  episode_title = grl_media_get_episode_title (media);

  if (episode_title != NULL && g_strcmp0 (episode_title, "") != 0) {
    gtk_label_set_text (self->priv->episode_title_label, episode_title);
  }
  else {
    gtk_label_set_text (self->priv->episode_title_label, "Unknown Title");
  }
}

/* -------------------------------------------------------------------------- *
 * External
 * -------------------------------------------------------------------------- */

TotemEpisodeView *
totem_episode_view_new (GrlMedia *media)
{
  TotemEpisodeView *self;

  g_return_val_if_fail (media != NULL, NULL);

  self = g_object_new (TOTEM_TYPE_EPISODE_VIEW, NULL);
  self->priv->media = g_object_ref (media);

  totem_episode_view_update (self);

  return self;
}

/* -------------------------------------------------------------------------- *
 * Object
 * -------------------------------------------------------------------------- */

static void
totem_episode_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (totem_episode_view_parent_class)->finalize (object);
}

static void
totem_episode_view_init (TotemEpisodeView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->priv = totem_episode_view_get_instance_private (self);

  self->priv->media = NULL;
}

static void
totem_episode_view_class_init (TotemEpisodeViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->finalize = totem_episode_view_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/totem/grilo/totem-episode-view.ui");
  gtk_widget_class_bind_template_child_private (widget_class, TotemEpisodeView, episode_number_label);
  gtk_widget_class_bind_template_child_private (widget_class, TotemEpisodeView, episode_title_label);
  gtk_widget_class_bind_template_child_private (widget_class, TotemEpisodeView, episode_viewed_button);
  gtk_widget_class_bind_template_child_private (widget_class, TotemEpisodeView, subtitles_combo);
  gtk_widget_class_bind_template_child_private (widget_class, TotemEpisodeView, watch_now_button);
  gtk_widget_class_bind_template_child_private (widget_class, TotemEpisodeView, revealer);
}
