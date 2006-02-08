####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = Column_View.xsl Title_Listing_(Horizontal).xsl\
	Title_Listing_(Vertical).xsl Group_View.xsl Image_List.xsl \
	Loan_View.xsl Group_Summary.xsl Tri-Column.xsl

####### kdevelop will overwrite this part!!! (end)############

xsldir = $(kde_datadir)/tellico/report-templates
xsl_DATA = Column_View.xsl Title_Listing_(Horizontal).xsl \
	Title_Listing_(Vertical).xsl Group_View.xsl Image_List.xsl \
	Loan_View.xsl Group_Summary.xsl Tri-Column.xsl

KDE_OPTIONS = noautodist

CLEANFILES = *~

# probably a better way to do this
uninstall-hook:
	-if [ -d $(xsldir) ]; then rmdir $(xsldir); fi
