project(introns_db_fill)
cmake_minimum_required(VERSION 3.0)
find_package(Qt4 COMPONENTS QtCore QtSql REQUIRED)
find_package(ZLIB REQUIRED)

include(${QT_USE_FILE})

set(CMAKE_CXX_FLAGS -std=c++11)

include_directories(${QT_INCLUDES})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
    database.cpp
    gbkparser.cpp
    gzipreader.cpp
    iniparser.cpp
    main.cpp
)


add_executable(introns_db_fill ${SOURCES})
target_link_libraries(introns_db_fill ${QT_LIBRARIES} ${ZLIB_LIBRARIES})
