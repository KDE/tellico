
########### next target ###############

SET(gui_STAT_SRCS
   boolfieldwidget.cpp
   choicefieldwidget.cpp
   collectiontypecombo.cpp
   combobox.cpp
   countdelegate.cpp
   datefieldwidget.cpp
   datewidget.cpp
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
   richtextlabel.cpp
   stringmapdialog.cpp
   tablefieldwidget.cpp
   tabwidget.cpp
   treeview.cpp
   urlfieldwidget.cpp
   )

KDE4_ADD_LIBRARY(gui STATIC ${gui_STAT_SRCS})
