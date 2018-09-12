project(scpFwd.exe)

cmake_minimum_required(VERSION 3.10)
SET(CMAKE_CXX_FLAGS "-std=c++11")

add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DSOURCE_CODE_LOCATION="${CMAKE_SOURCE_DIR}")

set(SOURCE_FILES scpForward.cpp dstorscp2.h dstorscp2.cpp loadConfig.h loadConfig.cpp watchDirs.hpp watchDirs.cpp mapping.hpp mapping.cpp loadProcessingConfig.h loadProcessingConfig.cpp md5.h)
find_package(DCMTK REQUIRED)

include_directories(${DCMTK_INCLUDE_DIRS})
include_directories(/opt/local/include/boost)

#add_library(${PROJECT_NAME} ${SOURCE_FILES})
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(APPLE)
    target_link_libraries(${PROJECT_NAME} /opt/local/lib/libz.1.2.11.dylib)
    target_link_libraries(${PROJECT_NAME} /opt/local/lib/libboost_system-mt.dylib)
    target_link_libraries(${PROJECT_NAME} /opt/local/lib/libboost_filesystem-mt.dylib)
else()
    target_link_libraries(${PROJECT_NAME} /lib64/libz.so)
    target_link_libraries(${PROJECT_NAME} /lib64/libboost_system-mt.so)
    target_link_libraries(${PROJECT_NAME} /lib64/libboost_filesystem-mt.so)
endif()
target_link_libraries(${PROJECT_NAME} ${DCMTK_LIBRARIES})


# Install this example rehearsal code along with this CMakeLists.txt file
install(FILES ${SOURCE_FILES} DESTINATION "scpFwd.exe")
install(FILES "CMakeLists.txt" DESTINATION "scpFwd.exe")
