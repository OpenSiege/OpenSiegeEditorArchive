
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
)

set(HEADERS
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(vsgqtviewer ${SOURCES} ${UI_HEADERS} ${HEADERS} ${FORMS})

target_include_directories(vsgqtviewer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} extern ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vsgqtviewer vsgQt)

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    target_compile_options(vsgqtviewer PRIVATE "/MP")
endif()


install(TARGETS vsgqtviewer DESTINATION bin)

