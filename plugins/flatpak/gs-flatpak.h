/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2016 Joaquim Rocha <jrocha@endlessm.com>
 * Copyright (C) 2016-2017 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GS_FLATPAK_H
#define __GS_FLATPAK_H

#include <gnome-software.h>

G_BEGIN_DECLS

#define GS_TYPE_FLATPAK (gs_flatpak_get_type ())

G_DECLARE_FINAL_TYPE (GsFlatpak, gs_flatpak, GS, FLATPAK, GObject)

/* helpers */
#define	gs_app_get_flatpak_kind_as_str(app)	gs_app_get_metadata_item(app,"flatpak::kind")
#define	gs_app_get_flatpak_name(app)		gs_app_get_metadata_item(app,"flatpak::name")
#define	gs_app_get_flatpak_arch(app)		gs_app_get_metadata_item(app,"flatpak::arch")
#define	gs_app_get_flatpak_branch(app)		gs_app_get_metadata_item(app,"flatpak::branch")
#define	gs_app_get_flatpak_commit(app)		gs_app_get_metadata_item(app,"flatpak::commit")
#define	gs_app_get_flatpak_file_type(app)	gs_app_get_metadata_item(app,"flatpak::file-type")
#define	gs_app_get_flatpak_object_id(app)	gs_app_get_metadata_item(app,"flatpak::object-id")
#define	gs_app_set_flatpak_name(app,val)	gs_app_set_metadata(app,"flatpak::name",val)
#define	gs_app_set_flatpak_arch(app,val)	gs_app_set_metadata(app,"flatpak::arch",val)
#define	gs_app_set_flatpak_branch(app,val)	gs_app_set_metadata(app,"flatpak::branch",val)
#define	gs_app_set_flatpak_commit(app,val)	gs_app_set_metadata(app,"flatpak::commit",val)
#define	gs_app_set_flatpak_file_type(app,val)	gs_app_set_metadata(app,"flatpak::file-type",val)
#define	gs_app_set_flatpak_object_id(app,val)	gs_app_set_metadata(app,"flatpak::object-id",val)

typedef enum {
	GS_FLATPAK_FLAG_NONE			= 0,
	GS_FLATPAK_FLAG_IS_TEMPORARY		= 1 << 0,
	/*< private >*/
	GS_FLATPAK_FLAG_LAST
} GsFlatpakFlags;

GsFlatpak	*gs_flatpak_new			(GsPlugin		*plugin,
						 FlatpakInstallation	*installation);
void		gs_flatpak_set_flags		(GsFlatpak		*self,
						 GsFlatpakFlags		 flags);
GsFlatpakFlags	gs_flatpak_get_flags		(GsFlatpak		*self);
AsAppScope	gs_flatpak_get_scope		(GsFlatpak		*self);
const gchar	*gs_flatpak_get_id		(GsFlatpak		*self);
gboolean	gs_flatpak_setup		(GsFlatpak		*self,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_installed	(GsFlatpak		*self,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_sources		(GsFlatpak		*self,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_updates		(GsFlatpak		*self,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_updates_pending	(GsFlatpak		*self,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_refresh		(GsFlatpak		*self,
						 guint			cache_age,
						 GsPluginRefreshFlags	flags,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_refine_app		(GsFlatpak		*self,
						 GsApp			*app,
						 GsPluginRefineFlags	flags,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_refine_wildcard	(GsFlatpak		*self,
						 GsApp			*app,
						 GsAppList		*list,
						 GsPluginRefineFlags	 flags,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_launch		(GsFlatpak		*self,
						 GsApp			*app,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_app_remove		(GsFlatpak		*self,
						 GsApp			*app,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_app_install		(GsFlatpak		*self,
						 GsApp			*app,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_update_app		(GsFlatpak		*self,
						 GsApp			*app,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_file_to_app		(GsFlatpak		*self,
						 GsAppList		*list,
						 GFile			*file,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_search		(GsFlatpak		*self,
						 gchar			**values,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_categories	(GsFlatpak		*self,
						 GPtrArray		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_category_apps	(GsFlatpak		*self,
						 GsCategory		*category,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_popular		(GsFlatpak		*self,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_featured		(GsFlatpak		*self,
						 GsAppList		*list,
						 GCancellable		*cancellable,
						 GError			**error);
gboolean	gs_flatpak_add_recent		(GsFlatpak		*self,
						 GsAppList		*list,
						 guint64		 age,
						 GCancellable		*cancellable,
						 GError			**error);
void		gs_plugin_flatpak_error_convert	(GError			**perror);

G_END_DECLS

#endif /* __GS_FLATPAK_H */

