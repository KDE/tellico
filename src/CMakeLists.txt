ADD_SUBDIRECTORY( core )
ADD_SUBDIRECTORY( config )
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

IF( Qt${QT_MAJOR_VERSION}Charts_FOUND )
  ADD_SUBDIRECTORY( charts )
ENDIF( Qt${QT_MAJOR_VERSION}Charts_FOUND )

IF( BUILD_TESTING )
  ADD_SUBDIRECTORY( tests )
ENDIF( BUILD_TESTING )

IF( ENABLE_WEBCAM )
  ADD_SUBDIRECTORY( barcode )
ENDIF( ENABLE_WEBCAM )

########### next target ###############

set(ICONS_PNG
  ${CMAKE_SOURCE_DIR}/icons/16-apps-tellico.png
  ${CMAKE_SOURCE_DIR}/icons/22-apps-tellico.png
  ${CMAKE_SOURCE_DIR}/icons/32-apps-tellico.png
  ${CMAKE_SOURCE_DIR}/icons/48-apps-tellico.png
  ${CMAKE_SOURCE_DIR}/icons/64-apps-tellico.png
  ${CMAKE_SOURCE_DIR}/icons/128-apps-tellico.png
)

ecm_add_app_icon(ICONS_SOURCES ICONS ${ICONS_PNG})

SET(tellico_SRCS
   ${ICONS_SOURCES}
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
   printhandler.cpp
   progressmanager.cpp
   reportdialog.cpp
   tellico_debug.cpp
   tellico_kernel.cpp
   viewstack.cpp

   tellico.qrc
   )

add_executable(tellico ${tellico_SRCS} ../icons/icons.qrc)

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
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Widgets
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_MAJOR_VERSION}::PrintSupport
    KF${KF_MAJOR_VERSION}::Crash
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::KIOFileWidgets
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${LIBXSLT_EXSLT_LIBRARIES}
    )

IF( Qt${QT_MAJOR_VERSION}Charts_FOUND )
  TARGET_LINK_LIBRARIES(tellico charts)
ENDIF( Qt${QT_MAJOR_VERSION}Charts_FOUND )

IF(USE_KHTML)
  TARGET_LINK_LIBRARIES(tellico KF${KF_MAJOR_VERSION}::KHtml)
  ADD_DEFINITIONS(-DUSE_KHTML=${USE_KHTML})
ELSE()
  TARGET_LINK_LIBRARIES(tellico Qt${QT_MAJOR_VERSION}::WebEngineWidgets)
ENDIF()

IF( KF${KF_MAJOR_VERSION}NewStuff_FOUND )
  IF(KF${KF_MAJOR_VERSION}_VERSION VERSION_GREATER_EQUAL "5.91.0")
    TARGET_LINK_LIBRARIES(tellico KF${KF_MAJOR_VERSION}::NewStuffWidgets)
  ELSE()
    TARGET_LINK_LIBRARIES(tellico KF${KF_MAJOR_VERSION}::NewStuff)
  ENDIF()
ENDIF( KF${KF_MAJOR_VERSION}NewStuff_FOUND )

IF( KDEPIMLIBS_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF${KF_MAJOR_VERSION}::Contacts ${KDEPIMLIBS_KCAL_LIBS} KF${KF_MAJOR_VERSION}::AkonadiContact)
ENDIF( KDEPIMLIBS_FOUND )

IF( ${KCDDB_TARGET_NAME}_FOUND )
  TARGET_LINK_LIBRARIES(tellico ${KCDDB_TARGET_NAME})
ELSEIF( Libkcddb_FOUND )
  TARGET_LINK_LIBRARIES(tellico KDE::Libkcddb::kcddb)
ENDIF( ${KCDDB_TARGET_NAME}_FOUND)

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

IF( KF${KF_MAJOR_VERSION}Sane_FOUND )
  TARGET_LINK_LIBRARIES(tellico KF${KF_MAJOR_VERSION}::Sane)
ENDIF( KF${KF_MAJOR_VERSION}Sane_FOUND )

########### install files ###############

INSTALL(TARGETS tellico ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
INSTALL(FILES tellicorc DESTINATION ${KDE_INSTALL_CONFDIR} )
