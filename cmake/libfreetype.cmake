if (TARGET freetype)
    return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/libzlib.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tool.cmake)

unset(FREETYPE_INCLUDE_DIR CACHE)
unset(FREETYPE_INCLUDE_DIRS CACHE)
unset(FREETYPE_LIBRARY CACHE)
unset(FREETYPE_LIBRARIES CACHE)

set( LIB_FREETYPE TryFindPackageFirst CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_FREETYPE PROPERTY STRINGS None TryFindPackageFirst UsingFindPackage FromSource)

if(LIB_FREETYPE STREQUAL TryFindPackageFirst)
    find_package(Freetype QUIET)
    if (FREETYPE_FOUND)
        message(STATUS "[LIB_FREETYPE] using system lib.")
        set(LIB_FREETYPE UsingFindPackage)
    else()
        message(STATUS "[LIB_FREETYPE] compiling from source.")
        set(LIB_FREETYPE FromSource)
    endif()
endif()

if(LIB_FREETYPE STREQUAL FromSource)

    set(LIBS_REPOSITORY_URL "https://github.com/A-Ribeiro/public_libs/raw/main")
    tool_download_lib_package(${LIBS_REPOSITORY_URL} freetype)

    set(SKIP_INSTALL_ALL ON)
    tool_include_lib(freetype)

    tool_get_dirs(freetype_DOWNLOADED_PATH freetype_BINARY_PATH freetype)
    set(freetype_INCLUDE_PATH 
        "${freetype_BINARY_PATH}/include"
        "${freetype_DOWNLOADED_PATH}/include")

    message(${freetype_INCLUDE_PATH})

    set(FREETYPE_INCLUDE_DIR ${freetype_INCLUDE_PATH})
    set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR})
    set(FREETYPE_LIBRARY libjpeg)
    set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})

    #tool_make_global(CMAKE_PREFIX_PATH)
    tool_make_global(FREETYPE_INCLUDE_DIR)
    tool_make_global(FREETYPE_INCLUDE_DIRS)
    tool_make_global(FREETYPE_LIBRARY)
    tool_make_global(FREETYPE_LIBRARIES)

    #include_directories("${ARIBEIRO_GEN_INCLUDE_DIR}/freetype/")

elseif(LIB_FREETYPE STREQUAL UsingFindPackage)

    if (NOT TARGET freetype)

        find_package(Freetype REQUIRED QUIET)

        add_library(freetype INTERFACE)
        set_target_properties(freetype PROPERTIES LINKER_LANGUAGE CXX)
        set_target_properties(freetype PROPERTIES FOLDER "LIBS")
        target_include_directories(freetype INTERFACE ${FREETYPE_INCLUDE_DIRS})
        target_link_libraries(freetype INTERFACE ${FREETYPE_LIBRARIES})

    endif()

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()
