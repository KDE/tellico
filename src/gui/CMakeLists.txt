
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

add_library(gui STATIC ${gui_STAT_SRCS})

TARGET_LINK_LIBRARIES(gui
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    KF5::KIOCore
    KF5::KHtml
    KF5::SonnetCore
    KF5::SonnetUi
    KF5::KDELibs4Support
)
