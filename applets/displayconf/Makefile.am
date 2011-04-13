bin_PROGRAMS = mex-displayconf
noinst_PROGRAMS = test-message

mex_displayconf_SOURCES = displayconf.c message.h message.c
mex_displayconf_CFLAGS =				\
	$(DISPLAYCONF_CFLAGS)			\
	$(COMPILER_FLAGS) 			\
	-DG_LOG_DOMAIN=\"MexDisplayConf\"
mex_displayconf_LDADD = $(DISPLAYCONF_LIBS)


test_message_SOURCES = message.c
test_message_CFLAGS =				\
	$(DISPLAYCONF_CFLAGS)			\
	$(COMPILER_FLAGS) 			\
	-DG_LOG_DOMAIN=\"MexDisplayConf\"	\
	-DSTANDALONE
test_message_LDADD = $(DISPLAYCONF_LIBS)
