
lib_LTLIBRARIES = replyto.la

replyto_la_SOURCES = \
	replyto.c \
	replyto.h

replyto_la_LDFLAGS = -module -no-undefined -avoid-version

AM_CPPFLAGS = \
	$(GTK_CFLAGS) \
	-I$(includedir) \
	-I$(sylpheed_build_dir)/libsylph \
	-I$(sylpheed_build_dir)/src \
	-I$(sylplugin_factory_source_dir)/src \
	$(SYLPF_CFLAGS)


replyto_la_LIBADD = \
	$(GTK_LIBS) \
	$(sylpheed_build_dir)/src/libsylpheed-plugin-0.la \
	$(sylpheed_build_dir)/libsylph/libsylph-0.la \
	$(SYLPF_LIBS)

install:
install-plugin: install-am
