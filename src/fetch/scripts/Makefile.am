####### kdevelop will overwrite this part!!! (begin)##########

EXTRA_DIST = \
fr.allocine.py fr.allocine.py.spec \
ministerio_de_cultura.py ministerio_de_cultura.py.spec \
dark_horse_comics.py dark_horse_comics.py.spec

####### kdevelop will overwrite this part!!! (end)############

scriptdir = $(kde_datadir)/tellico/data-sources
script_SCRIPTS = \
fr.allocine.py \
ministerio_de_cultura.py \
dark_horse_comics.py
script_DATA = \
fr.allocine.py.spec \
ministerio_de_cultura.py.spec \
dark_horse_comics.py.spec

KDE_OPTIONS = noautodist

CLEANFILES = *~

# probably a better way to do this
uninstall-hook:
	-if [ -d $(scriptdir) ]; then rmdir $(scriptdir); fi
