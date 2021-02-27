message("Searching for LibUCL")

pkg_check_modules(ucl QUIET IMPORTED_TARGET UCL)

find_path(UCL_INCLUDE_DIR ucl.h
	PATHS
	  "/usr/include"
	  "/usr/local/include"
	  ${UCL_INCLUDEDIR}
	)

find_library(UCL_LIBRARY
	NAMES libucl.so libucl.a
	PATHS
	  "/usr/lib"
	  "/usr/local/lib"
	  ${UCL_LIBDIR}
	)

if (UCL_INCLUDE_DIR AND UCL_LIBRARY)
	set(UCL_FOUND TRUE)
	set(UCL_LIBRARIES ${UCL_LIBRARY})
	set(UCL_INCLUDE_DIRS ${UCL_INCLUDE_DIR})
else ()
	message("LibUCL not found")
	set(UCL_FOUND FALSE)
endif (UCL_INCLUDE_DIR AND UCL_LIBRARY)

find_package_handle_standard_args(UCL DEFAULT_MSG
	UCL_LIBRARIES
	UCL_INCLUDE_DIRS)


if(UCL_FOUND AND NOT TARGET UCL::UCL)
	add_library(UCL::UCL UNKNOWN IMPORTED)
	set_property(TARGET UCL::UCL APPEND PROPERTY
	  IMPORTED_LOCATION "${UCL_LIBRARY}")
	set_target_properties(UCL::UCL PROPERTIES
	  INTERFACE_INCLUDE_DIRECTORIES "${UCL_INCLUDE_DIRS}")
endif()