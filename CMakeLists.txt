cmake_minimum_required(VERSION 3.6)
project(ResearchMac)

enable_language(C)
enable_language(CXX)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=gnu11")
#set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_STANDARD 17)

include_directories(
        /usr/local/fxbrainossl/include
)

#FIND_LIBRARY(APR_LIBRARY
#        NAMES ${APR_NAMES}
#        HINTS
#        /usr/local/opt/apr/libexec/lib
#        PATHS
#        )
#
#FIND_PATH(APR_INCLUDE_DIR apr.h
#        /usr/local/opt/apr/libexec/include/apr-1/
#        )
#
#SET(APR_NAMES ${APR_NAMES} apr-1)
#FIND_LIBRARY(APR_LIBRARY
#        NAMES ${APR_NAMES}
#        HINTS
#        /usr/local/opt/apr/libexec/lib
#        PATHS
#        )

set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
FIND_PACKAGE(OpenSSL)

find_package(Boost 1.66.0)

# set the POCO paths and libs
#set(POCO_PREFIX      "/usr/local/Cellar/poco/1.8.1") # the directory containing "include" and "lib"
#set(POCO_INCLUDE_DIR "${POCO_PREFIX}/include")
#set(POCO_LIB_DIR     "${POCO_PREFIX}/lib")
#set(POCO_LIBS        "${POCO_LIB_DIR}/libPocoNetd.a"
#        "${POCO_LIB_DIR}/libPocoUtild.a"
#        "${POCO_LIB_DIR}/libPocoJSONd.a"
#        "${POCO_LIB_DIR}/libPocoXMLd.a"
#        "${POCO_LIB_DIR}/libPocoFoundationd.a"
#        "pthread")

# /Cellar/mongo-c-driver/1.9.2/lib/cmake/libmongoc-1.0/libmongoc-1.0-config.cmake
# ./Cellar/mongo-c-driver/1.9.2/lib/cmake/libmongoc-1.0/libmongoc-1.0-config-version.cmake
# ./Cellar/mongo-c-driver/1.9.2/lib/cmake/libbson-1.0/libbson-1.0-config-version.cmake
#FIND_PACKAGE(libmongoc-1.0 1.9.2 REQUIRED)
##
#IF (APR_LIBRARY AND APR_INCLUDE_DIR)
#    SET(APR_LIBRARIES ${APR_LIBRARY})
#    SET(APR_FOUND "YES")
#ELSE (APR_LIBRARY AND APR_INCLUDE_DIR)
#    SET(APR_FOUND "NO")
#ENDIF (APR_LIBRARY AND APR_INCLUDE_DIR)

# PCRE names its libraries differently for debug vs. release builds.
# We can't query our own CMAKE_BUILD_TYPE at configure time.
# If the debug version exists in PREFIX/lib, default to that one.
IF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib/pcred.lib")
    SET(default_pcre_libraries ${CMAKE_INSTALL_PREFIX}/lib/pcred.lib)
ELSE()
    SET(default_pcre_libraries ${CMAKE_INSTALL_PREFIX}/lib/pcre.lib)
ENDIF()

SET(PCRE_INCLUDE_DIR      "${CMAKE_INSTALL_PREFIX}/include" CACHE STRING "Directory with PCRE include files")
SET(PCRE_LIBRARIES        ${default_pcre_libraries}      CACHE STRING "PCRE libraries to link with")

# Deprecated declarations.
SET (NATIVE_APR_INCLUDE_PATH ${APR_INCLUDE_DIR} )
#GET_FILENAME_COMPONENT (NATIVE_APR_LIB_PATH ${APR_LIBRARY} PATH)

#MARK_AS_ADVANCED(
#        APR_LIBRARY
#        APR_INCLUDE_DIR
#)

# Next, APRUTIL.
#
#FIND_PATH(APRUTIL_INCLUDE_DIR apu.h
#        /usr/local/opt/apr-util/libexec/include/apr-1
#        )
#
#SET(APRUTIL_NAMES ${APRUTIL_NAMES} aprutil-1)
#FIND_LIBRARY(APRUTIL_LIBRARY
#        NAMES ${APRUTIL_NAMES}
#        HINTS
#        /usr/local/opt/apr-util/libexec/lib/
#        PATHS
#        )
#
#IF (APRUTIL_LIBRARY AND APRUTIL_INCLUDE_DIR)
#    SET(APRUTIL_LIBRARIES ${APRUTIL_LIBRARY})
#    SET(APRUTIL_FOUND "YES")
#ELSE (APRUTIL_LIBRARY AND APRUTIL_INCLUDE_DIR)
#    SET(APRUTIL_FOUND "NO")
#ENDIF (APRUTIL_LIBRARY AND APRUTIL_INCLUDE_DIR)
#
#IF (APRUTIL_FOUND)
#    IF (NOT APRUTIL_FIND_QUIETLY)
#
#    ENDIF (NOT APRUTIL_FIND_QUIETLY)
#ELSE (APRUTIL_FOUND)
#    IF (APRUTIL_FIND_REQUIRED)
#        MESSAGE(FATAL_ERROR "Could not find APRUTIL library")
#    ENDIF (APRUTIL_FIND_REQUIRED)
#ENDIF (APRUTIL_FOUND)

## Deprecated declarations.
#SET (NATIVE_APRUTIL_INCLUDE_PATH ${APRUTIL_INCLUDE_DIR} )
#GET_FILENAME_COMPONENT (NATIVE_APRUTIL_LIB_PATH ${APRUTIL_LIBRARY} PATH)
#
#MARK_AS_ADVANCED(
#        APRUTIL_LIBRARY
#        APRUTIL_INCLUDE_DIR
#)

#IF (APR_FOUND)
#    IF (NOT APR_FIND_QUIETLY)
        MESSAGE(STATUS "  Found APR headers................ : ${APR_INCLUDE_DIR}")
        MESSAGE(STATUS "  Found APR library................ : ${APR_LIBRARIES}")
        MESSAGE(STATUS "  Found APRUTIL headers............ : ${APRUTIL_INCLUDE_DIR}")
        MESSAGE(STATUS "  Found APRUTIL library............ : ${APRUTIL_LIBRARIES}")
        MESSAGE(STATUS "  APR include directory ........... : ${APR_INCLUDE_DIR}")
        MESSAGE(STATUS "  APR libraries ................... : ${APR_LIBRARIES}")
        MESSAGE(STATUS "  OpenSSL include directory ....... : ${OPENSSL_INCLUDE_DIR}")
        MESSAGE(STATUS "  OpenSSL libraries ............... : ${OPENSSL_LIBRARIES}")
        MESSAGE(STATUS "  PCRE include directory .......... : ${PCRE_INCLUDE_DIR}")
        MESSAGE(STATUS "  PCRE libraries .................. : ${PCRE_LIBRARIES}")
        MESSAGE(STATUS "  MONGOC version .................. : ${MONGOC_VERSION}")
        MESSAGE(STATUS "  MONGOC include path ............. : ${MONGOC_INCLUDE_DIRS}")
        MESSAGE(STATUS "  MONGOC libraries ................ : ${MONGOC_LIBRARIES}")
        MESSAGE(STATUS "  POCO libraries .................. : ${POCO_LIB_DIR}")
        MESSAGE(STATUS "  POCO include path ................: ${POCO_INCLUDE_DIR}")

#    ENDIF (NOT APR_FIND_QUIETLY)
#ELSE (APR_FOUND)
#    IF (APR_FIND_REQUIRED)
#        MESSAGE(FATAL_ERROR "Could not find APR library")
#    ENDIF (APR_FIND_REQUIRED)
#ENDIF (APR_FOUND)



set(SOURCE_FILES keystore.c)
add_executable(ResearchMac ${SOURCE_FILES})

#target_include_directories (ResearchMac PRIVATE "${OPENSSL_INCLUDE_DIR}" "${OPENSSL_INCLUDE_DIR}")
#target_link_libraries (ResearchMac PRIVATE "${OPENSSL_LIBRARIES}")
#target_compile_definitions (ResearchMac PRIVATE "${MONGOC_DEFINITIONS}")

IF(OPENSSL_FOUND)
    TARGET_LINK_LIBRARIES(ResearchMac ${OPENSSL_LIBRARIES})
ENDIF()

IF(Boost_FOUND)
    target_include_directories(ResearchMac PRIVATE "${Boost_INCLUDE_DIRS}")
    TARGET_LINK_LIBRARIES(ResearchMac ${Boost_LIBRARIES})
ENDIF()

set(CMAKE_CXX_FLAGS "$(CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -headerpad_max_install_names -framework CoreFoundation \
-framework IOKit -framework CoreServices -framework SystemConfiguration -framework Security -lapr-1 -laprutil-1 -lpcre")