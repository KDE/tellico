####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = tellico.png book.png hi16-app-tellico.png hi32-app-tellico.png person-open.png person.png video.png album.png bibtex.png coin.png card.png stamp.png wine.png comic.png

####### kdevelop will overwrite this part!!! (end)############

KDE_OPTIONS = noautodist

icondir = ${kde_icondir}
icon_ICON = tellico

picdir = ${kde_datadir}/tellico/pics
pic_DATA = book.png tellico.png person.png person-open.png album.png video.png bibtex.png coin.png card.png stamp.png wine.png comic.png

# probably a better way to do this
uninstall-hook:
	-if [ -d $(picdir) ]; then rmdir $(picdir); fi
