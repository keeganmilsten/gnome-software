/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2017 Richard Hughes <richard@hughsie.com>
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

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>

#include "gnome-software-private.h"

#include "gs-debug.h"

static void
gs_cmd_show_results_apps (GsAppList *list)
{
	GPtrArray *related;
	GsApp *app;
	GsApp *app_rel;
	guint i;
	guint j;

	for (j = 0; j < gs_app_list_length (list); j++) {
		g_autofree gchar *tmp = NULL;
		app = gs_app_list_index (list, j);
		tmp = gs_app_to_string (app);
		g_print ("%s\n", tmp);
		related = gs_app_get_related (app);
		for (i = 0; i < related->len; i++) {
			g_autofree gchar *tmp_rel = NULL;
			app_rel = GS_APP (g_ptr_array_index (related, i));
			tmp_rel = gs_app_to_string (app_rel);
			g_print ("\t%s\n", tmp_rel);
		}
	}
}

static gchar *
gs_cmd_pad_spaces (const gchar *text, guint length)
{
	gsize i;
	GString *str;
	str = g_string_sized_new (length + 1);
	g_string_append (str, text);
	for (i = strlen (text); i < length; i++)
		g_string_append_c (str, ' ');
	return g_string_free (str, FALSE);
}

static void
gs_cmd_show_results_categories (GPtrArray *list)
{
	GPtrArray *subcats;
	GsCategory *cat;
	GsCategory *parent;
	guint i;

	for (i = 0; i < list->len; i++) {
		g_autofree gchar *tmp = NULL;
		cat = GS_CATEGORY (g_ptr_array_index (list, i));
		parent = gs_category_get_parent (cat);
		if (parent != NULL){
			g_autofree gchar *id = NULL;
			id = g_strdup_printf ("%s/%s [%u]",
					      gs_category_get_id (parent),
					      gs_category_get_id (cat),
					      gs_category_get_size (cat));
			tmp = gs_cmd_pad_spaces (id, 32);
			g_print ("%s : %s\n",
				 tmp, gs_category_get_name (cat));
		} else {
			tmp = gs_cmd_pad_spaces (gs_category_get_id (cat), 32);
			g_print ("%s : %s\n",
				 tmp, gs_category_get_name (cat));
			subcats = gs_category_get_children (cat);
			gs_cmd_show_results_categories (subcats);
		}
	}
}

static GsPluginRefineFlags
gs_cmd_refine_flag_from_string (const gchar *flag, GError **error)
{
	if (g_strcmp0 (flag, "all") == 0)
		return G_MAXINT32;
	if (g_strcmp0 (flag, "license") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_LICENSE;
	if (g_strcmp0 (flag, "url") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_URL;
	if (g_strcmp0 (flag, "description") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_DESCRIPTION;
	if (g_strcmp0 (flag, "size") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_SIZE;
	if (g_strcmp0 (flag, "rating") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_RATING;
	if (g_strcmp0 (flag, "version") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_VERSION;
	if (g_strcmp0 (flag, "history") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_HISTORY;
	if (g_strcmp0 (flag, "setup-action") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_SETUP_ACTION;
	if (g_strcmp0 (flag, "update-details") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_UPDATE_DETAILS;
	if (g_strcmp0 (flag, "origin") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_ORIGIN;
	if (g_strcmp0 (flag, "related") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_RELATED;
	if (g_strcmp0 (flag, "menu-path") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_MENU_PATH;
	if (g_strcmp0 (flag, "upgrade-removed") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_UPGRADE_REMOVED;
	if (g_strcmp0 (flag, "provenance") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_PROVENANCE;
	if (g_strcmp0 (flag, "reviews") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_REVIEWS;
	if (g_strcmp0 (flag, "review-ratings") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_REVIEW_RATINGS;
	if (g_strcmp0 (flag, "key-colors") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_KEY_COLORS;
	if (g_strcmp0 (flag, "icon") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_ICON;
	if (g_strcmp0 (flag, "permissions") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_PERMISSIONS;
	if (g_strcmp0 (flag, "origin-hostname") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_ORIGIN_HOSTNAME;
	if (g_strcmp0 (flag, "origin-ui") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_ORIGIN_UI;
	if (g_strcmp0 (flag, "runtime") == 0)
		return GS_PLUGIN_REFINE_FLAGS_REQUIRE_RUNTIME;
	g_set_error (error,
		     GS_PLUGIN_ERROR,
		     GS_PLUGIN_ERROR_NOT_SUPPORTED,
		     "GsPluginRefineFlag '%s' not recognised", flag);
	return 0;
}

static guint64
gs_cmd_parse_refine_flags (const gchar *extra, GError **error)
{
	GsPluginRefineFlags tmp;
	guint i;
	guint64 refine_flags = GS_PLUGIN_REFINE_FLAGS_DEFAULT;
	g_auto(GStrv) split = NULL;

	if (extra == NULL)
		return GS_PLUGIN_REFINE_FLAGS_DEFAULT;

	split = g_strsplit (extra, ",", -1);
	for (i = 0; split[i] != NULL; i++) {
		tmp = gs_cmd_refine_flag_from_string (split[i], error);
		if (tmp == 0)
			return G_MAXUINT64;
		refine_flags |= tmp;
	}
	return refine_flags;
}

static GsPluginRefreshFlags
gs_cmd_refresh_flag_from_string (const gchar *flag)
{
	if (flag == NULL || g_strcmp0 (flag, "all") == 0)
		return G_MAXINT32;
	if (g_strcmp0 (flag, "metadata") == 0)
		return GS_PLUGIN_REFRESH_FLAGS_METADATA;
	if (g_strcmp0 (flag, "payload") == 0)
		return GS_PLUGIN_REFRESH_FLAGS_PAYLOAD;
	return GS_PLUGIN_REFRESH_FLAGS_NONE;
}

int
main (int argc, char **argv)
{
	AsProfile *profile = NULL;
	GOptionContext *context;
	gboolean prefer_local = FALSE;
	gboolean profile_enable = FALSE;
	gboolean ret;
	gboolean show_results = FALSE;
	gboolean verbose = FALSE;
	guint64 refine_flags = GS_PLUGIN_REFINE_FLAGS_DEFAULT;
	gint i;
	guint cache_age = 0;
	guint max_results = 0;
	gint repeat = 1;
	int status = 0;
	g_auto(GStrv) plugin_blacklist = NULL;
	g_auto(GStrv) plugin_whitelist = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GsAppList) list = NULL;
	g_autoptr(GPtrArray) categories = NULL;
	g_autoptr(GsDebug) debug = gs_debug_new ();
	g_autofree gchar *plugin_blacklist_str = NULL;
	g_autofree gchar *plugin_whitelist_str = NULL;
	g_autofree gchar *refine_flags_str = NULL;
	g_autoptr(GsApp) app = NULL;
	g_autoptr(GFile) file = NULL;
	g_autoptr(GsPluginLoader) plugin_loader = NULL;
	g_autoptr(AsProfileTask) ptask = NULL;
	const GOptionEntry options[] = {
		{ "show-results", '\0', 0, G_OPTION_ARG_NONE, &show_results,
		  "Show the results for the action", NULL },
		{ "refine-flags", '\0', 0, G_OPTION_ARG_STRING, &refine_flags_str,
		  "Set any refine flags required for the action", NULL },
		{ "repeat", '\0', 0, G_OPTION_ARG_INT, &repeat,
		  "Repeat the action this number of times", NULL },
		{ "cache-age", '\0', 0, G_OPTION_ARG_INT, &cache_age,
		  "Use this maximum cache age in seconds", NULL },
		{ "max-results", '\0', 0, G_OPTION_ARG_INT, &max_results,
		  "Return a maximum number of results", NULL },
		{ "prefer-local", '\0', 0, G_OPTION_ARG_NONE, &prefer_local,
		  "Prefer local file sources to AppStream", NULL },
		{ "plugin-blacklist", '\0', 0, G_OPTION_ARG_STRING, &plugin_blacklist_str,
		  "Do not load specific plugins", NULL },
		{ "plugin-whitelist", '\0', 0, G_OPTION_ARG_STRING, &plugin_whitelist_str,
		  "Only load specific plugins", NULL },
		{ "verbose", '\0', 0, G_OPTION_ARG_NONE, &verbose,
		  "Show verbose debugging information", NULL },
		{ "profile", '\0', 0, G_OPTION_ARG_NONE, &profile_enable,
		  "Show profiling information", NULL },
		{ NULL}
	};

	setlocale (LC_ALL, "");
	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gtk_init ();

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "GNOME Software Test Program");
	g_option_context_add_main_entries (context, options, NULL);
	ret = g_option_context_parse (context, &argc, &argv, &error);
	if (!ret) {
		g_print ("Failed to parse options: %s\n", error->message);
		goto out;
	}
	if (verbose)
		g_setenv ("GS_DEBUG", "1", TRUE);

	/* prefer local sources */
	if (prefer_local)
		g_setenv ("GNOME_SOFTWARE_PREFER_LOCAL", "true", TRUE);

	/* parse any refine flags */
	refine_flags = gs_cmd_parse_refine_flags (refine_flags_str, &error);
	if (refine_flags == G_MAXUINT64) {
		g_print ("Flag unknown: %s\n", error->message);
		goto out;
	}

	/* load plugins */
	plugin_loader = gs_plugin_loader_new ();
	profile = gs_plugin_loader_get_profile (plugin_loader);
	ptask = as_profile_start_literal (profile, "GsCmd");
	g_assert (ptask != NULL);
	if (g_file_test (LOCALPLUGINDIR, G_FILE_TEST_EXISTS))
		gs_plugin_loader_add_location (plugin_loader, LOCALPLUGINDIR);
	if (plugin_whitelist_str != NULL)
		plugin_whitelist = g_strsplit (plugin_whitelist_str, ",", -1);
	if (plugin_blacklist_str != NULL)
		plugin_blacklist = g_strsplit (plugin_blacklist_str, ",", -1);
	ret = gs_plugin_loader_setup (plugin_loader,
				      plugin_whitelist,
				      plugin_blacklist,
				      GS_PLUGIN_FAILURE_FLAGS_NONE,
				      NULL,
				      &error);
	if (!ret) {
		g_print ("Failed to setup plugins: %s\n", error->message);
		goto out;
	}
	gs_plugin_loader_dump_state (plugin_loader);

	/* ensure that at least some metadata of any age is present, and also
	 * spin up the plugins enough as to prime caches */
	if (g_getenv ("GS_CMD_NO_INITIAL_REFRESH") == NULL) {
		g_autoptr(GsPluginJob) plugin_job = NULL;
		plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_REFRESH,
						 "age", G_MAXUINT,
						 "refresh-flags", GS_PLUGIN_REFRESH_FLAGS_METADATA,
						 NULL);
		ret = gs_plugin_loader_job_action (plugin_loader, plugin_job,
						    NULL, &error);
		if (!ret) {
			g_print ("Failed to refresh plugins: %s\n", error->message);
			goto out;
		}
	}

	/* do action */
	if (argc == 2 && g_strcmp0 (argv[1], "installed") == 0) {
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_INSTALLED,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job,
							     NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 3 && g_strcmp0 (argv[1], "search") == 0) {
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_SEARCH,
							 "search", argv[2],
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job, NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 3 && g_strcmp0 (argv[1], "action-upgrade-download") == 0) {
		g_autoptr(GsPluginJob) plugin_job = NULL;
		app = gs_app_new (argv[2]);
		gs_app_set_kind (app, AS_APP_KIND_OS_UPGRADE);
		plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_UPGRADE_DOWNLOAD,
						 "app", app,
						 NULL);
		ret = gs_plugin_loader_job_action (plugin_loader, plugin_job,
						    NULL, &error);
		if (ret)
			gs_app_list_add (list, app);
	} else if (argc == 3 && g_strcmp0 (argv[1], "refine") == 0) {
		app = gs_app_new (argv[2]);
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_REFINE,
							 "app", app,
							 NULL);
			ret = gs_plugin_loader_job_action (plugin_loader, plugin_job,
							    NULL, &error);
			if (!ret)
				break;
		}
		list = gs_app_list_new ();
		gs_app_list_add (list, app);
	} else if (argc == 3 && g_strcmp0 (argv[1], "launch") == 0) {
		app = gs_app_new (argv[2]);
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_LAUNCH,
							 "app", app,
							 NULL);
			ret = gs_plugin_loader_job_action (plugin_loader, plugin_job,
							    NULL, &error);
			if (!ret)
				break;
		}
	} else if (argc == 3 && g_strcmp0 (argv[1], "filename-to-app") == 0) {
		g_autoptr(GsPluginJob) plugin_job = NULL;
		file = g_file_new_for_path (argv[2]);
		plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_FILE_TO_APP,
						 "file", file,
						 "refine-flags", refine_flags,
						 "max-results", max_results,
						 NULL);
		app = gs_plugin_loader_job_process_app (plugin_loader, plugin_job, NULL, &error);
		if (app == NULL) {
			ret = FALSE;
		} else {
			list = gs_app_list_new ();
			gs_app_list_add (list, app);
		}
	} else if (argc == 3 && g_strcmp0 (argv[1], "url-to-app") == 0) {
		g_autoptr(GsPluginJob) plugin_job = NULL;
		plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_URL_TO_APP,
						 "search", argv[2],
						 "refine-flags", refine_flags,
						 "max-results", max_results,
						 NULL);
		app = gs_plugin_loader_job_process_app (plugin_loader, plugin_job,
						    NULL, &error);
		if (app == NULL) {
			ret = FALSE;
		} else {
			list = gs_app_list_new ();
			gs_app_list_add (list, app);
		}
	} else if (argc == 2 && g_strcmp0 (argv[1], "updates") == 0) {
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_UPDATES,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job,
							     NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 2 && g_strcmp0 (argv[1], "upgrades") == 0) {
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_DISTRO_UPDATES,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job,
							     NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 2 && g_strcmp0 (argv[1], "sources") == 0) {
		g_autoptr(GsPluginJob) plugin_job = NULL;
		plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_SOURCES,
						 "refine-flags", refine_flags,
						 "max-results", max_results,
						 NULL);
		list = gs_plugin_loader_job_process (plugin_loader,
						     plugin_job,
						     NULL,
						     &error);
		if (list == NULL)
			ret = FALSE;
	} else if (argc == 2 && g_strcmp0 (argv[1], "popular") == 0) {
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_POPULAR,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job,
							     NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 2 && g_strcmp0 (argv[1], "featured") == 0) {
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_FEATURED,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job,
							      NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 2 && g_strcmp0 (argv[1], "recent") == 0) {
		if (cache_age == 0)
			cache_age = 60 * 60 * 24 * 60;
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_RECENT,
							 "age", cache_age,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job,
							     NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 2 && g_strcmp0 (argv[1], "get-categories") == 0) {
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (categories != NULL)
				g_ptr_array_unref (categories);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_CATEGORIES,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			categories = gs_plugin_loader_job_get_categories (plugin_loader,
									 plugin_job,
									 NULL, &error);
			if (categories == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc == 3 && g_strcmp0 (argv[1], "get-category-apps") == 0) {
		g_autoptr(GsCategory) category = NULL;
		g_autoptr(GsCategory) parent = NULL;
		g_auto(GStrv) split = NULL;
		split = g_strsplit (argv[2], "/", 2);
		if (g_strv_length (split) == 1) {
			category = gs_category_new (split[0]);
		} else {
			parent = gs_category_new (split[0]);
			category = gs_category_new (split[1]);
			gs_category_add_child (parent, category);
		}
		for (i = 0; i < repeat; i++) {
			g_autoptr(GsPluginJob) plugin_job = NULL;
			if (list != NULL)
				g_object_unref (list);
			plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_GET_CATEGORY_APPS,
							 "category", category,
							 "refine-flags", refine_flags,
							 "max-results", max_results,
							 NULL);
			list = gs_plugin_loader_job_process (plugin_loader, plugin_job, NULL, &error);
			if (list == NULL) {
				ret = FALSE;
				break;
			}
		}
	} else if (argc >= 2 && g_strcmp0 (argv[1], "refresh") == 0) {
		GsPluginRefreshFlags refresh_flags;
		g_autoptr(GsPluginJob) plugin_job = NULL;
		refresh_flags = gs_cmd_refresh_flag_from_string (argv[2]);
		plugin_job = gs_plugin_job_newv (GS_PLUGIN_ACTION_REFRESH,
						 "age", cache_age,
						 "refresh-flags", refresh_flags,
						 NULL);
		ret = gs_plugin_loader_job_action (plugin_loader, plugin_job,
						    NULL, &error);
	} else {
		ret = FALSE;
		g_set_error_literal (&error,
				     GS_PLUGIN_ERROR,
				     GS_PLUGIN_ERROR_FAILED,
				     "Did not recognise option, use 'installed', "
				     "'updates', 'popular', 'get-categories', "
				     "'get-category-apps', 'filename-to-app', "
				     "'sources', 'refresh', 'launch' or 'search'");
	}
	if (!ret) {
		g_print ("Failed: %s\n", error->message);
		goto out;
	}

	if (show_results) {
		if (list != NULL)
			gs_cmd_show_results_apps (list);
		if (categories != NULL)
			gs_cmd_show_results_categories (categories);
	}
out:
	if (profile_enable)
		as_profile_dump (profile);
	g_option_context_free (context);
	return status;
}

/* vim: set noexpandtab: */
