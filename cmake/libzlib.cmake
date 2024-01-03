if (TARGET zlib)
    return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/tool.cmake)

unset(ZLIB_INCLUDE_DIR CACHE)
unset(ZLIB_LIBRARY CACHE)
unset(ZLIB_LIBRARIES CACHE)

set( LIB_ZLIB TryFindPackageFirst CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_ZLIB PROPERTY STRINGS None TryFindPackageFirst UsingFindPackage FromSource)

if(LIB_ZLIB STREQUAL TryFindPackageFirst)
    
    find_package(ZLIB QUIET)
    if (ZLIB_FOUND)
        message(STATUS "[LIB_ZLIB] using system lib.")
        set(LIB_ZLIB UsingFindPackage)
    else()
        message(STATUS "[LIB_ZLIB] compiling from source.")
        set(LIB_ZLIB FromSource)
    endif()
endif()

if(LIB_ZLIB STREQUAL FromSource)

    set(LIBS_REPOSITORY_URL "https://github.com/A-Ribeiro/public_libs/raw/main")
    tool_download_lib_package(${LIBS_REPOSITORY_URL} zlib)

    set(SKIP_INSTALL_ALL ON)
    tool_include_lib(zlib)
    #unset(SKIP_INSTALL_ALL)

    #include_directories("${ARIBEIRO_GEN_INCLUDE_DIR}/zlib/")

    # allow prefix path to search zlib
    #PARENT_SCOPE
    
    #tool_copy_to_include_dir(zlib ${ZLIB_PUBLIC_HDRS})

    tool_get_dirs(zlib_DOWNLOADED_PATH zlib_BINARY_PATH zlib)
    
    set(zlib_INCLUDE_PATH "${zlib_BINARY_PATH}/include")

    # tool_copy_to_dir(
    #     #output dir
    #     ${zlib_INCLUDE_PATH} 
    #     #files
    #     ${zlib_BINARY_PATH}/zconf.h
    #     ${zlib_DOWNLOADED_PATH}/zlib.h
    # )

    #set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${zlib_INCLUDE_PATH}")
    set(ZLIB_INCLUDE_DIR "${zlib_INCLUDE_PATH}")
    #set(ZLIB_LIBRARY $<TARGET_PROPERTY:zlib,ARCHIVE_OUTPUT_NAME>)
    set(ZLIB_LIBRARY zlib)
    set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})

    #tool_make_global(CMAKE_PREFIX_PATH)
    tool_make_global(ZLIB_INCLUDE_DIR)
    tool_make_global(ZLIB_LIBRARY)
    tool_make_global(ZLIB_LIBRARIES)

    # target_include_directories(zlib PUBLIC ${ZLIB_INCLUDE_DIR})

elseif(LIB_ZLIB STREQUAL UsingFindPackage)

    # if (NOT TARGET zlib)

    #     find_package(ZLIB REQUIRED QUIET)

    #     add_library(zlib OBJECT ${ZLIB_LIBRARIES})
    #     target_link_libraries(zlib ${ZLIB_LIBRARIES})
    #     include_directories(${ZLIB_INCLUDE_DIR})
    #     set_target_properties(zlib PROPERTIES LINKER_LANGUAGE CXX)

    #     # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    #     set_target_properties(zlib PROPERTIES FOLDER "LIBS")

    # endif()

    find_package(ZLIB REQUIRED QUIET)
    
    add_library(zlib INTERFACE)
    set_target_properties(zlib PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(zlib PROPERTIES FOLDER "LIBS")
    target_include_directories(zlib INTERFACE ${ZLIB_INCLUDE_DIR})
    target_link_libraries(zlib INTERFACE ${ZLIB_LIBRARIES})

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()
