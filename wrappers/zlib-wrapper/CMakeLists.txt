project (zlib-wrapper)

# if (NOT TARGET md5-wrapper)
#     add_subdirectory(../md5-wrapper "${CMAKE_BINARY_DIR}/lib/md5-wrapper")
# endif()

include(../../cmake/libzlib.cmake)

find_package(InteractiveToolkit REQUIRED QUIET)

if (NOT TARGET InteractiveToolkit-Extension AND NOT DEFINED INTERACTIVETOOLKIT_EXTENSION_INCLUDED)
    add_subdirectory(../../ "${CMAKE_BINARY_DIR}/lib/InteractiveToolkit-Extension")
endif()

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE PUBLIC_INL RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.inl)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

add_library( ${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${PUBLIC_INL} ${SRC} )

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
PRIVATE
    ${ZLIB_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} 
PUBLIC 
    ${INTERACTIVETOOLKIT_LIBRARIES}
    InteractiveToolkit-Extension
#    md5-wrapper
PRIVATE
    zlib
)
