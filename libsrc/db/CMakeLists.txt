add_library(database
	${CMAKE_SOURCE_DIR}/include/db/AuthTable.h
	${CMAKE_SOURCE_DIR}/include/db/DBManager.h
	${CMAKE_SOURCE_DIR}/include/db/InstanceTable.h
	${CMAKE_SOURCE_DIR}/include/db/MetaTable.h
	${CMAKE_SOURCE_DIR}/include/db/SettingsTable.h
	${CMAKE_SOURCE_DIR}/libsrc/db/DBManager.cpp
)

target_link_libraries(database
	hyperion
	hyperion-utils
	Qt${QT_VERSION_MAJOR}::Sql
)
