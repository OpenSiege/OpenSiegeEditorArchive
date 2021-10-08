

macro(add_target_clang_format)
    set(options)
    set(oneValueArgs )
    set(multiValueArgs FILES EXCLUDE)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    find_program(CLANGFORMAT clang-format)
    if (CLANGFORMAT)
        file(GLOB FILES_TO_FORMAT
            ${ARGS_FILES}
        )
        foreach(EXCLUDE ${ARGS_EXCLUDES})
            list(REMOVE_ITEM FILES_TO_FORMAT ${EXCLUDE})
        endforeach()
        add_custom_target(clang-format-editor
            COMMAND ${CLANGFORMAT} -i ${FILES_TO_FORMAT}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Automated code format using clang-format"
        )
    endif()
endmacro()

set(SOURCES
    main.cpp
   
    vsg/ReaderWriterRAW.cpp
    vsg/ReaderWriterRegion.cpp
    vsg/ReaderWriterSiegeNodeList.cpp
    vsg/ReaderWriterSNO.cpp 
    vsg/Aspect.cpp
    vsg/ReaderWriterASP.cpp
   
    io/FileNameMap.cpp
    io/Fuel.cpp
    io/FuelParser.cpp
    io/FuelScanner.cpp
    io/LocalFileSys.cpp
    io/BinaryReader.cpp

    cfg/ArgsConfig.cpp
    cfg/RegistryConfig.cpp
    cfg/SteamConfig.cpp
    cfg/UserConfig.cpp
    cfg/WritableConfig.cpp

    world/SiegeNode.cpp
    
    MainWindow.cpp
    SiegePipeline.cpp
    LoadMapDialog.cpp
)

set(HEADERS
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(siege-editor ${SOURCES} ${UI_HEADERS} ${HEADERS} ${FORMS})

target_include_directories(siege-editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} extern ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(siege-editor vsgQt)

add_target_clang_format(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cfg/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cfg/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/io/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/io/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vsg/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vsg/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

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

