if (TARGET md5)
    return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/tool.cmake)

set( LIB_MD5 FromSource CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_MD5 PROPERTY STRINGS None FromSource)

if(LIB_MD5 STREQUAL FromSource)
    
    message(STATUS "[LIB_MD5] compiling from source.")

    set(LIBS_REPOSITORY_URL "https://github.com/A-Ribeiro/public_libs/raw/main")
    tool_download_lib_package(${LIBS_REPOSITORY_URL} md5)
    tool_include_lib(md5)

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()
