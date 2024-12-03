cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(MyLibrary VERSION 1.0 LANGUAGES CXX)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


set(MinhookPath ${CMAKE_CURRENT_SOURCE_DIR}/minhook)

include_directories(
    ${MinhookPath}/include
)

link_directories(
    ${MinhookPath}/lib
)

# 添加源文件
add_library(MyLibrary SHARED
    src/MyLibrary.cpp
 "tests/target_test.cpp")

target_link_libraries(MyLibrary minhook.x64)

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