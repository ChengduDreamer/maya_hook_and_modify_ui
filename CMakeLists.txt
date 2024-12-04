cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(MyLibrary VERSION 1.0 LANGUAGES CXX)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(QT_ROOT F:/Qt5/5.15/5.15.2/msvc2019_64)

set(CMAKE_PREFIX_PATH ${QT_ROOT}/lib/cmake)

set(MinhookPath ${CMAKE_CURRENT_SOURCE_DIR}/minhook)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS WebSockets REQUIRED)
find_package(Qt5 COMPONENTS Sql REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

include_directories(
    ${MinhookPath}/include
    ${QT_ROOT}/include
)

link_directories(
    ${MinhookPath}/lib
)

# 添加源文件
add_library(MyLibrary SHARED
    src/MyLibrary.cpp
 )

 add_library(MyLibrary2 SHARED
    src/MyLibrary2.cpp
 )

target_link_libraries(MyLibrary minhook.x64  )

target_link_libraries(MyLibrary2 minhook.x64 Qt5::Widgets Qt5::Core)

# 设置库的公共头文件路径
target_include_directories(MyLibrary PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# 设置版本信息
set_target_properties(MyLibrary PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 安装规则
install(TARGETS MyLibrary
    EXPORT MyLibraryTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(EXPORT MyLibraryTargets
    FILE MyLibraryTargets.cmake
    NAMESPACE MyLibrary::
    DESTINATION lib/cmake/MyLibrary
)

install(DIRECTORY src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 添加测试
enable_testing()
add_subdirectory(tests)