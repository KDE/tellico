####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = summary.xsl bookcase-by-author.xsl bookcase-by-title.xsl 

install-data-local:
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase-by-author.xsl $(kde_datadir)/bookcase/bookcase-by-author.xsl
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase-by-title.xsl $(kde_datadir)/bookcase/bookcase-by-title.xsl

uninstall-local:
	-rm -f $(kde_datadir)/bookcase/bookcase-by-author.xsl
	-rm -f $(kde_datadir)/bookcase/bookcase-by-title.xsl

####### kdevelop will overwrite this part!!! (end)############
