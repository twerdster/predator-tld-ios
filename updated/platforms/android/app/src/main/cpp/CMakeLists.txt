cmake_minimum_required(VERSION 3.22)
project(predator_android LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY JOB_POOLS predator_compile=2 predator_link=1)
set(CMAKE_JOB_POOL_COMPILE predator_compile)
set(CMAKE_JOB_POOL_LINK predator_link)
set(OpenCV_STATIC ON)
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc)
get_filename_component(PREDATOR_UPDATED "${CMAKE_CURRENT_LIST_DIR}/../../../../../.." ABSOLUTE)
add_library(predator_android SHARED
    bridge.cpp
    "${PREDATOR_UPDATED}/platforms/common/PredatorSession.cpp"
    "${PREDATOR_UPDATED}/src/Tld.cpp"
    "${PREDATOR_UPDATED}/src/Tracker.cpp"
    "${PREDATOR_UPDATED}/src/TldDetector.cpp"
    "${PREDATOR_UPDATED}/src/Classifier.cpp"
    "${PREDATOR_UPDATED}/src/Fern.cpp"
    "${PREDATOR_UPDATED}/src/PointTest.cpp"
    "${PREDATOR_UPDATED}/src/TldImage.cpp"
    "${PREDATOR_UPDATED}/src/Utils.cpp")
target_include_directories(predator_android PRIVATE
    "${PREDATOR_UPDATED}/src" "${PREDATOR_UPDATED}/platforms/common")
target_include_directories(predator_android SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(predator_android PRIVATE ${OpenCV_LIBS} log)
