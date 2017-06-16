/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Canonical Ltd
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

#include <gs-utils.h>
#include <gs-plugin.h>
#include <glib/gi18n.h>
#include <json-glib/json-glib.h>
#include "gs-snapd.h"
#include "gs-ubuntuone.h"

struct GsPluginPrivate {
	GHashTable	*store_snaps;
};

typedef gboolean (*AppFilterFunc)(const gchar *id, JsonObject *object, gpointer data);

const gchar *
gs_plugin_get_name (void)
{
	return "snap";
}

void
gs_plugin_initialize (GsPlugin *plugin)
{
	/* create private area */
	plugin->priv = GS_PLUGIN_GET_PRIVATE (GsPluginPrivate);

	if (!gs_snapd_exists ()) {
		g_debug ("disabling '%s' as snapd not running",
			 gs_plugin_get_name ());
		gs_plugin_set_enabled (plugin, FALSE);
	}

	plugin->priv->store_snaps = g_hash_table_new_full (g_str_hash, g_str_equal,
							   g_free, (GDestroyNotify) json_object_unref);
}

static gboolean
gs_plugin_snap_set_app_pixbuf_from_data (GsApp *app, const gchar *buf, gsize count, GError **error)
{
	g_autoptr(GdkPixbufLoader) loader = NULL;
	g_autoptr(GError) error_local = NULL;

	loader = gdk_pixbuf_loader_new ();
	if (!gdk_pixbuf_loader_write (loader, buf, count, &error_local)) {
		g_debug ("icon_data[%" G_GSIZE_FORMAT "]=%s", count, buf);
		g_set_error (error,
			     GS_PLUGIN_ERROR,
			     GS_PLUGIN_ERROR_FAILED,
			     "Failed to write: %s",
			     error_local->message);
		return FALSE;
	}
	if (!gdk_pixbuf_loader_close (loader, &error_local)) {
		g_set_error (error,
			     GS_PLUGIN_ERROR,
			     GS_PLUGIN_ERROR_FAILED,
			     "Failed to close: %s",
			     error_local->message);
		return FALSE;
	}
	gs_app_set_pixbuf (app, gdk_pixbuf_loader_get_pixbuf (loader));
	return TRUE;
}

static JsonArray *
find_snaps (GsPlugin *plugin, const gchar *section, gboolean match_name, const gchar *query, GCancellable *cancellable, GError **error)
{
	g_autoptr(JsonArray) snaps = NULL;
	guint i;

	snaps = gs_snapd_find (section, match_name, query, cancellable, error);
	if (snaps == NULL)
		return NULL;

	/* cache results */
	for (i = 0; i < json_array_get_length (snaps); i++) {
		JsonObject *snap = json_array_get_object_element (snaps, i);
		g_hash_table_insert (plugin->priv->store_snaps, g_strdup (json_object_get_string_member (snap, "name")), json_object_ref (snap));
	}

	return g_steal_pointer (&snaps);
}

static GsApp *
snap_to_app (GsPlugin *plugin, JsonObject *snap)
{
	GsApp *app;

	/* create a unique ID for deduplication, TODO: branch? */
	app = gs_app_new (json_object_get_string_member (snap, "name"));
	gs_app_set_kind (app, AS_APP_KIND_DESKTOP);
	gs_app_set_management_plugin (app, "snap");
	gs_app_add_quirk (app, AS_APP_QUIRK_NOT_REVIEWABLE);
	gs_app_set_name (app, GS_APP_QUALITY_HIGHEST, json_object_get_string_member (snap, "name"));
	gs_app_set_origin (app, _("Ubuntu Snappy Store")); // FIXME: Not necessarily from the snap store...
	if (gs_plugin_check_distro_id (plugin, "ubuntu"))
		gs_app_add_quirk (app, AS_APP_QUIRK_PROVENANCE);

	return app;
}

gboolean
gs_plugin_url_to_app (GsPlugin *plugin,
		      GList **list,
		      const gchar *url,
		      GCancellable *cancellable,
		      GError **error)
{
	g_autofree gchar *scheme = NULL;
	g_autoptr(JsonArray) snaps = NULL;
	JsonObject *snap;
	g_autofree gchar *path = NULL;
	g_autoptr(GsApp) app = NULL;

	/* not us */
	scheme = gs_utils_get_url_scheme (url);
	if (g_strcmp0 (scheme, "snap") != 0)
		return TRUE;

	/* create app */
	path = gs_utils_get_url_path (url);
	snaps = find_snaps (plugin, NULL, TRUE, path, cancellable, NULL);
	if (snaps == NULL || json_array_get_length (snaps) < 1)
		return TRUE;

	snap = json_array_get_object_element (snaps, 0);
	gs_plugin_add_app (list, snap_to_app (plugin, snap));

	return TRUE;
}

void
gs_plugin_destroy (GsPlugin *plugin)
{
	g_hash_table_unref (plugin->priv->store_snaps);
}

gboolean
gs_plugin_add_popular (GsPlugin *plugin,
		       GList **list,
		       GCancellable *cancellable,
		       GError **error)
{
	g_autoptr(JsonArray) snaps = NULL;
	guint i;

	snaps = find_snaps (plugin, "featured", FALSE, NULL, cancellable, error);
	if (snaps == NULL)
		return FALSE;

	for (i = 0; i < json_array_get_length (snaps); i++) {
		JsonObject *snap = json_array_get_object_element (snaps, i);
		gs_plugin_add_app (list, snap_to_app (plugin, snap));
	}

	return TRUE;
}

gboolean
gs_plugin_add_installed (GsPlugin *plugin,
			 GList **list,
			 GCancellable *cancellable,
			 GError **error)
{
	g_autoptr(JsonArray) snaps = NULL;
	guint i;

	snaps = gs_snapd_list (cancellable, error);
	if (snaps == NULL)
		return FALSE;

	for (i = 0; i < json_array_get_length (snaps); i++) {
		JsonObject *snap = json_array_get_object_element (snaps, i);
		const gchar *status;

		status = json_object_get_string_member (snap, "status");
		if (g_strcmp0 (status, "active") != 0)
			continue;

		gs_plugin_add_app (list, snap_to_app (plugin, snap));
	}

	return TRUE;
}

gboolean
gs_plugin_add_search (GsPlugin *plugin,
		      gchar **values,
		      GList **list,
		      GCancellable *cancellable,
		      GError **error)
{
	g_autofree gchar *query = NULL;
	g_autoptr(JsonArray) snaps = NULL;
	guint i;

	query = g_strjoinv (" ", values);
	snaps = find_snaps (plugin, NULL, FALSE, query, cancellable, error);
	if (snaps == NULL)
		return FALSE;

	for (i = 0; i < json_array_get_length (snaps); i++) {
		JsonObject *snap = json_array_get_object_element (snaps, i);
		gs_plugin_add_app (list, snap_to_app (plugin, snap));
	}

	return TRUE;
}

static gboolean
load_icon (GsPlugin *plugin, GsApp *app, const gchar *icon_url, GCancellable *cancellable, GError **error)
{
	if (icon_url == NULL || g_strcmp0 (icon_url, "") == 0) {
		g_autoptr(AsIcon) icon = as_icon_new ();
		as_icon_set_kind (icon, AS_ICON_KIND_STOCK);
		as_icon_set_name (icon, "package-x-generic");
		gs_app_set_icon (app, icon);
		return TRUE;
	}

	/* icon is optional, either loaded from snapd or from a URL */
	if (g_str_has_prefix (icon_url, "/")) {
		g_autofree gchar *icon_data = NULL;
		gsize icon_data_length;

		icon_data = gs_snapd_get_resource (icon_url, &icon_data_length, cancellable, error);
		if (icon_data == NULL)
			return FALSE;

		if (!gs_plugin_snap_set_app_pixbuf_from_data (app,
							      icon_data, icon_data_length,
							      error)) {
			g_prefix_error (error, "Failed to load %s: ", icon_url);
			return FALSE;
		}
	} else {
		g_autofree gchar *basename_tmp = NULL;
		g_autofree gchar *hash = NULL;
		g_autofree gchar *basename = NULL;
		g_autofree gchar *cache_dir = NULL;
		g_autofree gchar *cache_fn = NULL;
		g_autoptr(SoupMessage) message = NULL;
		g_autoptr(GdkPixbufLoader) loader = NULL;
		g_autoptr(GError) local_error = NULL;

		/* attempt to load from cache */
		basename_tmp = g_path_get_basename (icon_url);
		hash = g_compute_checksum_for_string (G_CHECKSUM_SHA1, icon_url, -1);
		basename = g_strdup_printf ("%s-%s", hash, basename_tmp);
		cache_dir = gs_utils_get_cachedir ("snap-icons", error);
		cache_fn = g_build_filename (cache_dir, basename, NULL);
		if (cache_fn == NULL)
			return FALSE;
		if (g_file_test (cache_fn, G_FILE_TEST_EXISTS)) {
			g_autofree gchar *data = NULL;
			gsize data_len;

			if (g_file_get_contents (cache_fn, &data, &data_len, &local_error) &&
			    gs_plugin_snap_set_app_pixbuf_from_data (app,
								     data, data_len,
								     error))
				return TRUE;

			g_warning ("Failed to load cached icon: %s", local_error->message);
		}

		/* load from URL */
		message = soup_message_new (SOUP_METHOD_GET, icon_url);
		if (message == NULL) {
			g_set_error (error,
				     GS_PLUGIN_ERROR,
				     GS_PLUGIN_ERROR_NOT_SUPPORTED,
				     "Failed to parse icon URL: %s",
				     icon_url);
			return FALSE;
		}
		soup_session_send_message (plugin->soup_session, message);
		if (!gs_plugin_snap_set_app_pixbuf_from_data (app,
					(const gchar *) message->response_body->data,
					message->response_body->length,
					error)) {
			g_prefix_error (error, "Failed to load %s: ", icon_url);
			return FALSE;
		}

		/* write to cache */
		if (!g_file_set_contents (cache_fn, message->response_body->data, message->response_body->length, &local_error))
			g_warning ("Failed to save icon to cache: %s", local_error->message);
	}

	return TRUE;
}

static JsonObject *
get_store_snap (GsPlugin *plugin, const gchar *name, GCancellable *cancellable, GError **error)
{
	JsonObject *snap = NULL;
	g_autoptr(JsonArray) snaps = NULL;

	/* use cached version if available */
	snap = g_hash_table_lookup (plugin->priv->store_snaps, name);
	if (snap != NULL)
		return json_object_ref (snap);

	snaps = find_snaps (plugin, NULL, TRUE, name, cancellable, error);
	if (snaps == NULL || json_array_get_length (snaps) < 1)
		return NULL;

	return json_object_ref (json_array_get_object_element (snaps, 0));
}

static gboolean
gs_plugin_refine_app (GsPlugin *plugin,
		      GsApp *app,
		      GsPluginRefineFlags flags,
		      GCancellable *cancellable,
		      GError **error)
{
	const gchar *id, *icon_url = NULL;
	g_autoptr(JsonObject) local_snap = NULL;
	g_autoptr(JsonObject) store_snap = NULL;

	/* not us */
	if (g_strcmp0 (gs_app_get_management_plugin (app), "snap") != 0)
		return TRUE;

	id = gs_app_get_id (app);
	if (id == NULL)
		id = gs_app_get_source_default (app);

	/* get information from installed snaps */
	local_snap = gs_snapd_list_one (id, cancellable, NULL);
	if (local_snap != NULL) {
		JsonArray *apps;
		g_autoptr(GDateTime) install_date = NULL;
		const gchar *launch_name = NULL;

		if (gs_app_get_state (app) == AS_APP_STATE_UNKNOWN)
			gs_app_set_state (app, AS_APP_STATE_INSTALLED);

		gs_app_set_name (app, GS_APP_QUALITY_NORMAL, json_object_get_string_member (local_snap, "name"));
		gs_app_set_summary (app, GS_APP_QUALITY_NORMAL, json_object_get_string_member (local_snap, "summary"));
		gs_app_set_description (app, GS_APP_QUALITY_NORMAL, json_object_get_string_member (local_snap, "description"));
		gs_app_set_version (app, json_object_get_string_member (local_snap, "version"));
		if (json_object_has_member (local_snap, "installed-size"))
			gs_app_set_size (app, json_object_get_int_member (local_snap, "installed-size"));
		if (json_object_has_member (local_snap, "install-date"))
			install_date = gs_snapd_parse_date (json_object_get_string_member (local_snap, "install-date"));
		if (install_date != NULL)
			gs_app_set_install_date (app, g_date_time_to_unix (install_date));
		icon_url = json_object_get_string_member (local_snap, "icon");
		if (g_strcmp0 (icon_url, "") == 0)
			icon_url = NULL;

		apps = json_object_get_array_member (local_snap, "apps");
		if (apps && json_array_get_length (apps) > 0)
			launch_name = json_object_get_string_member (json_array_get_object_element (apps, 0), "name");

		if (launch_name)
			gs_app_set_metadata (app, "snap::launch-name", launch_name);
		else
			gs_app_add_quirk (app, AS_APP_QUIRK_NOT_LAUNCHABLE);
	}

	/* get information from snap store */
	store_snap = get_store_snap (plugin, id, cancellable, NULL);
	if (store_snap != NULL) {
		if (gs_app_get_state (app) == AS_APP_STATE_UNKNOWN)
			gs_app_set_state (app, AS_APP_STATE_AVAILABLE);

		gs_app_set_name (app, GS_APP_QUALITY_NORMAL, json_object_get_string_member (store_snap, "name"));
		gs_app_set_summary (app, GS_APP_QUALITY_NORMAL, json_object_get_string_member (store_snap, "summary"));
		gs_app_set_description (app, GS_APP_QUALITY_NORMAL, json_object_get_string_member (store_snap, "description"));
		gs_app_set_version (app, json_object_get_string_member (store_snap, "version"));
		if (gs_app_get_size (app) == GS_APP_SIZE_UNKNOWN && json_object_has_member (store_snap, "download-size"))
			gs_app_set_size (app, json_object_get_int_member (store_snap, "download-size"));
		if (icon_url == NULL) {
			icon_url = json_object_get_string_member (store_snap, "icon");
			if (g_strcmp0 (icon_url, "") == 0)
				icon_url = NULL;
		}

		if (json_object_has_member (store_snap, "screenshots") && gs_app_get_screenshots (app)->len == 0) {
			JsonArray *screenshots;
			guint i;

			screenshots = json_object_get_array_member (store_snap, "screenshots");
			for (i = 0; i < json_array_get_length (screenshots); i++) {
				JsonObject *screenshot = json_array_get_object_element (screenshots, i);
				g_autoptr(AsScreenshot) ss = NULL;
				g_autoptr(AsImage) image = NULL;

				ss = as_screenshot_new ();
				as_screenshot_set_kind (ss, AS_SCREENSHOT_KIND_NORMAL);
				image = as_image_new ();
				as_image_set_url (image, json_object_get_string_member (screenshot, "url"));
				as_image_set_kind (image, AS_IMAGE_KIND_SOURCE);
				if (json_object_has_member (screenshot, "width"))
					as_image_set_width (image, json_object_get_int_member (screenshot, "width"));
				if (json_object_has_member (screenshot, "height"))
					as_image_set_height (image, json_object_get_int_member (screenshot, "height"));
				as_screenshot_add_image (ss, image);
				gs_app_add_screenshot (app, ss);
			}
		}
	}

	/* load icon if requested */
	if (gs_app_get_pixbuf (app) == NULL && gs_app_get_icon (app) == NULL) {
		if (!load_icon (plugin, app, icon_url, cancellable, error))
			return FALSE;
	}

	return TRUE;
}

gboolean
gs_plugin_refine (GsPlugin *plugin,
		  GList **list,
		  GsPluginRefineFlags flags,
		  GCancellable *cancellable,
		  GError **error)
{
	GList *link;

	for (link = *list; link; link = link->next) {
		GsApp *app = link->data;
		g_autoptr(GError) local_error = NULL;

		if (!gs_plugin_refine_app (plugin, app, flags, cancellable, &local_error))
			g_warning ("Failed to refine snap: %s", local_error->message);
	}

	return TRUE;
}

typedef struct
{
	GsPlugin *plugin;
	GsApp *app;
} ProgressData;

static void
progress_cb (JsonObject *result, gpointer user_data)
{
	ProgressData *data = user_data;
	JsonArray *tasks;
	GList *task_list, *l;
	gint64 done = 0, total = 0;

	tasks = json_object_get_array_member (result, "tasks");
	task_list = json_array_get_elements (tasks);

	for (l = task_list; l != NULL; l = l->next) {
		JsonObject *task, *progress;
		gint64 task_done, task_total;

		task = json_node_get_object (l->data);
		progress = json_object_get_object_member (task, "progress");
		task_done = json_object_get_int_member (progress, "done");
		task_total = json_object_get_int_member (progress, "total");

		done += task_done;
		total += task_total;
	}

	gs_plugin_progress_update (data->plugin, data->app, 100 * done / total);

	g_list_free (task_list);
}

gboolean
gs_plugin_app_install (GsPlugin *plugin,
		       GsApp *app,
		       GCancellable *cancellable,
		       GError **error)
{
	ProgressData data;

	/* We can only install apps we know of */
	if (g_strcmp0 (gs_app_get_management_plugin (app), "snap") != 0)
		return TRUE;

	gs_app_set_state (app, AS_APP_STATE_INSTALLING);
	data.plugin = plugin;
	data.app = app;
	if (!gs_snapd_install (gs_app_get_id (app), progress_cb, &data, cancellable, error)) {
		gs_app_set_state (app, AS_APP_STATE_AVAILABLE);
		return FALSE;
	}
	gs_app_set_state (app, AS_APP_STATE_INSTALLED);
	return TRUE;
}

// Check if an app is graphical by checking if it uses a known GUI interface.
// This doesn't necessarily mean that every binary uses this interfaces, but is probably true.
// https://bugs.launchpad.net/bugs/1595023
static gboolean
is_graphical (GsApp *app, GCancellable *cancellable)
{
	g_autoptr(JsonObject) result = NULL;
	JsonArray *plugs;
	guint i;
	g_autoptr(GError) error = NULL;

	result = gs_snapd_get_interfaces (cancellable, &error);
	if (result == NULL) {
		g_warning ("Failed to check interfaces: %s", error->message);
		return FALSE;
	}

	plugs = json_object_get_array_member (result, "plugs");
	for (i = 0; i < json_array_get_length (plugs); i++) {
		JsonObject *plug = json_array_get_object_element (plugs, i);
		const gchar *interface;

		// Only looks at the plugs for this snap
		if (g_strcmp0 (json_object_get_string_member (plug, "snap"), gs_app_get_id (app)) != 0)
			continue;

		interface = json_object_get_string_member (plug, "interface");
		if (interface == NULL)
			continue;

		if (g_strcmp0 (interface, "unity7") == 0 || g_strcmp0 (interface, "x11") == 0 || g_strcmp0 (interface, "mir") == 0)
			return TRUE;
	}

	return FALSE;
}

gboolean
gs_plugin_launch (GsPlugin *plugin,
		  GsApp *app,
		  GCancellable *cancellable,
		  GError **error)
{
	const gchar *launch_name;
	g_autofree gchar *binary_name = NULL;
	GAppInfoCreateFlags flags = G_APP_INFO_CREATE_NONE;
	g_autoptr(GAppInfo) info = NULL;

	/* We can only launch apps we know of */
	if (g_strcmp0 (gs_app_get_management_plugin (app), "snap") != 0)
		return TRUE;

	launch_name = gs_app_get_metadata_item (app, "snap::launch-name");
	if (!launch_name)
		return TRUE;

	if (g_strcmp0 (launch_name, gs_app_get_id (app)) == 0)
		binary_name = g_strdup_printf ("/snap/bin/%s", launch_name);
	else
		binary_name = g_strdup_printf ("/snap/bin/%s.%s", gs_app_get_id (app), launch_name);

	if (!is_graphical (app, cancellable))
		flags |= G_APP_INFO_CREATE_NEEDS_TERMINAL;
	info = g_app_info_create_from_commandline (binary_name, NULL, flags, error);
	if (info == NULL)
		return FALSE;

	return g_app_info_launch (info, NULL, NULL, error);
}

gboolean
gs_plugin_app_remove (GsPlugin *plugin,
		      GsApp *app,
		      GCancellable *cancellable,
		      GError **error)
{
	ProgressData data;

	/* We can only remove apps we know of */
	if (g_strcmp0 (gs_app_get_management_plugin (app), "snap") != 0)
		return TRUE;

	gs_app_set_state (app, AS_APP_STATE_REMOVING);
	data.plugin = plugin;
	data.app = app;
	if (!gs_snapd_remove (gs_app_get_id (app), progress_cb, &data, cancellable, error)) {
		gs_app_set_state (app, AS_APP_STATE_INSTALLED);
		return FALSE;
	}
	gs_app_set_state (app, AS_APP_STATE_AVAILABLE);
	return TRUE;
}
