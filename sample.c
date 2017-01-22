#include <glib.h>
#include "totem-series-summary.h"

struct {
    char *gibest_hash;
    gint64 file_size;
    char *url;
    char *title;
} videos[] = {
    /* Series */
/*    { "7dc362e0ab70fc85", 99222120, "House.S01E01.mkv", "HOUSE" },*/
    { "", 0, "Breaking.Bad.S01E01.mkv", "Breaking Bad" },
    { "", 0, "Breaking.Bad.S01E02.mkv", "Breaking Bad" },
    { "", 0, "Breaking.Bad.S03E02.mkv", "Breaking Bad" },
    { "", 0, "Breaking.Bad.S03E01.mkv", "Breaking Bad" },
    { "", 0, "Breaking.Bad.S02E03.mkv", "Breaking Bad" },
    { "", 0, "Breaking.Bad.S02E01.mkv", "Breaking Bad" },
    { "", 0, "Breaking.Bad.S02E02.mkv", "Breaking Bad" }
};

GrlKeyID gibest_hash_key = GRL_METADATA_KEY_INVALID;

#define THETVDB_ID  "grl-thetvdb"
#define THETVDB_KEY "3F476CEF2FBD0FB0"

#define TMDB_ID  "grl-tmdb"
#define TMDB_KEY "719b9b296835b04cd919c4bf5220828a"

#define LUA_FACTORY_ID "grl-lua-factory"

#define TRACKER_ID       "grl-tracker"
#define OPENSUBTITLES_ID "grl-opensubtitles"

static void
setup_grilo(void)
{
  GrlConfig *config;
  GrlRegistry *registry;
  GError *error = NULL;

  registry = grl_registry_get_default ();

  /* For metadata in the filename */
  grl_registry_load_all_plugins (registry, FALSE, &error);

  /* For Movies and Series */
  config = grl_config_new (THETVDB_ID, NULL);
  grl_config_set_api_key (config, THETVDB_KEY);
  grl_registry_add_config (registry, config, &error);
  g_assert_no_error (error);
  grl_registry_activate_plugin_by_id (registry, THETVDB_ID, &error);
  g_assert_no_error (error);

  config = grl_config_new (TMDB_ID, NULL);
  grl_config_set_api_key (config, TMDB_KEY);
  grl_registry_add_config (registry, config, &error);
  grl_registry_activate_plugin_by_id (registry, TMDB_ID, &error);
  g_assert_no_error (error);

  /* On lua-factory we use grl-video-title-parsing source */
  grl_registry_activate_plugin_by_id (registry, LUA_FACTORY_ID, &error);
  g_assert_no_error (error);

  /* For subtitles */
  grl_registry_activate_plugin_by_id (registry, TRACKER_ID, &error);
  grl_registry_activate_plugin_by_id (registry, OPENSUBTITLES_ID, &error);
  g_assert_no_error (error);

  gibest_hash_key = grl_registry_lookup_metadata_key (registry, "gibest-hash");
}

gint main(gint argc, gchar *argv[])
{
    GtkSettings *gtk_settings;
    TotemSeriesSummary *tss;
    GrlMedia *video;
    GtkWidget *win;
    gint i;

    gtk_init (&argc, &argv);
    grl_init (&argc, &argv);

    gtk_settings = gtk_settings_get_default ();
    g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

    setup_grilo();
    g_assert_true (gibest_hash_key != GRL_METADATA_KEY_INVALID);
    tss = totem_series_summary_new ();
    g_return_val_if_fail (TOTEM_IS_SERIES_SUMMARY (tss), 1);

    for (i = 0; i < G_N_ELEMENTS (videos); i++) {
      video = grl_media_video_new();
      g_debug ("url: %s", videos[i].url);
      grl_media_set_url (video, videos[i].url);
      grl_media_set_title (video, videos[i].url);
      grl_data_set_string (GRL_DATA (video), gibest_hash_key, videos[i].gibest_hash);
      grl_media_set_size (video, videos[i].file_size);
      totem_series_summary_add_video (tss, video);
      g_object_unref (video);
    }

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect (GTK_WINDOW (win), "destroy", G_CALLBACK (gtk_main_quit), NULL);
    gtk_window_set_default_size (GTK_WINDOW (win), 100, 100);

    gtk_window_set_title(GTK_WINDOW(win), "Totem TVSHOWS");
    gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET(tss));
    gtk_widget_show (GTK_WIDGET(tss));
    gtk_widget_show (win);
    gtk_main ();
    return 0;
}
