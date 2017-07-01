/*
 * Copyright (C) 2015 Victor Toso.
 *
 * Contact: Victor Toso <me@victortoso.com>
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

#include "totem-series-summary.h"

#include <net/grl-net.h>
#include <string.h>

#include "totem-series-view.h"

typedef struct _TotemSeriesSummaryPrivate
{
  GrlRegistry *registry;
  GrlSource *tvdb_source;
  GrlSource *video_title_parsing_source;
  GrlSource *opensubtitles_source;
  GrlKeyID tvdb_poster_key;
  GrlKeyID subtitles_lang_key;
  GrlKeyID subtitles_url_key;

  TotemSeriesView *view;

  /* List for pending OperationSpecs */
  GList *pending_ops;
} TotemSeriesSummaryPrivate;

/* For every added GrlVideo, we will need to check and request metadata related
 * to this video. The OperationSpec will be alive while there is any pending
 * request to Grilo. */
typedef struct
{
  TotemSeriesSummary *totem_series_summary;
  GrlMedia           *video;

  gchar    *poster_path;

  GList            *pending_grl_ops;
} OperationSpec;

#define POSTER_WIDTH  266
#define POSTER_HEIGHT 333

G_DEFINE_TYPE_WITH_PRIVATE (TotemSeriesSummary, totem_series_summary, GTK_TYPE_BIN);

/* -------------------------------------------------------------------------- *
 * Internal / Helpers
 * -------------------------------------------------------------------------- */

static void
operation_spec_free (OperationSpec *os)
{
  TotemSeriesSummaryPrivate *priv;

  if (os->pending_grl_ops != NULL) {
    /* Wait pending grilo operations to finish */
    return;
  }

  priv = os->totem_series_summary->priv;
  priv->pending_ops = g_list_remove (priv->pending_ops, os);

  g_clear_object (&os->video);
  g_clear_pointer (&os->poster_path, g_free);
  g_slice_free (OperationSpec, os);
}

static void
add_video_to_summary_and_free (OperationSpec *os)
{
  TotemSeriesSummary *self = os->totem_series_summary;

  /* FIXME: For now, we add the video in the end. It should be an update here */
  totem_series_view_add_video (self->priv->view, os->video);
  operation_spec_free (os);
}

static void
resolve_poster_done (GObject      *source_object,
                     GAsyncResult *res,
                     gpointer      user_data)
{
  OperationSpec *os;
  gchar *data;
  gsize len;
  GError *err = NULL;

  os = user_data;
  grl_net_wc_request_finish (GRL_NET_WC (source_object),
                             res, &data, &len, &err);
  if (err != NULL) {
    g_warning ("Fetch image failed due: %s", err->message);
    g_error_free (err);
  } else {
    g_file_set_contents (os->poster_path, data, len, &err);
  }

  /* Update interface */
  add_video_to_summary_and_free (os);
}

static void
resolve_metadata_done (GrlSource    *source,
                       guint         operation_id,
                       GrlMedia     *media,
                       gpointer      user_data,
                       const GError *error)
{
  TotemSeriesSummaryPrivate *priv;
  OperationSpec *os = user_data;
  const gchar *title, *poster_url;

  os->pending_grl_ops = g_list_remove (os->pending_grl_ops,
                                       GUINT_TO_POINTER (operation_id));

  if (error) {
    g_warning ("Resolve operation failed: %s", error->message);
    operation_spec_free (os);
    return;
  }

  priv = os->totem_series_summary->priv;

  title = grl_media_get_show (media);
  if (title == NULL) {
    g_warning ("Basic information is missing - no title");
    operation_spec_free (os);
    return;
  }

  poster_url = grl_data_get_string (GRL_DATA (media), priv->tvdb_poster_key);
  if (poster_url != NULL) {
    os->poster_path = g_build_filename (g_get_tmp_dir (), title, NULL);
    if (!g_file_test (os->poster_path, G_FILE_TEST_EXISTS)) {
      GrlNetWc *wc = grl_net_wc_new ();
      grl_net_wc_request_async (wc, poster_url, NULL, resolve_poster_done, os);
      g_object_unref (wc);
      return;
    }
  }

  add_video_to_summary_and_free (os);
}

static void
resolve_by_the_tvdb (OperationSpec *os)
{
  TotemSeriesSummaryPrivate *priv;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;
  guint op_id;

  priv = os->totem_series_summary->priv;
  caps = grl_source_get_caps (priv->tvdb_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_AUTHOR,
                                    GRL_METADATA_KEY_GENRE,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    priv->tvdb_poster_key,
                                    GRL_METADATA_KEY_INVALID);
  op_id = grl_source_resolve (priv->tvdb_source,
                              os->video,
                              keys,
                              options,
                              resolve_metadata_done,
                              os);
  g_object_unref (options);
  g_list_free (keys);

  os->pending_grl_ops = g_list_prepend (os->pending_grl_ops,
                                        GUINT_TO_POINTER (op_id));
}

static void
resolve_video_summary_media (OperationSpec *os)
{
  TotemSeriesSummary *self;

  g_assert_nonnull (os);

  self = os->totem_series_summary;

  if (grl_media_get_show (os->video) != NULL) {
    /* And set basic information */
    /* FIXME: We should add the video here (or earlier) so we can update the UI
     * (if necessary) with basic inforation of tv-show
     *
     * totem_series_view_add_video (self->priv->view, os->video);
     */
    resolve_by_the_tvdb (os);

    return;
  }

  g_warning ("video type is not defined: %s", grl_media_get_url (os->video));
  operation_spec_free (os);
}

static void
resolve_by_video_title_parsing_done (GrlSource    *source,
                                     guint         operation_id,
                                     GrlMedia     *media,
                                     gpointer      user_data,
                                     const GError *error)
{
  OperationSpec *os = user_data;

  os->pending_grl_ops = g_list_remove (os->pending_grl_ops,
                                       GUINT_TO_POINTER (operation_id));
  if (error != NULL) {
    g_warning ("video-title-parsing failed: %s", error->message);
    operation_spec_free (os);
    return;
  }

  resolve_video_summary_media (os);
}

static void
resolve_by_video_title_parsing (OperationSpec *os)
{
  TotemSeriesSummaryPrivate *priv;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;
  guint op_id;

  priv = os->totem_series_summary->priv;
  caps = grl_source_get_caps (priv->video_title_parsing_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    GRL_METADATA_KEY_SHOW,
                                    GRL_METADATA_KEY_SEASON,
                                    GRL_METADATA_KEY_EPISODE,
                                    GRL_METADATA_KEY_INVALID);

  /* We want to extract all metadata from file's name */
  grl_data_set_boolean (GRL_DATA (os->video),
                        GRL_METADATA_KEY_TITLE_FROM_FILENAME,
                        TRUE);
  op_id = grl_source_resolve (priv->video_title_parsing_source,
                              os->video,
                              keys,
                              options,
                              resolve_by_video_title_parsing_done,
                              os);
  g_object_unref (options);
  g_list_free (keys);

  os->pending_grl_ops = g_list_prepend (os->pending_grl_ops,
                                        GUINT_TO_POINTER (op_id));
}

/* -------------------------------------------------------------------------- *
 * External
 * -------------------------------------------------------------------------- */

TotemSeriesSummary *
totem_series_summary_new (void)
{
  TotemSeriesSummary *self;
  TotemSeriesSummaryPrivate *priv;
  GrlSource *source;
  GrlRegistry *registry;

  self = g_object_new (TOTEM_TYPE_SERIES_SUMMARY, NULL);
  priv = self->priv;
  registry = grl_registry_get_default();
  priv->registry = registry;

  /* Those plugins should be loaded as requirement */

  source = grl_registry_lookup_source (registry, "grl-thetvdb");
  if (source != NULL) {
    priv->tvdb_source = source;
    priv->tvdb_poster_key =
        grl_registry_lookup_metadata_key (registry, "thetvdb-poster");
  } else {
    g_warning ("Failed to load tvdb source");
  }

  source = grl_registry_lookup_source (registry, "grl-video-title-parsing");
  g_return_val_if_fail (source != NULL, NULL);
  priv->video_title_parsing_source = source;

  source = grl_registry_lookup_source (registry, "grl-opensubtitles");
  if (source != NULL) {
    priv->opensubtitles_source = source;
    priv->subtitles_lang_key =
        grl_registry_lookup_metadata_key (registry, "subtitles-lang");
    priv->subtitles_url_key =
        grl_registry_lookup_metadata_key (registry, "subtitles-url");
/*    priv->subtitles_enabled = TRUE;*/
/*  } else {*/
/*    priv->subtitles_enabled = FALSE;*/
/*    g_warning ("Opensubtitles not available");*/
  }

  return self;
}

gboolean
totem_series_summary_add_video (TotemSeriesSummary *self,
                                GrlMedia           *video)
{
  const gchar *url;
  OperationSpec *os;

  g_return_val_if_fail (TOTEM_IS_SERIES_SUMMARY (self), FALSE);
  g_return_val_if_fail (video != NULL, FALSE);

  url = grl_media_get_url (video);
  if (url == NULL) {
    g_warning ("Video does not have url: can't initialize totem-video-summary");
    return FALSE;
  }

#ifndef ON_DEVELOPMENT
  if (!g_file_test (url, G_FILE_TEST_EXISTS)) {
    g_warning ("Video file does not exist");
    return FALSE;
  }
#endif

  os = g_slice_new0 (OperationSpec);
  os->totem_series_summary = self;
  os->video = g_object_ref (video);
  self->priv->pending_ops = g_list_prepend (self->priv->pending_ops, os);
  if (self->priv->video_title_parsing_source != NULL) {
    resolve_by_video_title_parsing (os);
  } else {
    resolve_video_summary_media (os);
  }
  return TRUE;
}

/* -------------------------------------------------------------------------- *
 * Object
 * -------------------------------------------------------------------------- */

static void
totem_series_summary_finalize (GObject *object)
{
  TotemSeriesSummaryPrivate *priv = TOTEM_SERIES_SUMMARY (object)->priv;

  if (priv->pending_ops) {
    GList *it = priv->pending_ops;

    /* Cancel all pending operations and release its data */
    while (it != NULL) {
      GList *next = it->next;
      OperationSpec *os = it->data;
      GList *it_os;

      for (it_os = os->pending_grl_ops; it_os != NULL; it_os = it_os->next) {
        grl_operation_cancel (GPOINTER_TO_UINT (it_os->data));
      }
      g_clear_pointer (&os->pending_grl_ops, g_list_free);
      operation_spec_free (os);
      it = next;
    }
    g_warn_if_fail (priv->pending_ops == NULL);
  }

  G_OBJECT_CLASS (totem_series_summary_parent_class)->finalize (object);
}

static void
totem_series_summary_init (TotemSeriesSummary *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->priv = totem_series_summary_get_instance_private (self);
}

static void
totem_series_summary_class_init (TotemSeriesSummaryClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->finalize = totem_series_summary_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/totem/grilo/totem-series-summary.ui");
  gtk_widget_class_bind_template_child_private (widget_class, TotemSeriesSummary, view);
}
