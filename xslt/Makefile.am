####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = bookcase-by-author.xsl bookcase-by-title.xsl bookcase-printing.xsl bookcase2bibtex.xsl bookcase2bibtexml.xsl bibtexml2bookcase.xsl 

install-data-local:
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase-by-author.xsl $(kde_datadir)/bookcase/bookcase-by-author.xsl
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase-by-title.xsl $(kde_datadir)/bookcase/bookcase-by-title.xsl
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase-printing.xsl $(kde_datadir)/bookcase/bookcase-printing.xsl
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase2bibtex.xsl $(kde_datadir)/bookcase/bookcase2bibtex.xsl
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase2bibtexml.xsl $(kde_datadir)/bookcase/bookcase2bibtexml.xsl
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bibtexml2bookcase.xsl $(kde_datadir)/bookcase/bibtexml2bookcase.xsl

uninstall-local:
	-rm -f $(kde_datadir)/bookcase/bookcase-by-author.xsl
	-rm -f $(kde_datadir)/bookcase/bookcase-by-title.xsl
	-rm -f $(kde_datadir)/bookcase/bookcase-printing.xsl
	-rm -f $(kde_datadir)/bookcase/bookcase2bibtex.xsl
	-rm -f $(kde_datadir)/bookcase/bookcase2bibtexml.xsl
	-rm -f $(kde_datadir)/bookcase/bibtexml2bookcase.xsl

####### kdevelop will overwrite this part!!! (end)############
