AM_CPPFLAGS =						\
	-DG_LOG_DOMAIN=\"GsPluginApt\"

plugindir = $(GS_PLUGIN_DIR)
plugin_LTLIBRARIES = libgs_plugin_apt.la

libgs_plugin_apt_la_SOURCES =				\
	gs-plugin-apt.cc
libgs_plugin_apt_la_LIBADD =				\
	$(GS_PLUGIN_LIBS)				\
	$(GIO_UNIX_LIBS)				\
	-lapt-pkg
libgs_plugin_apt_la_LDFLAGS = -module -avoid-version
libgs_plugin_apt_la_CPPFLAGS =				\
	$(GS_PLUGIN_CFLAGS)				\
	$(GIO_UNIX_CFLAGS)				\
	$(WARN_CFLAGS)

%.metainfo.xml: %.metainfo.xml.in
	$(AM_V_GEN) msgfmt --xml -d $(top_srcdir)/po --template $< -o $@

appdata_in_files = org.gnome.Software.Plugin.Apt.metainfo.xml.in
appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
EXTRA_DIST = $(appdata_in_files)
CLEANFILES = $(appdata_DATA)

-include $(top_srcdir)/git.mk
