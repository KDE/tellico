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

IF( KDE4_BUILD_TESTS )
  ADD_SUBDIRECTORY( tests )
ENDIF( KDE4_BUILD_TESTS )

IF( ENABLE_WEBCAM )
  ADD_SUBDIRECTORY( barcode )
ENDIF( ENABLE_WEBCAM )

########### next target ###############

SET(tellico_SRCS
   borrower.cpp
   borrowerdialog.cpp
   collection.cpp
   collectionfactory.cpp
   collectionfieldsdialog.cpp
   configdialog.cpp
   controller.cpp
   detailedlistview.cpp
   document.cpp
   entry.cpp
   entryeditdialog.cpp
   entrygroup.cpp
   entryiconview.cpp
   entrycomparison.cpp
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
   groupiterator.cpp
   groupview.cpp
   importdialog.cpp
   loandialog.cpp
   loanview.cpp
   main.cpp
   mainwindow.cpp
   progressmanager.cpp
   reportdialog.cpp
   statusbar.cpp
   tellico_kernel.cpp
   tellico_utils.cpp
   viewstack.cpp
   )

KDE4_ADD_EXECUTABLE(tellico ${tellico_SRCS})

TARGET_LINK_LIBRARIES(tellico
    core
    cite
    fetch
    collections
    images
    translators
    gui
    commands
    models
    utils
    newstuff
    pilotdb
    rtf2html
    btparse
    csv
    )

TARGET_LINK_LIBRARIES(tellico
    ${QT_AND_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_KHTML_LIBS}
    ${KDE4_KNEWSTUFF2_LIBS}
    ${KDE4_SOLID_LIBS}
    ${QIMAGEBLITZ_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${LIBEXSLT_LIBRARIES}
    )

IF( KABC_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${KABC_LIBRARIES})
ENDIF( KABC_FOUND )

IF( KCAL_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${KCAL_LIBRARIES})
ENDIF( KCAL_FOUND )

IF( KCDDB_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${KCDDB_LIBRARIES})
ENDIF( KCDDB_FOUND)

IF( POPPLER_QT4_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${POPPLER_QT4_LIBRARIES})
ENDIF( POPPLER_QT4_FOUND )

IF( LIBEXEMPI_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${LIBEXEMPI_LIBRARIES})
ENDIF( LIBEXEMPI_FOUND )

IF( TAGLIB_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${TAGLIB_LIBRARIES})
ENDIF( TAGLIB_FOUND )

IF( YAZ_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${YAZ_LIBRARIES})
ENDIF( YAZ_FOUND )

IF( ENABLE_WEBCAM )
  TARGET_LINK_LIBRARIES(tellico barcode )
ENDIF( ENABLE_WEBCAM )

IF( KSANE_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${KSANE_LIBRARY})
ENDIF( KSANE_FOUND )

IF( QCA2_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${QCA2_LIBRARIES})
ENDIF( QCA2_FOUND )

INSTALL( TARGETS tellico ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

INSTALL(FILES   tellicorc DESTINATION ${CONFIG_INSTALL_DIR} )
INSTALL(FILES   tellicoui.rc DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
