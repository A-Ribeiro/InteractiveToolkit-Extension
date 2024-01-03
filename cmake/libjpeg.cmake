if (TARGET libjpeg)
    return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/tool.cmake)

unset(JPEG_INCLUDE_DIR CACHE)
unset(JPEG_LIBRARY CACHE)
unset(JPEG_LIBRARIES CACHE)

set( LIB_JPEG TryFindPackageFirst CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_JPEG PROPERTY STRINGS None TryFindPackageFirst UsingFindPackage FromSource)

if(LIB_JPEG STREQUAL TryFindPackageFirst)
    find_package(JPEG QUIET)
    if (JPEG_FOUND)
        message(STATUS "[LIB_JPEG] using system lib.")
        set(LIB_JPEG UsingFindPackage)
    else()
        message(STATUS "[LIB_JPEG] compiling from source.")
        set(LIB_JPEG FromSource)
    endif()
endif()

if(LIB_JPEG STREQUAL FromSource)

    set(LIBS_REPOSITORY_URL "https://github.com/A-Ribeiro/public_libs/raw/main")
    tool_download_lib_package(${LIBS_REPOSITORY_URL} libjpeg)

    tool_include_lib(libjpeg)

    tool_get_dirs(jpeg_DOWNLOADED_PATH jpeg_BINARY_PATH libjpeg)
    set(jpeg_INCLUDE_PATH "${jpeg_BINARY_PATH}/include")

    set(JPEG_INCLUDE_DIR "${jpeg_INCLUDE_PATH}")
    set(JPEG_LIBRARY libjpeg)
    set(JPEG_LIBRARIES ${JPEG_LIBRARY})

    #tool_make_global(CMAKE_PREFIX_PATH)
    tool_make_global(JPEG_INCLUDE_DIR)
    tool_make_global(JPEG_LIBRARY)
    tool_make_global(JPEG_LIBRARIES)

    #include_directories("${ARIBEIRO_GEN_INCLUDE_DIR}/libjpeg/")

elseif(LIB_JPEG STREQUAL UsingFindPackage)

    find_package(JPEG REQUIRED QUIET)
    
    add_library(libjpeg INTERFACE)
    set_target_properties(libjpeg PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(libjpeg PROPERTIES FOLDER "LIBS")
    target_include_directories(libjpeg INTERFACE ${JPEG_INCLUDE_DIR})
    target_link_libraries(libjpeg INTERFACE ${JPEG_LIBRARIES})

    # if (NOT TARGET libjpeg)

    #     find_package(JPEG REQUIRED QUIET)

    #     #message("includeDIR: ${JPEG_INCLUDE_DIR}")
    #     #message("Libs: ${JPEG_LIBRARIES}")

    #     add_library(libjpeg OBJECT ${JPEG_LIBRARIES})
    #     target_link_libraries(libjpeg ${JPEG_LIBRARIES})
    #     include_directories(${JPEG_INCLUDE_DIR})
    #     set_target_properties(libjpeg PROPERTIES LINKER_LANGUAGE CXX)

    #     # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    #     set_target_properties(libjpeg PROPERTIES FOLDER "LIBS")

    # endif()

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()
