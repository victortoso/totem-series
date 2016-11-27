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

#ifndef TOTEM_EPISODE_VIEW_H
#define TOTEM_EPISODE_VIEW_H

#include <emeus.h>
#include <grilo.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_EPISODE_VIEW             (totem_episode_view_get_type())

#define TOTEM_EPISODE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_EPISODE_VIEW, TotemEpisodeView))
#define TOTEM_EPISODE_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_EPISODE_VIEW, TotemEpisodeViewClass))
#define TOTEM_IS_EPISODE_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_EPISODE_VIEW))
#define TOTEM_IS_EPISODE_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_EPISODE_VIEW))
#define TOTEM_EPISODE_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TOTEM_TYPE_EPISODE_VIEW, TotemEpisodeViewClass))

typedef struct _TotemEpisodeView        TotemEpisodeView;
typedef struct _TotemEpisodeViewClass   TotemEpisodeViewClass;
typedef struct _TotemEpisodeViewPrivate TotemEpisodeViewPrivate;

struct _TotemEpisodeView
{
  GtkBin parent_instance;
  TotemEpisodeViewPrivate *priv;
};

struct _TotemEpisodeViewClass
{
  GtkBinClass parent_class;
};

GType               totem_episode_view_get_type           (void) G_GNUC_CONST;

/* External */
TotemEpisodeView *totem_episode_view_new (void);
void totem_episode_view_set_media (TotemEpisodeView *self,
                                   GrlMedia         *media);

G_END_DECLS

#endif /* TOTEM_EPISODE_VIEW_H */
