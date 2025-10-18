add_subdirectory(core)
add_subdirectory(config)
add_subdirectory(gui)
add_subdirectory(collections)
add_subdirectory(images)
add_subdirectory(translators)
add_subdirectory(fetch)
add_subdirectory(commands)
add_subdirectory(cite)
add_subdirectory(newstuff)
add_subdirectory(models)
add_subdirectory(utils)
add_subdirectory(3rdparty)

if(Qt${QT_MAJOR_VERSION}Charts_FOUND)
    add_subdirectory(charts)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(ENABLE_WEBCAM)
    add_subdirectory(barcode)
endif()

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

set(tellico_SRCS
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
    filterparser.cpp
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

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(tellico PRIVATE -Werror=undef)
endif()

target_link_libraries(tellico
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

target_link_libraries(tellico
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

if(Qt${QT_MAJOR_VERSION}Charts_FOUND)
    target_link_libraries(tellico charts)
endif()

if(USE_KHTML)
    target_link_libraries(tellico KF${KF_MAJOR_VERSION}::KHtml)
    add_definitions(-DUSE_KHTML=${USE_KHTML})
else()
    target_link_libraries(tellico Qt${QT_MAJOR_VERSION}::WebEngineWidgets)
endif()

if(KF${KF_MAJOR_VERSION}NewStuff_FOUND)
    if(KF${KF_MAJOR_VERSION}_VERSION VERSION_GREATER_EQUAL "5.91.0")
    target_link_libraries(tellico KF${KF_MAJOR_VERSION}::NewStuffWidgets)
    else()
    target_link_libraries(tellico KF${KF_MAJOR_VERSION}::NewStuff)
    endif()
endif()

if(KDEPIMLIBS_FOUND)
    target_link_libraries(tellico
        KF${KF_MAJOR_VERSION}::Contacts
        ${KDEPIMLIBS_KCAL_LIBS}
        KF${KF_MAJOR_VERSION}::AkonadiContact
    )
endif()

if(${KCDDB_TARGET_NAME}_FOUND)
    target_link_libraries(tellico ${KCDDB_TARGET_NAME})
endif()

if(Exempi_FOUND)
    target_link_libraries(tellico ${Exempi_LIBRARIES})
endif()

if(CDIO_FOUND)
    target_link_libraries(tellico ${CDIO_LIBRARIES})
endif()

if(TAGLIB_FOUND)
    target_link_libraries(tellico ${TAGLIB_LIBRARIES})
endif()

if(Yaz_FOUND)
    target_link_libraries(tellico ${Yaz_LIBRARIES})
endif()

if(ENABLE_WEBCAM)
    target_link_libraries(tellico barcode ${LIBV4L_LIBRARIES})
endif()

if(${KSANE_TARGET_NAME}_FOUND)
    target_link_libraries(tellico ${KSANE_TARGET_NAME})
endif()

########### install files ###############

install(TARGETS tellico ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES tellicorc DESTINATION ${KDE_INSTALL_CONFDIR})
