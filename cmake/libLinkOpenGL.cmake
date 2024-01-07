if (TARGET LinkOpenGL)
    return()
endif()

set( LIB_OPENGL UsingFindPackage CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_OPENGL PROPERTY STRINGS None UsingFindPackage)

if(LIB_OPENGL STREQUAL UsingFindPackage)

    if (NOT TARGET LinkOpenGL)
        message(STATUS "[LIB_OPENGL] using system lib.")

        if (ITK_RPI)
            set(OPENGL_LIBRARIES ${GLES_LIBRARY})
        else()
            set(OpenGL_GL_PREFERENCE GLVND)
            find_package(OpenGL REQUIRED QUIET)
            include_directories(${OPENGL_INCLUDE_DIR})
        endif()
        
        add_library(LinkOpenGL INTERFACE)
        #add_library(LinkOpenGL OBJECT ${OPENGL_LIBRARIES})
        target_link_libraries(LinkOpenGL INTERFACE ${OPENGL_LIBRARIES})
        
        # set the target's folder (for IDEs that support it, e.g. Visual Studio)
        #set_target_properties(LinkOpenGL PROPERTIES FOLDER "LIBS")
    endif()

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()
