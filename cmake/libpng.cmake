if (TARGET libpng)
    return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/tool.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libzlib.cmake)


unset(PNG_INCLUDE_DIR CACHE)
unset(PNG_LIBRARY CACHE)
unset(PNG_LIBRARIES CACHE)

set( LIB_PNG TryFindPackageFirst CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_PNG PROPERTY STRINGS None TryFindPackageFirst UsingFindPackage FromSource)

if(LIB_PNG STREQUAL TryFindPackageFirst)
    find_package(PNG QUIET)
    if (PNG_FOUND)
        message(STATUS "[LIB_PNG] using system lib.")
        set(LIB_PNG UsingFindPackage)
    else()
        message(STATUS "[LIB_PNG] compiling from source.")
        set(LIB_PNG FromSource)
    endif()
endif()

if(LIB_PNG STREQUAL FromSource)

    set(LIBS_REPOSITORY_URL "https://github.com/A-Ribeiro/public_libs/raw/main")
    tool_download_lib_package(${LIBS_REPOSITORY_URL} libpng)

    set(SKIP_INSTALL_ALL ON)
    option(PNG_SHARED "Build shared lib" OFF)
    option(PNG_STATIC "Build static lib" ON)
    option(PNG_TESTS  "Build libpng tests" OFF)

    if (APPLE)
    set(PNG_ARM_NEON off CACHE STRING "Enable ARM NEON optimizations:
        check: (default) use internal checking code;
        off: disable the optimizations;
        on: turn on unconditionally.")
    endif()

    tool_include_lib(libpng)

    #include_directories("${ARIBEIRO_GEN_INCLUDE_DIR}/libpng/")

    tool_get_dirs(png_DOWNLOADED_PATH png_BINARY_PATH libpng)
    set(png_INCLUDE_PATH "${png_DOWNLOADED_PATH}")

    set(PNG_INCLUDE_DIR "${png_INCLUDE_PATH}")
    set(PNG_LIBRARY png_static)
    set(PNG_LIBRARIES ${PNG_LIBRARY})

    #tool_make_global(CMAKE_PREFIX_PATH)
    tool_make_global(PNG_INCLUDE_DIR)
    tool_make_global(PNG_LIBRARY)
    tool_make_global(PNG_LIBRARIES)

    if (APPLE)
        #ifndef PNG_ARM_NEON_FILE
        #  ifdef __APPLE__
        #     define PNG_ARM_NEON_FILE "arm_neon.h"
        #  endif
        #endif
        target_compile_definitions(png_static PUBLIC PNG_ARM_NEON_FILE="arm_neon.h")
    endif()

elseif(LIB_PNG STREQUAL UsingFindPackage)

    find_package(PNG REQUIRED QUIET)

    # if (NOT TARGET libpng)

    #     #message("includeDIR: ${PNG_INCLUDE_DIR}")
    #     #message("Libs: ${PNG_LIBRARIES}")

    #     # add_library(libpng OBJECT ${PNG_LIBRARIES})
    #     # target_link_libraries(libpng ${PNG_LIBRARIES})
    #     # include_directories(${PNG_INCLUDE_DIR})
    #     # set_target_properties(libpng PROPERTIES LINKER_LANGUAGE CXX)

    #     # # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    #     # set_target_properties(libpng PROPERTIES FOLDER "LIBS")

    # endif()

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()


add_library(libpng INTERFACE)
set_target_properties(libpng PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libpng PROPERTIES FOLDER "LIBS")
target_include_directories(libpng INTERFACE ${PNG_INCLUDE_DIR})
target_link_libraries(libpng INTERFACE ${PNG_LIBRARIES} zlib)
