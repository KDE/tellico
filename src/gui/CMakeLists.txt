
########### next target ###############

SET(gui_STAT_SRCS
   boolfieldwidget.cpp
   choicefieldwidget.cpp
   collectiontypecombo.cpp
   combobox.cpp
   countdelegate.cpp
   cursorsaver.cpp
   datefieldwidget.cpp
   datewidget.cpp
   guiproxy.cpp
   imagefieldwidget.cpp
   imagewidget.cpp
   fieldwidget.cpp
   kwidgetlister.cpp
   lineedit.cpp
   linefieldwidget.cpp
   listwidgetitem.cpp
   numberfieldwidget.cpp
   parafieldwidget.cpp
   previewdialog.cpp
   progress.cpp
   ratingfieldwidget.cpp
   ratingwidget.cpp
   spinbox.cpp
   statusbar.cpp
   stringmapdialog.cpp
   tablefieldwidget.cpp
   tabwidget.cpp
   treeview.cpp
   urlfieldwidget.cpp
   )

# The KDateComboBox class was added to KDE in version 4.7
# If we're building under an earlier version, use a local copy
IF(${KDE_VERSION} VERSION_LESS "4.6.99")
  LIST(APPEND gui_STAT_SRCS kdatecombobox.cpp klocalizeddate.cpp)
ENDIF(${KDE_VERSION} VERSION_LESS "4.6.99")

KDE4_ADD_LIBRARY(gui STATIC ${gui_STAT_SRCS})
