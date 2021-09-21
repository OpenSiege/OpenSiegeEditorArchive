

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

add_executable(siege-editor ${SOURCES} ${UI_HEADERS} ${HEADERS} ${FORMS})

target_include_directories(siege-editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} extern ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(siege-editor vsgQt)

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    
    foreach(FILE ${SOURCES}) 
        get_filename_component(PARENT_DIR "${FILE}" PATH)

        string(REPLACE "/" "\\" GROUP "${PARENT_DIR}")

        if ("${FILE}" MATCHES ".*\\.cpp")
            set(GROUP "Source Files\\${GROUP}")
        elseif("${FILE}" MATCHES ".*\\.hpp")
            set(GROUP "Header Files\\${GROUP}")
        endif()

        source_group("${GROUP}" FILES "${FILE}")
    endforeach()

    target_compile_options(siege-editor PRIVATE "/MP")
endif()


install(TARGETS siege-editor DESTINATION bin)

