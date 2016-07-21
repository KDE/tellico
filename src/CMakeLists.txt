ADD_SUBDIRECTORY( core )
ADD_SUBDIRECTORY( gui )
ADD_SUBDIRECTORY( collections )
ADD_SUBDIRECTORY( images )
ADD_SUBDIRECTORY( translators )
ADD_SUBDIRECTORY( fetch )
ADD_SUBDIRECTORY( commands )
ADD_SUBDIRECTORY( cite )
ADD_SUBDIRECTORY( newstuff )
ADD_SUBDIRECTORY( models )
ADD_SUBDIRECTORY( utils )
ADD_SUBDIRECTORY( 3rdparty )

IF( BUILD_TESTS )
  ADD_SUBDIRECTORY( tests )
ENDIF( BUILD_TESTS )

IF( ENABLE_WEBCAM )
  ADD_SUBDIRECTORY( barcode )
ENDIF( ENABLE_WEBCAM )

########### next target ###############

SET(tellico_SRCS
   bibtexkeydialog.cpp
   borrower.cpp
   borrowerdialog.cpp
   collection.cpp
   collectionfactory.cpp
   collectionfieldsdialog.cpp
   configdialog.cpp
   controller.cpp
   dbusinterface.cpp
   detailedlistview.cpp
   derivedvalue.cpp
   document.cpp
   entry.cpp
   entryeditdialog.cpp
   entrygroup.cpp
   entryiconview.cpp
   entrycomparison.cpp
   entrymatchdialog.cpp
   entrymerger.cpp
   entryupdatejob.cpp
   entryupdater.cpp
   entryview.cpp
   exportdialog.cpp
   fetchdialog.cpp
   fetcherconfigdialog.cpp
   field.cpp
   fieldcompletion.cpp
   fieldformat.cpp
   filter.cpp
   filterdialog.cpp
   filterview.cpp
   groupview.cpp
   importdialog.cpp
   loandialog.cpp
   loanview.cpp
   main.cpp
   mainwindow.cpp
   progressmanager.cpp
   reportdialog.cpp
   tellico_kernel.cpp
   viewstack.cpp
   )

add_executable(tellico ${tellico_SRCS})

TARGET_LINK_LIBRARIES(tellico
    core
    cite
    fetch
    collections
    images
    translators
    gui
    commands
    tellicomodels
    utils
    newstuff
    rtf2html-tellico
    ${TELLICO_BTPARSE_LIBS}
    ${TELLICO_CSV_LIBS}
    )

TARGET_LINK_LIBRARIES(tellico
    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    KF5::Crash
    KF5::KHtml
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::WindowSystem
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${LIBXSLT_EXSLT_LIBRARIES}
    )

IF( KF5NewStuff_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF5::NewStuff)
ENDIF( KF5NewStuff_FOUND )

IF( KDEPIMLIBS_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF5::Contacts ${KDEPIMLIBS_KCAL_LIBS} KF5::AkonadiContact)
ENDIF( KDEPIMLIBS_FOUND )

IF( KF5Cddb_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF5::Cddb)
ELSEIF( Libkcddb_FOUND )
  TARGET_LINK_LIBRARIES(tellico KDE::Libkcddb::kcddb)
ENDIF( KF5Cddb_FOUND)

IF( Exempi_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${Exempi_LIBRARIES})
ENDIF( Exempi_FOUND )

IF( TAGLIB_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${TAGLIB_LIBRARIES})
ENDIF( TAGLIB_FOUND )

IF( Yaz_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${Yaz_LIBRARIES})
ENDIF( Yaz_FOUND )

IF( ENABLE_WEBCAM )
  TARGET_LINK_LIBRARIES(tellico barcode ${LIBV4L_LIBRARIES})
ENDIF( ENABLE_WEBCAM )

IF( KF5Sane_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF5::Sane)
ENDIF( KF5Sane_FOUND )

########### install files ###############

INSTALL(TARGETS tellico ${INSTALL_TARGETS_DEFAULT_ARGS} )
INSTALL(FILES tellicorc DESTINATION ${KDE_INSTALL_CONFDIR} )
INSTALL(FILES tellicoui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/tellico )
