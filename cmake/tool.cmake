
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
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
			get_filename_component(FILENAME_WITHOUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}" NAME)
            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
                        $<TARGET_FILE_DIR:${PROJECT_NAME}>/${FILENAME_WITHOUT_PATH} )
        elseif(EXISTS "${FILENAME}")
			get_filename_component(FILENAME_WITHOUT_PATH "${FILENAME}" NAME)
            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
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
            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}
                        $<TARGET_FILE_DIR:${PROJECT_NAME}>/${DIRECTORY} )
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