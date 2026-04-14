cmake_minimum_required (VERSION 3.10)

project(InteractiveToolkit-Extension)

# "For this is how God loved the world:
# he gave his only Son, so that everyone
# who believes in him may not perish
# but may have eternal life."
#
# John 3:16

set(INTERACTIVETOOLKIT_EXTENSION_INCLUDED TRUE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

ENABLE_LANGUAGE(CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(cmake/tool.cmake)

set(ITKEXT_BCRYPT OFF CACHE BOOL "Set this if you want building bcrypt support" )
set(ITKEXT_FONT OFF CACHE BOOL "Set this if you want building font+atlas support" )
set(ITKEXT_IMAGE OFF CACHE BOOL "Set this if you want building image(JPEG+PNG) support" )
set(ITKEXT_IMAGE_ATLAS OFF CACHE BOOL "Set this if you want building image/texture atlas support" )
set(ITKEXT_MODEL OFF CACHE BOOL "Set this if you want building model(BASOF) support" )
set(ITKEXT_NETWORK OFF CACHE BOOL "Set this if you want building network support" )
set(ITKEXT_NETWORK_TLS OFF CACHE BOOL "Set this if you want building network/tls (mbedtls) support" )

if (ITKEXT_FONT)
    set(ITKEXT_IMAGE_ATLAS ON)
endif()

if (NOT ITKEXT_NETWORK)
    set(ITKEXT_NETWORK_TLS OFF)
endif()

if (ITKEXT_IMAGE)
    include(cmake/libpng.cmake)
    include(cmake/libjpeg.cmake)
endif()

if (ITKEXT_NETWORK_TLS)
    include(cmake/libmbedtls.cmake)
endif()

find_package(InteractiveToolkit REQUIRED QUIET)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE PUBLIC_INL RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.inl)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

set(EXCLUDE_WRAPPERS_DIR_REG_EXP "wrappers/.*")
tool_remove_from_list(PUBLIC_HEADERS ${EXCLUDE_WRAPPERS_DIR_REG_EXP})
tool_remove_from_list(PUBLIC_INL ${EXCLUDE_WRAPPERS_DIR_REG_EXP})
tool_remove_from_list(SRC ${EXCLUDE_WRAPPERS_DIR_REG_EXP})


if (NOT ITKEXT_IMAGE_ATLAS)
    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/atlas/.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/atlas/.*")
    tool_remove_from_list(SRC "src/atlas/.*")
endif()

if (NOT ITKEXT_FONT)
    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/font/.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/font/.*")
    tool_remove_from_list(SRC "src/font/.*")
endif()

if(NOT ITKEXT_BCRYPT)
    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/hashing/Bcrypt.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/hashing/Bcrypt.*")
    tool_remove_from_list(SRC "src/hashing/Bcrypt.*")
endif()

if(NOT ITKEXT_IMAGE)
    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/image/.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/image/.*")
    tool_remove_from_list(SRC "src/image/.*")
endif()

if(NOT ITKEXT_MODEL)
    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/model/.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/model/.*")
    tool_remove_from_list(SRC "src/model/.*")
endif()

if(NOT ITKEXT_NETWORK)
    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/network/.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/network/.*")
    tool_remove_from_list(SRC "src/network/.*")
endif()

if(NOT ITKEXT_NETWORK_TLS)
    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/network/tls/.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/network/tls/.*")
    tool_remove_from_list(SRC "src/network/tls/.*")

    tool_remove_from_list(PUBLIC_HEADERS "InteractiveToolkit-Extension/network/SocketTCP_SSL.*")
    tool_remove_from_list(PUBLIC_INL "InteractiveToolkit-Extension/network/SocketTCP_SSL.*")
    tool_remove_from_list(SRC "src/network/SocketTCP_SSL.*")

endif()


define_source_group(${PUBLIC_HEADERS} ${PUBLIC_INL} ${SRC})

tool_append_if_not_exists_and_make_global(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (NOT TARGET zlib-wrapper)
    add_subdirectory(wrappers/zlib-wrapper "${CMAKE_BINARY_DIR}/lib/zlib-wrapper")
endif()
# if (NOT TARGET md5-wrapper)
#     add_subdirectory(wrappers/md5-wrapper "${CMAKE_BINARY_DIR}/lib/md5-wrapper")
# endif()

add_library(${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${PUBLIC_INL} ${SRC})


if (ITKEXT_IMAGE_ATLAS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ITKEXT_IMAGE_ATLAS)
endif()

if (ITKEXT_FONT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ITKEXT_FONT)
endif()

if(ITKEXT_BCRYPT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ITKEXT_BCRYPT)
endif()

if(ITKEXT_IMAGE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ITKEXT_IMAGE)
endif()

if(ITKEXT_MODEL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ITKEXT_MODEL)
endif()

if(ITKEXT_NETWORK)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ITKEXT_NETWORK)
endif()

if(ITKEXT_NETWORK_TLS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ITKEXT_NETWORK_TLS)
endif()

tool_configure_build_flags( ${PROJECT_NAME} "include/InteractiveToolkit-Extension/buildFlags.h.in" "include/InteractiveToolkit-Extension/buildFlags.h")
tool_configure_build_flags( ${PROJECT_NAME} "include/InteractiveToolkit-Extension/opts.cmake.in" "include/InteractiveToolkit-Extension/opts.cmake")

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
#    md5-wrapper
)

if (ITKEXT_IMAGE)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC ${INTERACTIVETOOLKIT_LIBRARIES}
        PRIVATE libpng libjpeg
    )
endif()


if (ITKEXT_NETWORK_TLS)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC ${INTERACTIVETOOLKIT_LIBRARIES}
        PRIVATE mbedtls-all
    )
endif()
