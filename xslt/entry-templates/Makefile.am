####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = Default.xsl Video.xsl Album.xsl Fancy.xsl Compact.xsl

####### kdevelop will overwrite this part!!! (end)############

xsldir = $(kde_datadir)/bookcase/entry-templates
xsl_DATA = Default.xsl Video.xsl Album.xsl Fancy.xsl Compact.xsl

KDE_OPTIONS = noautodist

# probably a better way to do this
uninstall-hook:
	-if [ -d $(xsldir) ]; then rmdir $(xsldir); fi
