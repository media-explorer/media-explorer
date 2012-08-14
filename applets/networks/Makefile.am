bin_PROGRAMS = mex-networks

connman-manager-introspection.h: $(srcdir)/introspection/connman-manager.xml
	/bin/sh $(top_srcdir)/build/stringify.sh $< $@ > $@
connman-service-introspection.h: $(srcdir)/introspection/connman-service.xml
	/bin/sh $(top_srcdir)/build/stringify.sh $< $@ > $@

BUILT_SOURCES = connman-manager-introspection.h \
                connman-service-introspection.h

DISTCLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = introspection/connman-manager.xml \
             introspection/connman-service.xml

mex_networks_SOURCES = main.c \
                            mtn-app.c mtn-app.h \
                            mtn-services-view.c mtn-services-view.h \
                            mtn-service-tile.c mtn-service-tile.h \
                            mtn-service-model.c mtn-service-model.h \
                            mtn-button-box.c mtn-button-box.h \
                            mtn-connman-service.c mtn-connman-service.h \
                            mtn-connman.c mtn-connman.h \
                            $(BUILT_SOURCES)


mex_networks_CFLAGS = \
                           -I$(top_srcdir) \
                           -I$(top_srcdir)/mex \
                           $(MEX_CFLAGS) \
                           $(NETWORKS_CFLAGS) \
                           -DG_LOG_DOMAIN=\"MexNetworks\" \
                           -DTHEMEDIR=\"$(pkgdatadir)/networks\"


mex_networks_LDADD= $(MEX_LIBS) \
                         $(NETWORKS_LIBS) \
                         $(top_builddir)/mex/libmex-@MEX_API_VERSION@.la


-include $(top_srcdir)/git.mk
