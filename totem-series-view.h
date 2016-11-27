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

#ifndef TOTEM_SERIES_VIEW_H
#define TOTEM_SERIES_VIEW_H

#include <emeus.h>
#include <grilo.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_SERIES_VIEW             (totem_series_view_get_type())

#define TOTEM_SERIES_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_SERIES_VIEW, TotemSeriesView))
#define TOTEM_SERIES_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_SERIES_VIEW, TotemSeriesViewClass))
#define TOTEM_IS_SERIES_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_SERIES_VIEW))
#define TOTEM_IS_SERIES_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_SERIES_VIEW))
#define TOTEM_SERIES_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TOTEM_TYPE_SERIES_VIEW, TotemSeriesViewClass))

typedef struct _TotemSeriesView        TotemSeriesView;
typedef struct _TotemSeriesViewClass   TotemSeriesViewClass;
typedef struct _TotemSeriesViewPrivate TotemSeriesViewPrivate;

struct _TotemSeriesView
{
  GtkBin parent_instance;
  TotemSeriesViewPrivate *priv;
};

struct _TotemSeriesViewClass
{
  GtkBinClass parent_class;
};

GType               totem_series_view_get_type           (void) G_GNUC_CONST;

/* External */
TotemSeriesView *totem_series_view_new (void);
gboolean totem_series_view_add_video (TotemSeriesView *self,
                                      GrlMedia        *video);

G_END_DECLS

#endif /* TOTEM_SERIES_VIEW_H */
