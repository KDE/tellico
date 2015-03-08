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
   bibtexkeydialog.cpp
   borrower.cpp
   borrowerdialog.cpp
   collection.cpp
   collectionfactory.cpp
   collectionfieldsdialog.cpp
   configdialog.cpp
   controller.cpp
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
   tellico_utils.cpp
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
    pilotdb-tellico
    rtf2html-tellico
    ${TELLICO_BTPARSE_LIBS}
    ${TELLICO_CSV_LIBS}
    )

TARGET_LINK_LIBRARIES(tellico
    
    
    KF5::KIOCore
    KF5::KHtml
    
    ${QIMAGEBLITZ_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${LIBEXSLT_LIBRARIES}
    )

IF( ENABLE_KNEWSTUFF3 )
  TARGET_LINK_LIBRARIES(tellico KF5::NewStuff)
ELSE( ENABLE_KNEWSTUFF3 )
  TARGET_LINK_LIBRARIES(tellico ${KDE4_KNEWSTUFF2_LIBS})
ENDIF( ENABLE_KNEWSTUFF3 )

IF( KDEPIMLIBS_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF5::Contacts ${KDEPIMLIBS_KCAL_LIBS} KF5::AkonadiContact)
ENDIF( KDEPIMLIBS_FOUND )

IF( KDEMULTIMEDIA_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${KCDDB_LIBRARY})
ENDIF( KDEMULTIMEDIA_FOUND)

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
  TARGET_LINK_LIBRARIES(tellico barcode ${LIBV4L_LIBRARIES})
ENDIF( ENABLE_WEBCAM )

IF( KSANE_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF5::Sane)
ENDIF( KSANE_FOUND )

INSTALL( TARGETS tellico ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

INSTALL(FILES   tellicorc DESTINATION ${KDE_INSTALL_CONFDIR} )
INSTALL(FILES   tellicoui.rc DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
