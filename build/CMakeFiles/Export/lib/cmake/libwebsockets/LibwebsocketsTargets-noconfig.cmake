#----------------------------------------------------------------
# Generated CMake target import file for configuration "".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "websockets" for configuration ""
set_property(TARGET websockets APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(websockets PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "/usr/lib/libz.dylib;/usr/lib/libssl.dylib;/usr/lib/libcrypto.dylib;m"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libwebsockets.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS websockets )
list(APPEND _IMPORT_CHECK_FILES_FOR_websockets "${_IMPORT_PREFIX}/lib/libwebsockets.a" )

# Import target "websockets_shared" for configuration ""
set_property(TARGET websockets_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(websockets_shared PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "/usr/lib/libz.dylib;/usr/lib/libssl.dylib;/usr/lib/libcrypto.dylib;m"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libwebsockets.dylib"
  IMPORTED_SONAME_NOCONFIG "/usr/local/lib/libwebsockets.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS websockets_shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_websockets_shared "${_IMPORT_PREFIX}/lib/libwebsockets.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
