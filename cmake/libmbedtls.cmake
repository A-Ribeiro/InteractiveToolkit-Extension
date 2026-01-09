if (TARGET mbedtls OR TARGET mbedx509 OR TARGET tfpsacrypto OR TARGET mbedcrypto)
    return()
endif()

#include(${CMAKE_SOURCE_DIR}/libs/InteractiveToolkit-Extension/cmake/tool.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tool.cmake)

set( LIB_MBEDTLS TryFindPackageFirst CACHE STRING "Choose the Library Source." )
set_property(CACHE LIB_MBEDTLS PROPERTY STRINGS None TryFindPackageFirst UsingFindPackage FromSource)

tool_append_if_not_exists_and_make_global(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")

if(LIB_MBEDTLS STREQUAL TryFindPackageFirst)
    find_package(MbedTLS COMPONENTS mbedtls tfpsacrypto mbedx509 QUIET)
    if (MBEDTLS_FOUND)
        message(STATUS "[LIB_MBEDTLS] using system lib.")
        set(LIB_MBEDTLS UsingFindPackage)
    else()
        message(STATUS "[LIB_MBEDTLS] compiling from source.")
        set(LIB_MBEDTLS FromSource)
    endif()
endif()

if(LIB_MBEDTLS STREQUAL FromSource)

    message(STATUS "")
    message(STATUS "BUILDING MBEDTLS FROM SOURCE")
    message(STATUS "")
    message(STATUS "run: pip3 install jsonschema jinja2")
    message(STATUS "")

    
    
    tool_download_git_package("https://github.com/Mbed-TLS/mbedtls.git" mbedtls)

    # Mbed TLS Config Patch
    tool_get_dirs(mbedtls_DOWNLOADED_PATH mbedtls_BINARY_PATH mbedtls)

    # - crypto_config.h (for version >= 4)
    if(EXISTS "${mbedtls_DOWNLOADED_PATH}/tf-psa-crypto/include/psa/crypto_config.h")
        file(READ "${mbedtls_DOWNLOADED_PATH}/tf-psa-crypto/include/psa/crypto_config.h" MBEDTLS_CONFIG_CONTENTS)
        set(MBEDTLS_CONFIG_CONTENTS_BEFORE "${MBEDTLS_CONFIG_CONTENTS}")
        
        string(REPLACE "//#define MBEDTLS_DEPRECATED_REMOVED" "#define MBEDTLS_DEPRECATED_REMOVED" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
        string(REPLACE "//#define MBEDTLS_CHECK_RETURN_WARNING" "#define MBEDTLS_CHECK_RETURN_WARNING" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
        string(REPLACE "//#define MBEDTLS_THREADING_ALT" "#define MBEDTLS_THREADING_ALT" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
        string(REPLACE "//#define MBEDTLS_THREADING_C" "#define MBEDTLS_THREADING_C" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
        
        if (NOT MBEDTLS_CONFIG_CONTENTS_BEFORE STREQUAL MBEDTLS_CONFIG_CONTENTS)
            file(WRITE "${mbedtls_DOWNLOADED_PATH}/tf-psa-crypto/include/psa/crypto_config.h" "${MBEDTLS_CONFIG_CONTENTS}")
        endif()

        # - threading_alt.h
        if(NOT EXISTS "${mbedtls_DOWNLOADED_PATH}/tf-psa-crypto/include/mbedtls/threading_alt.h")
            file(WRITE "${mbedtls_DOWNLOADED_PATH}/tf-psa-crypto/include/mbedtls/threading_alt.h" "#ifndef __custom_threading_alt_h__\n#define __custom_threading_alt_h__\ntypedef void* mbedtls_platform_mutex_t;\ntypedef void* mbedtls_platform_condition_variable_t;\n#endif\n")
        endif()
    else()
        # - mbedtls_config.h (for version < 4)
        if(EXISTS "${mbedtls_DOWNLOADED_PATH}/include/mbedtls/mbedtls_config.h")
            file(READ "${mbedtls_DOWNLOADED_PATH}/include/mbedtls/mbedtls_config.h" MBEDTLS_CONFIG_CONTENTS)
            set(MBEDTLS_CONFIG_CONTENTS_BEFORE "${MBEDTLS_CONFIG_CONTENTS}")

            string(REPLACE "//#define MBEDTLS_DEPRECATED_REMOVED" "#define MBEDTLS_DEPRECATED_REMOVED" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
            string(REPLACE "//#define MBEDTLS_CHECK_RETURN_WARNING" "#define MBEDTLS_CHECK_RETURN_WARNING" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
            string(REPLACE "//#define MBEDTLS_THREADING_ALT" "#define MBEDTLS_THREADING_ALT" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
            string(REPLACE "//#define MBEDTLS_THREADING_C" "#define MBEDTLS_THREADING_C" MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")

            if (NOT MBEDTLS_CONFIG_CONTENTS_BEFORE STREQUAL MBEDTLS_CONFIG_CONTENTS)
                file(WRITE "${mbedtls_DOWNLOADED_PATH}/include/mbedtls/mbedtls_config.h" "${MBEDTLS_CONFIG_CONTENTS}")
            endif()
        
            # - threading_alt.h
            if(NOT EXISTS "${mbedtls_DOWNLOADED_PATH}/include/mbedtls/threading_alt.h")
                file(WRITE "${mbedtls_DOWNLOADED_PATH}/include/mbedtls/threading_alt.h" "#ifndef __custom_threading_alt_h__\n#define __custom_threading_alt_h__\ntypedef void* mbedtls_threading_mutex_t;\n#endif\n")
            endif()
        endif()
    endif()


    option(ENABLE_TESTING "Build Mbed TLS tests." OFF)
    option(USE_STATIC_MBEDTLS_LIBRARY "Build Mbed TLS static library." ON)
    option(USE_SHARED_MBEDTLS_LIBRARY "Build Mbed TLS shared library." OFF)

    set(MBEDTLS_AS_SUBPROJECT ON)
    set(TF_PSA_CRYPTO_AS_SUBPROJECT ON)

    option(USE_STATIC_TF_PSA_CRYPTO_LIBRARY "Build TF-PSA-Crypto static library." ON)
    option(USE_SHARED_TF_PSA_CRYPTO_LIBRARY "Build TF-PSA-Crypto shared library." OFF)
    if (WIN32)
    option(MSVC_STATIC_RUNTIME "Build the libraries with /MT compiler flag" ON)
    endif()
    
    # Enable GEN_FILES on Windows to generate required files like psa_crypto_driver_wrappers_no_static.c
    option(GEN_FILES "Generate the auto-generated files as needed" ON)
    option(ENABLE_PROGRAMS "Build Mbed TLS programs." OFF)
    # set(ENABLE_PROGRAMS OFF CACHE BOOL "Build Mbed TLS programs." FORCE)

    tool_include_lib(mbedtls)

    if (TARGET builtin)
        # target_compile_definitions(builtin PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(builtin PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(builtin PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET everest)
        # target_compile_definitions(everest PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(everest PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(everest PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET lib)
        # target_compile_definitions(lib PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(lib PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(lib PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET libmbedtls_generated_files_target)
        # target_compile_definitions(libmbedtls_generated_files_target PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(libmbedtls_generated_files_target PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(libmbedtls_generated_files_target PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET libmbedx509_generated_files_target)
        # target_compile_definitions(libmbedx509_generated_files_target PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(libmbedx509_generated_files_target PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(libmbedx509_generated_files_target PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET libtfpsacrypto_generated_files_target)
        # target_compile_definitions(libtfpsacrypto_generated_files_target PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(libtfpsacrypto_generated_files_target PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(libtfpsacrypto_generated_files_target PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET mbedtls-apidoc)
        # target_compile_definitions(mbedtls-apidoc PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(mbedtls-apidoc PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(mbedtls-apidoc PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET tfpsacrypto-apidoc)
        # target_compile_definitions(tfpsacrypto-apidoc PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(tfpsacrypto-apidoc PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(tfpsacrypto-apidoc PROPERTIES CXX_STANDARD 11)
    endif()


    if (TARGET mbedtls)
        # target_compile_definitions(mbedtls PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(mbedtls PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(mbedtls PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET mbedx509)
        # target_compile_definitions(mbedx509 PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(mbedx509 PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(mbedx509 PROPERTIES CXX_STANDARD 11)
    endif()

    if (TARGET p256-m)
        # target_compile_definitions(p256-m PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(p256-m PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(p256-m PROPERTIES CXX_STANDARD 11)
    endif()
    
    if (TARGET tfpsacrypto)
        # target_compile_definitions(tfpsacrypto PUBLIC -DMBEDTLS_STATIC)
        set_target_properties(tfpsacrypto PROPERTIES FOLDER "LIBS/MBEDTLS")
        set_target_properties(tfpsacrypto PROPERTIES CXX_STANDARD 11)
    endif()
    
    # include_directories("${ARIBEIRO_LIBS_DIR}/sfml/include/")

    message(STATUS "MbedTLS From Source - Link with:")
    message(STATUS "  mbedtls tfpsacrypto mbedx509")

    add_library(mbedtls-all INTERFACE)
    target_link_libraries(mbedtls-all INTERFACE mbedtls tfpsacrypto mbedx509)

    if(WIN32)
        # SystemCertificates on Windows
        target_link_libraries(mbedtls-all INTERFACE Crypt32)
    elseif(APPLE)
        find_library(SECURITY_FRAMEWORK Security)
        find_library(COREFRAMEWORK_FRAMEWORK CoreFoundation)
        target_link_libraries(mbedtls-all INTERFACE ${SECURITY_FRAMEWORK} ${COREFRAMEWORK_FRAMEWORK})
    endif()

elseif(LIB_MBEDTLS STREQUAL UsingFindPackage)

    message(STATUS "MbedTLS FIND PACKAGE - Link with:")
    message(STATUS "  mbedtls mbedcrypto mbedx509")

    add_library(mbedtls-all INTERFACE)
    target_link_libraries(mbedtls-all INTERFACE mbedtls mbedcrypto mbedx509)

    if(WIN32)
        # SystemCertificates on Windows
        target_link_libraries(mbedtls-all INTERFACE Crypt32)
    elseif(APPLE)
        find_library(SECURITY_FRAMEWORK Security)
        find_library(COREFRAMEWORK_FRAMEWORK CoreFoundation)
        target_link_libraries(mbedtls-all INTERFACE ${SECURITY_FRAMEWORK} ${COREFRAMEWORK_FRAMEWORK})
    endif()

else()
    message( FATAL_ERROR "You need to specify the lib source." )
endif()
