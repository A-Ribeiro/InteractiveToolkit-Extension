project (ft2-wrapper)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

define_source_group(${PUBLIC_HEADERS} ${SRC})

add_library( ${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${SRC} )

# set the target's folder (for IDEs that support it, e.g. Visual Studio)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "LIBS")

#link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC freetype aRibeiroCore aRibeiroData)

copy_headers_to_include_directory(${PROJECT_NAME} ${PUBLIC_HEADERS})
configure_include_file( ${PROJECT_NAME} "${PROJECT_NAME}.h.in" "${PROJECT_NAME}.h" ${PUBLIC_HEADERS} )
