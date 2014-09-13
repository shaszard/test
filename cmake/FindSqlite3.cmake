find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLITE3 DEFAULT_MSG SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR)

if(SQLITE3_FOUND)
	set(SQLite3_LIBRARIES ${SQLITE3_LIBRARY})
	set(SQLite3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
else()
	set(SQLite3_LIBRARIES)
	set(SQLite3_INCLUDE_DIRS)
endif()

mark_as_advanced(SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR)
