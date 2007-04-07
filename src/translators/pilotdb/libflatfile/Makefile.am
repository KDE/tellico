####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = liblibflatfile.a

AM_CPPFLAGS = $(all_includes)

liblibflatfile_a_METASOURCES = AUTO

liblibflatfile_a_SOURCES = DB.cpp Database.cpp


EXTRA_DIST = Database.cpp Database.h DB.cpp DB.h Field.h FType.h ListView.h ListViewColumn.h Record.h

####### kdevelop will overwrite this part!!! (end)############

# is this the right way to do this? I need to include the strop.o object file since its
# in the parent directory
liblibflatfile_a_LIBADD = ../strop.o
CLEANFILES = strop.Po

KDE_OPTIONS = noautodist
