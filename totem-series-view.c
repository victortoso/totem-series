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
  GHashTable *episodes;
  GList *seasons;
  gchar *show_name;
  GList *current_season;

  GtkButton *next_season;
  GtkButton *previous_season;
  GtkLabel *description_label;
  GtkLabel *cast_label;
  GtkLabel *director_label;
  GtkLabel *writers_label;
  GtkLabel *season_title;
  GtkStack *episodes_stack;
} TotemSeriesViewPrivate;

typedef struct _TotemSeasonSpec
{
  gint season_number;
  GtkWidget *season_view;
  GPtrArray *videos; /* weak reference */
} TotemSeasonSpec;

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

/*
 * Update the Series View based in the metadata of given @video
 */
static void
totem_series_view_update (TotemSeriesView *self,
                          GrlMedia        *video)
{
  const gchar *description;
  const gchar *cast;
  const gchar *director;
  const gchar *writers;
  gchar *season_title_string;
  gint season_number;

  g_return_if_fail (video != NULL);

  if (!g_list_next (self->priv->current_season)) {
      gtk_widget_hide (GTK_WIDGET (self->priv->next_season));
  } else {
      gtk_widget_show (GTK_WIDGET (self->priv->next_season));
  }

  if (!g_list_previous (self->priv->current_season)) {
      gtk_widget_hide (GTK_WIDGET (self->priv->previous_season));
  } else {
      gtk_widget_show (GTK_WIDGET (self->priv->previous_season));
  }

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

  /* FIXME: this is asserting in glib after I press next. GDateTime seems fine,
   * wtf:
   * GLib:ERROR:gdatetime.c:1539:g_date_time_get_ymd: assertion failed: (0 <= remaining_days)
  GDateTime *date;
  gchar *date_str;
  date = grl_media_get_publication_date(video);
  date_str = (date != NULL) ? g_date_time_format (date, " (%Y)") : g_strdup("");
  g_clear_pointer (&date, g_date_time_unref);
  g_clear_pointer (&date_str, g_free);
  */

  season_number = grl_media_get_season(video);
  season_title_string = g_strdup_printf ("Season %d", season_number);
  gtk_label_set_text (self->priv->season_title, season_title_string);
  g_free (season_title_string);
}

static void
totem_series_view_season_spec_add_video (TotemSeasonSpec *ss,
                                         GrlMedia *video)
{
  g_return_if_fail (ss != NULL);
  g_return_if_fail (video != NULL);
  g_ptr_array_add (ss->videos, (gpointer) video);
}

static void
totem_series_view_free_season_spec (gpointer data)
{
  TotemSeasonSpec *ss;
  if (data == NULL)
    return;

  ss = data;
  g_clear_pointer (&ss->videos, g_ptr_array_unref);
  g_free (ss);
}

static TotemSeasonSpec *
totem_series_view_get_season_spec (TotemSeriesView *self,
                                   gint             season_number)
{
  GList *it;

  g_return_val_if_fail (season_number >= 0, NULL);

  for (it = self->priv->seasons; it != NULL; it = it->next) {
    TotemSeasonSpec *ss = it->data;

    if (ss->season_number == season_number)
      return ss;
  }

  return NULL;
}

static gint
ascending_order (gconstpointer a, gconstpointer b)
{
  const TotemSeasonSpec *ss_a = a;
  const TotemSeasonSpec *ss_b = b;

  return (ss_a->season_number - ss_b->season_number);
}

static TotemSeasonSpec *
totem_series_view_new_season_spec (TotemSeriesView *self,
                                   gint             season_number)
{
  TotemSeasonSpec *ss;

  g_return_val_if_fail (season_number >= 0, NULL);

  ss = totem_series_view_get_season_spec (self, season_number);
  if (ss == NULL) {
    gchar *name;

    ss = g_new (TotemSeasonSpec, 1);
    ss->season_number = season_number;
    ss->videos = g_ptr_array_new ();
    ss->season_view = gtk_list_box_new ();
    gtk_widget_show (ss->season_view);
    g_debug ("new season spec: %d", season_number);

    self->priv->seasons = g_list_insert_sorted (self->priv->seasons, ss, ascending_order);

    name = g_strdup_printf ("Season: %02d", season_number);
    gtk_stack_add_named (self->priv->episodes_stack, ss->season_view, name);
    g_free (name);
  } else {
    g_debug ("got season spec: %d", season_number);
  }

  return ss;
}

static void
on_previous_season_clicked (GtkButton *button,
                            gpointer   user_data)
{
  TotemSeriesView *self;
  GList *prev;

  g_return_if_fail (TOTEM_IS_SERIES_VIEW (user_data));

  self = TOTEM_SERIES_VIEW (user_data);
  g_return_if_fail (self->priv->current_season != NULL);

  prev = g_list_previous (self->priv->current_season);
  if (prev != NULL) {
    TotemSeasonSpec *ss = prev->data;
    GrlMedia *video = g_ptr_array_index (ss->videos, 0);

    self->priv->current_season = prev;
    totem_series_view_update (self, video);
  }
}

static void
on_next_season_clicked (GtkButton *button,
                        gpointer   user_data)
{
  TotemSeriesView *self;
  GList *next;

  g_return_if_fail (TOTEM_IS_SERIES_VIEW (user_data));

  self = TOTEM_SERIES_VIEW (user_data);
  g_return_if_fail(self->priv->current_season != NULL);

  next = g_list_next(self->priv->current_season);
  if (next != NULL) {
    TotemSeasonSpec *ss = next->data;
    GrlMedia *video = g_ptr_array_index (ss->videos, 0);

    self->priv->current_season = next;
    totem_series_view_update (self, video);
  }
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

static void
series_view_add_episode (TotemSeriesView *self,
                         GrlMedia        *video)
{
  TotemSeasonSpec *ss;
  TotemEpisodeView *episode_view;

  ss = totem_series_view_new_season_spec (self, grl_media_get_season (video));
  g_assert_nonnull(ss);
  totem_series_view_season_spec_add_video (ss, video);

  episode_view = totem_episode_view_new (video);
  gtk_widget_show (GTK_WIDGET (episode_view));
  gtk_container_add (GTK_CONTAINER (ss->season_view), GTK_WIDGET (episode_view));

  g_hash_table_add (self->priv->episodes, g_object_ref (video));
}

gboolean
totem_series_view_add_video (TotemSeriesView *self,
                             GrlMedia        *video)
{
  const gchar *show;

  g_return_val_if_fail (video != NULL, FALSE);

  show = grl_media_get_show (video);
  g_return_val_if_fail (show != NULL, FALSE);

  if (self->priv->show_name == NULL) {
    /* First video */
    self->priv->show_name = g_strdup (show);
  } else if (g_strcmp0(self->priv->show_name, show) != 0) {
    g_warning ("Video belong to different show: '%s' instead of '%s'",
               show, self->priv->show_name);
    return FALSE;
  } else if (g_hash_table_contains (self->priv->episodes, video)) {
    g_warning ("Video already included on its series-view");
    return FALSE;
  }

  g_debug("# '%s' S%02dxE%02d", show, grl_media_get_season (video), grl_media_get_episode (video));

  series_view_add_episode (self, video);
  totem_series_view_update (self, video);

  if (self->priv->current_season == NULL)
    self->priv->current_season = self->priv->seasons;

  return TRUE;
}

/* -------------------------------------------------------------------------- *
 * Object
 * -------------------------------------------------------------------------- */

static void
totem_series_view_finalize (GObject *object)
{
  TotemSeriesViewPrivate *priv = TOTEM_SERIES_VIEW (object)->priv;

  if (priv->seasons) {
    g_list_free_full (priv->seasons, totem_series_view_free_season_spec);
    priv->seasons = NULL;
  }

  g_clear_pointer (&priv->episodes, g_hash_table_unref);
  g_free (priv->show_name);

  G_OBJECT_CLASS (totem_series_view_parent_class)->finalize (object);
}

static void
totem_series_view_init (TotemSeriesView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->priv = totem_series_view_get_instance_private (self);

  self->priv->episodes = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                g_object_unref, NULL);
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
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, episodes_stack);
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, next_season);
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesView, previous_season);

  gtk_widget_class_bind_template_callback (widget_class, on_previous_season_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_next_season_clicked);
}
