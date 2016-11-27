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

#ifndef TOTEM_SERIES_SUMMARY_H
#define TOTEM_SERIES_SUMMARY_H

#include <gtk/gtk.h>
#include <grilo.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_SERIES_SUMMARY             (totem_series_summary_get_type())

#define TOTEM_SERIES_SUMMARY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_SERIES_SUMMARY, TotemSeriesSummary))
#define TOTEM_SERIES_SUMMARY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_SERIES_SUMMARY, TotemSeriesSummaryClass))
#define TOTEM_IS_SERIES_SUMMARY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_SERIES_SUMMARY))
#define TOTEM_IS_SERIES_SUMMARY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_SERIES_SUMMARY))
#define TOTEM_SERIES_SUMMARY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TOTEM_TYPE_SERIES_SUMMARY, TotemSeriesSummaryClass))

typedef struct _TotemSeriesSummary        TotemSeriesSummary;
typedef struct _TotemSeriesSummaryClass   TotemSeriesSummaryClass;
typedef struct _TotemSeriesSummaryPrivate TotemSeriesSummaryPrivate;

struct _TotemSeriesSummary
{
  GtkBin parent_instance;
  TotemSeriesSummaryPrivate *priv;
};

struct _TotemSeriesSummaryClass
{
  GtkBinClass parent_class;
};

GType               totem_series_summary_get_type           (void) G_GNUC_CONST;

/* External */
TotemSeriesSummary *totem_series_summary_new (void);
gboolean totem_series_summary_add_video (TotemSeriesSummary *self,
                                         GrlMedia           *video);

G_END_DECLS

#endif /* TOTEM_SERIES_SUMMARY_H */
