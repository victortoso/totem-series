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

#include "totem-series-view.h"

#include <net/grl-net.h>
#include <string.h>

#include "totem-episode-view.h"

typedef struct _TotemSeriesViewPrivate
{
  GPtrArray *videos;
  GHashTable *seasons;

  GtkLabel *description_label;
  GtkLabel *cast_label;
  GtkLabel *director_label;
  GtkLabel *writers_label;
  GtkLabel *season_title;
  GtkStack *episodes;
} TotemSeriesViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (TotemSeriesView, totem_series_view, GTK_TYPE_BIN);

/* -------------------------------------------------------------------------- *
 * Internal / Helpers
 * -------------------------------------------------------------------------- */

/* For GrlKeys that have several values, return all of them in one
 * string separated by comma; */
static gchar *
get_data_from_media (GrlData *data,
                     GrlKeyID key)
{
  gint i, len;
  GString *s;

  len = grl_data_length (data, key);
  if (len <= 0)
    return NULL;

  s = g_string_new ("");
  for (i = 0; i < len; i++) {
    GrlRelatedKeys *relkeys;
    const gchar *element;

    relkeys = grl_data_get_related_keys (data, key, i);
    element = grl_related_keys_get_string (relkeys, key);

    if (i > 0)
      g_string_append (s, ", ");
    g_string_append (s, element);
  }
  return g_string_free (s, FALSE);
}

static void
totem_series_view_set_description (TotemSeriesView *self,
                                   const gchar     *description)
{
  gchar **lines;

  lines = g_strsplit (description, "\n", 2);
  gtk_label_set_text (self->priv->description_label, lines[0]);
  g_strfreev (lines);
}

static void
totem_series_view_set_cast (TotemSeriesView *self,
                            const gchar     *cast)
{
  gtk_label_set_text (self->priv->cast_label, cast);
}

static void
totem_series_view_set_director (TotemSeriesView *self,
                                const gchar     *director)
{
  gtk_label_set_text (self->priv->director_label, director);
}

static void
totem_series_view_set_writers (TotemSeriesView *self,
                               const gchar     *writers)
{
  gtk_label_set_text (self->priv->writers_label, writers);
}

static void
totem_series_view_update (TotemSeriesView *self)
{
  const GPtrArray *videos;
  GrlMedia *video;
  const gchar *description;
  const gchar *cast;
  const gchar *director;
  const gchar *writers;
  gchar *season_title_string;
  gint season_number;
  gint season_year;

  videos = self->priv->videos;

  g_return_if_fail (videos != NULL);
  g_return_if_fail (videos->len > 0);

  video = g_ptr_array_index (videos, 0);
  g_return_if_fail (video != NULL);

  description = grl_media_get_description (video);
  if (description == NULL)
    description = "";
  totem_series_view_set_description (self, description);

  cast = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_PERFORMER);
  if (cast == NULL)
    cast = "";
  totem_series_view_set_cast (self, cast);

  director = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_DIRECTOR);
  if (director == NULL)
    director = "";
  totem_series_view_set_director (self, director);

  writers = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_AUTHOR);
  if (writers == NULL)
    writers = "";
  totem_series_view_set_writers (self, writers);

  season_number = 0; // TODO
  season_year = 0; // TODO
  season_title_string = g_strdup_printf ("Season %d (%d)", season_number, season_year);
  gtk_label_set_text (self->priv->season_title, season_title_string);
  g_free (season_title_string);
}

/* -------------------------------------------------------------------------- *
 * External
 * -------------------------------------------------------------------------- */

TotemSeriesView *
totem_series_view_new (void)
{
  TotemSeriesView *self;

  self = g_object_new (TOTEM_TYPE_SERIES_VIEW, NULL);

  return self;
}

gboolean
totem_series_view_add_video (TotemSeriesView *self,
                             GrlMedia        *video)
{
  gintptr season_number;
  gchar *season_number_string;
  GtkWidget *season_view;
  TotemEpisodeView *episode_view;

  // TODO If the series isn't the same, don't add the new video

  season_number = (gintptr) grl_media_get_season (video);
  if (g_hash_table_contains (self->priv->seasons, (gpointer) season_number))
    season_view = g_hash_table_lookup (self->priv->seasons, (gpointer) season_number);
  else {
    season_view = gtk_list_box_new ();
    gtk_widget_show (season_view);
    g_hash_table_insert (self->priv->seasons, (gpointer) season_number, season_view);


    season_number_string = g_strdup_printf ("%ld", season_number);
    gtk_stack_add_named (self->priv->episodes, season_view, season_number_string);
    g_free (season_number_string);
  }

  // TODO if the episode is already contained, do not add it (?)

  episode_view = totem_episode_view_new ();
  totem_episode_view_set_media (episode_view, video);
  gtk_widget_show (GTK_WIDGET (episode_view));
  gtk_container_add (GTK_CONTAINER (season_view), GTK_WIDGET (episode_view));

  g_ptr_array_add (self->priv->videos, g_object_ref (video));

  totem_series_view_update (self);

  return TRUE;
}

/* -------------------------------------------------------------------------- *
 * Object
 * -------------------------------------------------------------------------- */

static void
totem_series_view_finalize (GObject *object)
{
  TotemSeriesViewPrivate *priv = TOTEM_SERIES_VIEW (object)->priv;

  if (priv->seasons != NULL) {
    g_hash_table_unref (priv->seasons);
    priv->seasons = NULL;
  }

  G_OBJECT_CLASS (totem_series_view_parent_class)->finalize (object);
}

static void
totem_series_view_init (TotemSeriesView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->priv = totem_series_view_get_instance_private (self);

  self->priv->videos = g_ptr_array_new_with_free_func (g_object_unref);
  self->priv->seasons = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
totem_series_view_class_init (TotemSeriesViewClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->finalize = totem_series_view_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/totem/grilo/totem-series-view.ui");
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, description_label);
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, cast_label);
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, director_label);
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, writers_label);
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, season_title);
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, episodes);
}
