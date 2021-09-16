set(SOURCES
    main.cpp
    
    vsg/ReaderWriterRAW.cpp
    vsg/ReaderWriterRegion.cpp
    vsg/ReaderWriterSiegeNodeList.cpp
    vsg/ReaderWriterSNO.cpp
    
    io/FileNameMap.cpp
    io/Fuel.cpp
    io/FuelParser.cpp
    io/FuelScanner.cpp
    io/LocalFileSys.cpp
    
    cfg/ArgsConfig.cpp
    cfg/RegistryConfig.cpp
    cfg/SteamConfig.cpp
    cfg/UserConfig.cpp
    cfg/WritableConfig.cpp
    
    #QOSGWidget.cpp
)

set(HEADERS
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# automoc doens't even work
#set(CMAKE_AUTOUIC OFF)
#set(UI ${CMAKE_CURRENT_LIST_DIR}/editor.ui)
#qt5_wrap_ui(UI_HEADERS ${UI})

add_executable(vsgqtviewer ${SOURCES} ${UI_HEADERS} ${HEADERS} ${FORMS})

target_include_directories(vsgqtviewer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} extern ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vsgqtviewer vsgQt)

if(MSVC)
    target_compile_options(vsgqtviewer PRIVATE "/MP")

    #source_group(TREE ${CMAKE_SOURCE_DIR}/src FILES ${SOURCES})
endif()


install(TARGETS vsgqtviewer DESTINATION bin)

