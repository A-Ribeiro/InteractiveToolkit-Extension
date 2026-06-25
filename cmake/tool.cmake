
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


macro (tool_remove_from_list item_list regular_expression)
    foreach (item ${${item_list}})
        if ("${item}" MATCHES ${regular_expression})
            list (REMOVE_ITEM ${item_list} ${item})
        endif()
    endforeach(item)
    # set(${item_list} ${${item_list}} PARENT_SCOPE)
endmacro()

macro(tool_define_source_group )
    foreach(entry IN ITEMS ${ARGN})
        get_filename_component(dirname "${entry}" DIRECTORY )
        if (dirname)
            string(REPLACE "/" "\\" dirname_replaced ${dirname})
            source_group(${dirname_replaced} FILES ${entry})
        else()
            source_group("" FILES ${entry})
        endif()
    endforeach()
endmacro()

macro(tool_define_source_group_base_path base_path )
    foreach(entry IN ITEMS ${ARGN})
    #message(STATUS "entry:${entry}")
        get_filename_component(dirname "${entry}" DIRECTORY )
        if (dirname)
            if (NOT dirname MATCHES "/$")
                set(dirname "${dirname}/")
            endif()
            
            string(FIND "${dirname}" "${base_path}" found)
            
            #message(STATUS "found:${found}")

            if (found VERSION_EQUAL 0)
                string(LENGTH "${base_path}" base_path_length)
                string(LENGTH "${dirname}" dirname_length)
                math(EXPR new_length "${dirname_length} - ${base_path_length}")
                #message(STATUS "base_path:${base_path}")
                #message(STATUS "dirname:${dirname}")
                string(SUBSTRING "${dirname}" 
                        "${base_path_length}" 
                        "${new_length}" dirname)
            endif()

            string(FIND "${dirname}" "/" found)
            string(LENGTH "${dirname}" dirname_length)

            if (found VERSION_EQUAL 0 AND dirname_length VERSION_GREATER 0)
                # message("dirname: ${dirname}")
                string(LENGTH "${dirname}" dirname_length)
                math(EXPR new_length "${dirname_length} - 1")
                string(SUBSTRING "${dirname}" 
                        "1" 
                        "${new_length}" dirname)
            endif()

            string(LENGTH "${dirname}" dirname_length)
            if (dirname_length VERSION_GREATER 0)
                string(REPLACE "/" "\\" dirname_replaced ${dirname})
                source_group(${dirname_replaced} FILES ${entry})
            else()
                source_group("" FILES ${entry})
            endif()

        else()
            source_group("" FILES ${entry})
        endif()
    endforeach()
endmacro()

macro(tool_copy_file_after_build PROJECT_NAME)
    foreach(FILENAME IN ITEMS ${ARGN})
        if (NOT WIN32)
            # bugfix for sh copy files with parentheses in the name
            string( REPLACE "(" "\\(" FILENAME "${FILENAME}" )
            string( REPLACE ")" "\\)" FILENAME "${FILENAME}" )
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
			get_filename_component(FILENAME_WITHOUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}" NAME)
            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
                        $<TARGET_FILE_DIR:${PROJECT_NAME}>/${FILENAME_WITHOUT_PATH} )
        elseif(EXISTS "${FILENAME}")
			get_filename_component(FILENAME_WITHOUT_PATH "${FILENAME}" NAME)
            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${FILENAME}
                        $<TARGET_FILE_DIR:${PROJECT_NAME}>/${FILENAME_WITHOUT_PATH} )
        else()
            message(FATAL_ERROR "File Does Not Exists: ${FILENAME}")
        endif()
    endforeach()
endmacro()

macro(tool_copy_directory_after_build PROJECT_NAME)
    foreach(DIRECTORY IN ITEMS ${ARGN})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}")
            file(GLOB_RECURSE COPY_FILES 
                RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}"
                "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/*")
            
            foreach(COPY_FILE ${COPY_FILES})
                if (NOT WIN32)
                    # bugfix for sh copy files with parentheses in the name
                    string( REPLACE "(" "\\(" COPY_FILE "${COPY_FILE}" )
                    string( REPLACE ")" "\\)" COPY_FILE "${COPY_FILE}" )
                endif()
                add_custom_command(
                    TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}/${COPY_FILE}"
                            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DIRECTORY}/${COPY_FILE}" )
            endforeach()
        else()
            message(FATAL_ERROR "Directory Does Not Exists: ${DIRECTORY}")
        endif()
    endforeach()
endmacro()



macro(tool_download_git_package REPOSITORY_URL LIBNAME)
    if(NOT EXISTS "${ARIBEIRO_LIBS_DIR}/${LIBNAME}/")
        file(MAKE_DIRECTORY "${ARIBEIRO_LIBS_DIR}")
        message(STATUS "[${LIBNAME}] cloning...")

        find_package(Git REQUIRED)

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" clone --recurse-submodules "${REPOSITORY_URL}" "${ARIBEIRO_LIBS_DIR}/${LIBNAME}/"
            OUTPUT_VARIABLE result
            #COMMAND_ERROR_IS_FATAL ANY
        )

        if(NOT EXISTS "${ARIBEIRO_LIBS_DIR}/${LIBNAME}/")
            message(FATAL_ERROR "Error to clone repository: ${REPOSITORY_URL}")
        endif()

        message(STATUS "[${LIBNAME}] done")

    endif()
endmacro()

macro(tool_download_git_package_branch REPOSITORY_URL BRANCH LIBNAME)
    if(NOT EXISTS "${ARIBEIRO_LIBS_DIR}/${LIBNAME}/")
        file(MAKE_DIRECTORY "${ARIBEIRO_LIBS_DIR}")
        message(STATUS "[${LIBNAME}] cloning...")

        find_package(Git REQUIRED)

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" clone --recurse-submodules --branch ${BRANCH} "${REPOSITORY_URL}" "${ARIBEIRO_LIBS_DIR}/${LIBNAME}/"
            OUTPUT_VARIABLE result
            #COMMAND_ERROR_IS_FATAL ANY
        )

        if(NOT EXISTS "${ARIBEIRO_LIBS_DIR}/${LIBNAME}/")
            message(FATAL_ERROR "Error to clone repository: ${REPOSITORY_URL}")
        endif()

        message(STATUS "[${LIBNAME}] done")

    endif()
endmacro()

# macro(tool_remove_compile_options)
#     get_directory_property(compile_opts COMPILE_OPTIONS)

#     foreach(entry IN ITEMS ${ARGN})
#         list(REMOVE_ITEM compile_opts ${entry})
#     endforeach()

#     #set_property(DIRECTORY ${CMAKE_CURRENT_SRC_DIR} APPEND PROPERTY COMPILE_OPTIONS ${compile_opts})
#     set_property(DIRECTORY ${CMAKE_CURRENT_SRC_DIR} PROPERTY COMPILE_OPTIONS ${compile_opts})
# endmacro()

macro(tool_replace_in_file FILE SEARCH REPLACE)

    set(StringReplaceSource "${SEARCH}")
    set(StringReplaceTarget "${REPLACE}")

    file(READ "${FILE}" AUX)
    string(FIND "${AUX}" "${StringReplaceTarget}" matchres)
    if(${matchres} EQUAL -1)
        string(REPLACE "${StringReplaceSource}" "${StringReplaceTarget}" output "${AUX}")
        file(WRITE "${FILE}" "${output}")
    endif ()

endmacro()


macro(tool_to_unique_list output)
    set(${output})
    foreach(var IN ITEMS ${ARGN})
        if (NOT "${var}" IN_LIST ${output})
            list(APPEND ${output} ${var})
        endif()
    endforeach()
endmacro()

macro(tool_configure_build_flags projectname inputfile outputfile)

    #output
    set(configure_COMPILE_DEFINITIONS "") # generate buildFlags.h
    set(configure_COMPILE_OPTIONS "") # generate opts.cmake
    set(configure_LINK_LIBRARIES "") # generate opts.cmake
    set(configure_LINK_OPTIONS "") # generate opts.cmake
    set(configure_INCLUDE_DIRECTORIES "") # generate opts.cmake
    set(configure_LIB_CMAKE_FLAGS "") # generate opts.cmake

    set(target_COMPILE_DEFINITIONS)
    set(target_COMPILE_OPTIONS)
    set(target_LINK_LIBRARIES)
    set(target_LINK_OPTIONS)
    set(target_INCLUDE_DIRECTORIES)

    if (TARGET ${projectname})
        get_target_property(target_COMPILE_DEFINITIONS ${projectname} COMPILE_DEFINITIONS)
        if ("${target_COMPILE_DEFINITIONS}" STREQUAL "target_COMPILE_DEFINITIONS-NOTFOUND")
            set(target_COMPILE_DEFINITIONS)
        endif()

        get_target_property(target_COMPILE_OPTIONS ${projectname} COMPILE_OPTIONS)
        if ("${target_COMPILE_OPTIONS}" STREQUAL "target_COMPILE_OPTIONS-NOTFOUND")
            set(target_COMPILE_OPTIONS)
        endif()

        get_target_property(target_LINK_LIBRARIES ${projectname} INTERFACE_LINK_LIBRARIES)
        if ("${target_LINK_LIBRARIES}" STREQUAL "target_LINK_LIBRARIES-NOTFOUND")
            set(target_LINK_LIBRARIES)
        endif()

        get_target_property(target_LINK_OPTIONS ${projectname} INTERFACE_LINK_OPTIONS)
        if ("${target_LINK_OPTIONS}" STREQUAL "target_LINK_OPTIONS-NOTFOUND")
            set(target_LINK_OPTIONS)
        endif()

        get_target_property(target_INCLUDE_DIRECTORIES ${projectname} INTERFACE_INCLUDE_DIRECTORIES)
        if ("${target_INCLUDE_DIRECTORIES}" STREQUAL "target_INCLUDE_DIRECTORIES-NOTFOUND")
            set(target_INCLUDE_DIRECTORIES)
        endif()

    endif()

    # get build flags
    #get_directory_property(aux COMPILE_DEFINITIONS)
    #
    # configure_COMPILE_DEFINITIONS
    #
    tool_to_unique_list(aux ${target_COMPILE_DEFINITIONS})
    foreach(var ${aux})
        if(NOT "${var}" STREQUAL "NDEBUG" AND "${var}" MATCHES "^ITKEXT_")
            set(configure_COMPILE_DEFINITIONS "${configure_COMPILE_DEFINITIONS}#ifndef ${var}\n#    define ${var}\n#endif\n")
        endif()
    endforeach()

    #unset(INTERACTIVETOOLKIT_INCLUDE_DIRS)

    #
    # COMPILE_OPTIONS
    #
    tool_to_unique_list(aux ${target_COMPILE_OPTIONS})
    foreach(var ${aux})
        set(configure_COMPILE_OPTIONS "${configure_COMPILE_OPTIONS}    list(APPEND ITKEXT_COMPILE_OPTIONS \"${var}\")\n")
    endforeach()

    #
    # LINK_LIBRARIES
    #
    tool_to_unique_list(aux ${target_LINK_LIBRARIES})
    foreach(var ${aux})
        set(configure_LINK_LIBRARIES "${configure_LINK_LIBRARIES}    list(APPEND ITKEXT_LIBRARIES \"${var}\")\n")
    endforeach()

    #
    # LINK_OPTIONS
    #
    tool_to_unique_list(aux ${target_LINK_OPTIONS})
    foreach(var ${aux})
        set(configure_LINK_OPTIONS "${configure_LINK_OPTIONS}    list(APPEND ITKEXT_LINK_OPTIONS \"${var}\")\n")
    endforeach()

    #
    # INCLUDE_DIRECTORIES
    #
    tool_to_unique_list(aux ${target_INCLUDE_DIRECTORIES})
    foreach(var ${aux})
        if (var STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/include")
            continue()
        endif()
        set(configure_INCLUDE_DIRECTORIES "${configure_INCLUDE_DIRECTORIES}    list(APPEND ITKEXT_INCLUDE_DIRECTORIES \"${var}\")\n")
    endforeach()

    #
    # CMAKE LIB_FLAGS
    #

    set(configure_LIB_CMAKE_FLAGS "${configure_LIB_CMAKE_FLAGS}    set(ITKEXT_BCRYPT ${ITKEXT_BCRYPT})\n")
    set(configure_LIB_CMAKE_FLAGS "${configure_LIB_CMAKE_FLAGS}    set(ITKEXT_FONT ${ITKEXT_FONT})\n")
    set(configure_LIB_CMAKE_FLAGS "${configure_LIB_CMAKE_FLAGS}    set(ITKEXT_IMAGE ${ITKEXT_IMAGE})\n")
    set(configure_LIB_CMAKE_FLAGS "${configure_LIB_CMAKE_FLAGS}    set(ITKEXT_IMAGE_ATLAS ${ITKEXT_IMAGE_ATLAS})\n")
    set(configure_LIB_CMAKE_FLAGS "${configure_LIB_CMAKE_FLAGS}    set(ITKEXT_MODEL ${ITKEXT_MODEL})\n")
    set(configure_LIB_CMAKE_FLAGS "${configure_LIB_CMAKE_FLAGS}    set(ITKEXT_NETWORK ${ITKEXT_NETWORK})\n")
    set(configure_LIB_CMAKE_FLAGS "${configure_LIB_CMAKE_FLAGS}    set(ITKEXT_NETWORK_TLS ${ITKEXT_NETWORK_TLS})\n")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${outputfile}"
        @ONLY
    )
endmacro()