####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = Column_View.xsl Title_Listing_(Horizontal).xsl\
	Title_Listing_(Vertical).xsl Group_View.xsl Image_List.xsl \
	Loan_View.xsl Group_Summary.xsl

####### kdevelop will overwrite this part!!! (end)############

xsldir = $(kde_datadir)/tellico/report-templates
xsl_DATA = Column_View.xsl Title_Listing_(Horizontal).xsl \
	Title_Listing_(Vertical).xsl Group_View.xsl Image_List.xsl \
	Loan_View.xsl Group_Summary.xsl

KDE_OPTIONS = noautodist

# probably a better way to do this
uninstall-hook:
	-if [ -d $(xsldir) ]; then rmdir $(xsldir); fi
