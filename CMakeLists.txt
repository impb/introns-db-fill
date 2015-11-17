project(introns_db_fill)
cmake_minimum_required(VERSION 3.0)
find_package(ZLIB REQUIRED)

if(NOT DEFINED USE_QT)
    set(USE_QT 4)
endif()

if(${USE_QT} GREATER 4)
    find_package(Qt5 5.3.0 COMPONENTS Core Sql REQUIRED)
    include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Sql_INCLUDE_DIRS})
    set(QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Sql_LIBRARIES})
else()
    find_package(Qt4 COMPONENTS QtCore QtSql REQUIRED)
    include(${QT_USE_FILE})
    include_directories(${QT_INCLUDES})
endif()

set(CMAKE_CXX_FLAGS -std=c++11)

include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
    database.cpp
    gbkparser.cpp
    gzipreader.cpp
    iniparser.cpp
    main.cpp
    logger.cpp
)


add_executable(introns_db_fill ${SOURCES})
target_link_libraries(introns_db_fill ${QT_LIBRARIES} ${ZLIB_LIBRARIES})
