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

add_executable(vsgqtviewer ${MODE} ${SOURCES} ${HEADERS} ${FORMS})

target_include_directories(vsgqtviewer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} extern)

target_link_libraries(vsgqtviewer vsgQt)

if(MSVC)
    target_compile_options(vsgqtviewer PRIVATE "/MP")

    #source_group(TREE ${CMAKE_SOURCE_DIR}/src FILES ${SOURCES})
endif()


install(TARGETS vsgqtviewer DESTINATION bin)

