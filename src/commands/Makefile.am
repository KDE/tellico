INCLUDES = $(all_includes)

noinst_LIBRARIES = libcommands.a
libcommands_a_SOURCES = \
	addentries.cpp modifyentries.cpp removeentries.cpp \
	addloans.cpp modifyloans.cpp removeloans.cpp \
	fieldcommand.cpp filtercommand.cpp reorderfields.cpp \
	group.cpp collectioncommand.cpp renamecollection.cpp \
	updateentries.cpp

libcommands_a_METASOURCES = AUTO
KDE_OPTIONS = noautodist
EXTRA_DIST = \
	addentries.h addentries.cpp \
	modifyentries.h modifyentries.cpp \
	removeentries.h removeentries.cpp \
	addloans.h addloans.cpp \
	modifyloans.h modifyloans.cpp \
	removeloans.h removeloans.cpp \
	fieldcommand.h fieldcommand.cpp \
	filtercommand.h filtercommand.cpp \
	reorderfields.h reorderfields.cpp \
	group.h group.cpp \
	collectioncommand.h collectioncommand.cpp \
	renamecollection.h renamecollection.cpp \
	updateentries.h updateentries.cpp


CLEANFILES = *~
