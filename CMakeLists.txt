cmake_minimum_required (VERSION 3.10)

project(InteractiveToolkit-Extension)

# "For this is how God loved the world:
# he gave his only Son, so that everyone
# who believes in him may not perish
# but may have eternal life."
#
# John 3:16

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

ENABLE_LANGUAGE(CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(cmake/tool.cmake)
include(cmake/libpng.cmake)
include(cmake/libjpeg.cmake)
include(cmake/libmbedtls.cmake)

find_package(InteractiveToolkit REQUIRED QUIET)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE PUBLIC_INL RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.inl)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

set(EXCLUDE_WRAPPERS_DIR_REG_EXP "wrappers/.*")
tool_remove_from_list(PUBLIC_HEADERS ${EXCLUDE_WRAPPERS_DIR_REG_EXP})
tool_remove_from_list(PUBLIC_INL ${EXCLUDE_WRAPPERS_DIR_REG_EXP})
tool_remove_from_list(SRC ${EXCLUDE_WRAPPERS_DIR_REG_EXP})

tool_append_if_not_exists_and_make_global(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (NOT TARGET zlib-wrapper)
    add_subdirectory(wrappers/zlib-wrapper "${CMAKE_BINARY_DIR}/lib/zlib-wrapper")
endif()
if (NOT TARGET md5-wrapper)
    add_subdirectory(wrappers/md5-wrapper "${CMAKE_BINARY_DIR}/lib/md5-wrapper")
endif()

add_library(${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${PUBLIC_INL} ${SRC})

#set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS 1)

# set the target's folder (for IDEs that support it, e.g. Visual Studio)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "LIBS")

target_compile_options(${PROJECT_NAME} 
PUBLIC
    ${INTERACTIVETOOLKIT_COMPILE_OPTIONS}
)

target_link_options(${PROJECT_NAME} 
PUBLIC
    ${INTERACTIVETOOLKIT_LINK_OPTIONS}
)

target_include_directories(${PROJECT_NAME} 
PUBLIC
    ${INTERACTIVETOOLKIT_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} 
PUBLIC 
    ${INTERACTIVETOOLKIT_LIBRARIES}
    zlib-wrapper
    md5-wrapper
    mbedtls-all
PRIVATE
    libpng
    libjpeg
)
