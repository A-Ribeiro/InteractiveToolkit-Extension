if (TARGET sfml-main OR TARGET sfml-system OR TARGET sfml-window OR TARGET sfml-graphics OR TARGET sfml-audio OR TARGET sfml-network)
    return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/tool.cmake)

set( LIB_SFML FromSource CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_SFML PROPERTY STRINGS None TryFindPackageFirst UsingFindPackage FromSource)

if(LIB_SFML STREQUAL TryFindPackageFirst)
    find_package(SFML 2 COMPONENTS system window graphics audio network QUIET)

    # if (SFML_INCLUDE_DIR AND SFML_LIBRARIES)
	#   set(SFML_FOUND TRUE)
    # else()
    #     unset(SFML_INCLUDE_DIR CACHE)
    #     unset(SFML_LIBRARIES CACHE)
	# endif()

    if (SFML_FOUND)
        message(STATUS "[LIB_SFML] using system lib.")
        set(LIB_SFML UsingFindPackage)
    else()
        message(STATUS "[LIB_SFML] compiling from source.")
        set(LIB_SFML FromSource)
    endif()
endif()

if(LIB_SFML STREQUAL FromSource)

    message(STATUS "")
    message(STATUS "BUILDING SFML FROM SOURCE")
    message(STATUS "")
    message(STATUS "You need to run the following code:")
    message(STATUS "")
    message(STATUS "    sudo apt install libxrandr-dev libxcursor-dev")
    message(STATUS "    sudo apt install libudev-dev libopenal-dev ")
    message(STATUS "    sudo apt install libogg-dev libvorbis-dev libflac-dev")
    message(STATUS "    sudo apt install libfreetype6-dev")
    message(STATUS "    sudo apt install libxi-dev")
    message(STATUS "    sudo apt install libharfbuzz-dev")
    message(STATUS "")
    message(STATUS "In case opengl is not found:")
    message(STATUS "")
    message(STATUS "    sudo apt install libgl1-mesa-dev")
    message(STATUS "")
    
    tool_download_git_package("https://github.com/SFML/SFML.git" sfml)

    set(BUILD_SHARED_LIBS OFF)

    set(SFML_BUILD_WINDOW TRUE CACHE BOOL "TRUE to build SFML's Window module. This setting is ignored, if the graphics module is built.")
    set(SFML_BUILD_GRAPHICS TRUE CACHE BOOL "TRUE to build SFML's Graphics module.")
    set(SFML_BUILD_AUDIO TRUE CACHE BOOL "TRUE to build SFML's Audio module.")
    set(SFML_BUILD_NETWORK TRUE CACHE BOOL "TRUE to build SFML's Network module.")

    # if (NOT MSVC)
    #     # add compile flag -w : do not treat warnings as errors
    #     add_compile_options(-w)
    # endif()

    if(WIN32)
        set( ARCH_TARGET x64 )
        if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
            set( ARCH_TARGET x86 )
        endif()
        if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
            set( ARCH_TARGET arm64 )
            if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
                set( ARCH_TARGET arm32 )
            endif()
        endif()

        tool_get_dirs(sfml_DOWNLOADED_PATH sfml_BINARY_PATH sfml)

        # set(STR_TO_ADD "tool_copy_file_after_build( sfml-audio \"${sfml_DOWNLOADED_PATH}/extlibs/bin/${ARCH_TARGET}/openal32.dll\" )")

        # SFML now uses miniaudio instead OpenAL
        #
        # # Insert the copy of openal32.dll to bin folder at build time
        # file(READ "${sfml_DOWNLOADED_PATH}/src/SFML/Audio/CMakeLists.txt" AUX)
        # string(FIND "${AUX}" "${STR_TO_ADD}" matchres)
        # if(${matchres} EQUAL -1)
        #     file(APPEND "${sfml_DOWNLOADED_PATH}/src/SFML/Audio/CMakeLists.txt" "\n${STR_TO_ADD}\n")
        # endif ()

        # replace wrong identifier on VS
        # file(READ "${sfml_DOWNLOADED_PATH}/src/SFML/Graphics/Font.cpp" AUX)
        # string(FIND "${AUX}" "//Font::Font(Font&&) noexcept = default;" matchres)
        # if(${matchres} EQUAL -1)
        #     string(REPLACE "Font::Font(Font&&) noexcept = default;" "//Font::Font(Font&&) noexcept = default;" output "${AUX}")
        #     file(WRITE "${sfml_DOWNLOADED_PATH}/src/SFML/Graphics/Font.cpp" "${output}")
        # endif ()



        # handle window resizing...
        file(READ "${sfml_DOWNLOADED_PATH}/src/SFML/Window/Win32/WindowImplWin32.cpp" AUX)
        string(FIND "${AUX}" "void (*AppKit_Window_win32_draw_on_resize_or_move)(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) = nullptr;" matchres)
        if(${matchres} EQUAL -1)
            string(REPLACE "#include <SFML/Window/Win32/WindowImplWin32.hpp>" "#include <SFML/Window/Win32/WindowImplWin32.hpp>\nvoid (*AppKit_Window_win32_draw_on_resize_or_move)(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) = nullptr;\n" output "${AUX}")
            file(WRITE "${sfml_DOWNLOADED_PATH}/src/SFML/Window/Win32/WindowImplWin32.cpp" "${output}")
        endif ()

        file(READ "${sfml_DOWNLOADED_PATH}/src/SFML/Window/Win32/WindowImplWin32.cpp" AUX)
        string(FIND "${AUX}" "if (AppKit_Window_win32_draw_on_resize_or_move)\n        AppKit_Window_win32_draw_on_resize_or_move(m_handle, message, wParam, lParam);" matchres)
        if(${matchres} EQUAL -1)
            string(REPLACE "switch (message)" "if (AppKit_Window_win32_draw_on_resize_or_move)\n        AppKit_Window_win32_draw_on_resize_or_move(m_handle, message, wParam, lParam);\n    switch (message)" output "${AUX}")
            file(WRITE "${sfml_DOWNLOADED_PATH}/src/SFML/Window/Win32/WindowImplWin32.cpp" "${output}")
        endif ()

    endif()

    #add_definitions(-DSFML_STATIC)

    tool_include_lib(sfml)
    
    # if (NOT MSVC)
    #     # remove compile flag -w
    #     tool_remove_compile_options(-w)
    # endif()

    if (TARGET sfml-main)
        target_compile_definitions(sfml-main PUBLIC -DSFML_STATIC)
        set_target_properties(sfml-main PROPERTIES FOLDER "LIBS/SFML")
        set_target_properties(sfml-main PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET sfml-system)
        target_compile_definitions(sfml-system PUBLIC -DSFML_STATIC)
        set_target_properties(sfml-system PROPERTIES FOLDER "LIBS/SFML")
        set_target_properties(sfml-system PROPERTIES CXX_STANDARD 11)
    endif()
    if (TARGET sfml-window)
        target_compile_definitions(sfml-window PUBLIC -DSFML_STATIC)
        set_target_properties(sfml-window PROPERTIES FOLDER "LIBS/SFML")
        set_target_properties(sfml-window PROPERTIES CXX_STANDARD 11)
    endif()
    if (TARGET sfml-graphics)
        target_compile_definitions(sfml-graphics PUBLIC -DSFML_STATIC)
        set_target_properties(sfml-graphics PROPERTIES FOLDER "LIBS/SFML")
        set_target_properties(sfml-graphics PROPERTIES CXX_STANDARD 11)
        if(WIN32)
            #target_compile_options(sfml-graphics PUBLIC /Zc:noexceptTypes- )
            #set_target_properties(sfml-graphics PROPERTIES CXX_STANDARD 11)
            #set_target_properties(sfml-graphics PROPERTIES CXX_STANDARD_REQUIRED ON)
            #set_target_properties(sfml-graphics PROPERTIES CXX_EXTENSIONS OFF)
        endif()
    endif()
    if (TARGET sfml-audio)
        target_compile_definitions(sfml-audio PUBLIC -DSFML_STATIC)
        set_target_properties(sfml-audio PROPERTIES FOLDER "LIBS/SFML")
        set_target_properties(sfml-audio PROPERTIES CXX_STANDARD 11)
    endif()
    if (TARGET sfml-network)
        target_compile_definitions(sfml-network PUBLIC -DSFML_STATIC)
        set_target_properties(sfml-network PROPERTIES FOLDER "LIBS/SFML")
        set_target_properties(sfml-network PROPERTIES CXX_STANDARD 11)
    endif()
    
    include_directories("${ARIBEIRO_LIBS_DIR}/sfml/include/")

elseif(LIB_SFML STREQUAL UsingFindPackage)

    #message(FATAL_ERROR "SFML FIND PACKAGE NOT IMPLEMENTED")

    message(STATUS "SFML FIND PACKAGE - Uses Target to link with:")
    message(STATUS "  sfml-window sfml-graphics sfml-audio sfml-network sfml-system")

    # tool_is_lib(assimp assimp_registered)
    # if (NOT ${assimp_registered})

    #     add_library(assimp OBJECT ${assimp_LIBRARIES})
    #     target_link_libraries(assimp ${assimp_LIBRARIES})
    #     include_directories(${assimp_INCLUDE_DIRS})

    #     # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    #     set_target_properties(assimp PROPERTIES FOLDER "LIBS")

    #     tool_register_lib(assimp)

    # endif()

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()
