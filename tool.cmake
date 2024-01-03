
set( ARIBEIRO_LIBS_DIR "${CMAKE_HOME_DIRECTORY}/libs/3rdparty" CACHE STRING "The configured directory to download the libraries." )

macro(tool_unzip ZIPFILE OUTDIR)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${ZIPFILE}"
        WORKING_DIRECTORY "${OUTDIR}"
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        file(REMOVE "${ZIPFILE}")
        message( FATAL_ERROR "Cannot unzip ${ZIPFILE}")
    endif()
endmacro()

macro(tool_download_lib_package REPOSITORY_URL LIBNAME)
    if(NOT EXISTS "${ARIBEIRO_LIBS_DIR}/${LIBNAME}.zip")
        file(MAKE_DIRECTORY "${ARIBEIRO_LIBS_DIR}")
        message(STATUS "[${LIBNAME}] downloading...")
        file(DOWNLOAD "${REPOSITORY_URL}/${LIBNAME}.zip" "${ARIBEIRO_LIBS_DIR}/${LIBNAME}.zip" SHOW_PROGRESS STATUS result)
        list (GET result 0 error_code)
        if(NOT error_code EQUAL 0)
            file(REMOVE "${ARIBEIRO_LIBS_DIR}/${LIBNAME}.zip")
            message(FATAL_ERROR "Cannot download: ${REPOSITORY_URL}/${LIBNAME}.zip")
        endif()
        message(STATUS "[${LIBNAME}] unzip...")
        tool_unzip(
            "${ARIBEIRO_LIBS_DIR}/${LIBNAME}.zip"
            "${ARIBEIRO_LIBS_DIR}"
        )
        message(STATUS "[${LIBNAME}] done")
    endif()
endmacro()

macro(tool_include_lib)
    #get_property(aux GLOBAL PROPERTY BUILDSYSTEM_TARGETS)
    #get_directory_property(aux BUILDSYSTEM_TARGETS)

    if (  ${ARGC} EQUAL 1 )
        set(LIBNAME ${ARGV0})
        if (NOT TARGET ${LIBNAME})
            add_subdirectory("${ARIBEIRO_LIBS_DIR}/${LIBNAME}" "${ARIBEIRO_LIBS_DIR}/build/${LIBNAME}")
        endif()
    elseif (  ${ARGC} EQUAL 2 )
        set(_PATH ${ARGV0})
        set(LIBNAME ${ARGV1})
        if (NOT TARGET ${LIBNAME})
            add_subdirectory("${ARIBEIRO_LIBS_DIR}/${_PATH}/${LIBNAME}" "${ARIBEIRO_LIBS_DIR}/build/${LIBNAME}")
        endif()
    else()
        message(FATAL_ERROR "incorrect number of arguments.")
    endif()
endmacro()


# macro(copy_headers_to_include_directory LIBNAME)
#     file(COPY ${ARGN} DESTINATION "${ARIBEIRO_LIBS_DIR}/build/${LIBNAME}/include" )
# endmacro()

macro(tool_copy_to_dir DEST_DIR)
    file(COPY ${ARGN} DESTINATION "${DEST_DIR}" )
endmacro()

macro(tool_get_dirs VAR_LIB_PATH VAR_BINARY_PATH LIBNAME)
    set(${VAR_LIB_PATH} "${ARIBEIRO_LIBS_DIR}/${LIBNAME}")
    set(${VAR_BINARY_PATH} "${ARIBEIRO_LIBS_DIR}/build/${LIBNAME}")
endmacro()

#set(OpenGLStarter_Integration ON)

macro ( tool_make_global _var )
    set ( ${_var} ${${_var}} CACHE INTERNAL "hide this!" FORCE )
endmacro( )

macro ( tool_append_if_not_exists_and_make_global _var _value )
    foreach(entry IN ITEMS ${${_var}})
        if("${entry}" STREQUAL "${_value}")
            return()
        endif()
    endforeach()
    set(${_var} ${${_var}} "${_value}")
    tool_make_global(${_var})
endmacro( )

# #parent walk
# set(currDir ${CMAKE_CURRENT_SOURCE_DIR})
# while(NOT "${currDir}" STREQUAL "")
#     message(STATUS "    dir: ${currDir}" )

#     # set_property(currDir
#     #     DIRECTORY ${currDir}
#     #     PROPERTY PARENT_DIRECTORY)

#     get_property(currDir
#         DIRECTORY ${currDir}
#         PROPERTY PARENT_DIRECTORY)
# endwhile()