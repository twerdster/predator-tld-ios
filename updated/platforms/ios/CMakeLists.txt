cmake_minimum_required(VERSION 3.20)
project(PredatorUpdated LANGUAGES CXX OBJCXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OPENCV_IOS_FRAMEWORK_DIR "" CACHE PATH "Directory containing official OpenCV 4 opencv2.framework")
set(PREDATOR_DEVELOPMENT_TEAM "" CACHE STRING "Private signing team, leave empty for unsigned build")
set(PREDATOR_BUNDLE_IDENTIFIER "org.awetzler.predator.updated" CACHE STRING "Application bundle identifier")
if(NOT EXISTS "${OPENCV_IOS_FRAMEWORK_DIR}/opencv2.framework/Headers/core/version.hpp")
  message(FATAL_ERROR "Set OPENCV_IOS_FRAMEWORK_DIR to a modern iOS OpenCV framework directory")
endif()
set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_executable(PredatorUpdated MACOSX_BUNDLE App/main.mm ../common/PredatorSession.cpp
  ${ROOT}/src/Tld.cpp ${ROOT}/src/Tracker.cpp ${ROOT}/src/TldDetector.cpp
  ${ROOT}/src/Classifier.cpp ${ROOT}/src/Fern.cpp ${ROOT}/src/PointTest.cpp
  ${ROOT}/src/TldImage.cpp ${ROOT}/src/Utils.cpp ../data/camel-320x180.gray)
set_source_files_properties(../data/camel-320x180.gray PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
target_include_directories(PredatorUpdated PRIVATE ${ROOT}/src ../common)
target_compile_options(PredatorUpdated PRIVATE "-F${OPENCV_IOS_FRAMEWORK_DIR}")
target_link_libraries(PredatorUpdated PRIVATE "${OPENCV_IOS_FRAMEWORK_DIR}/opencv2.framework"
  "-framework UIKit" "-framework Foundation" "-framework CoreGraphics" "-framework QuartzCore"
  "-framework Accelerate" z)
set_target_properties(PredatorUpdated PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/App/Info.plist"
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 15.0
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${PREDATOR_BUNDLE_IDENTIFIER}"
  XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
  XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${PREDATOR_DEVELOPMENT_TEAM}"
  XCODE_ATTRIBUTE_ENABLE_BITCODE NO
  XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator")

file(GLOB_RECURSE PREDATOR_NOTICES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../data/licenses/*")
foreach(notice IN LISTS PREDATOR_NOTICES)
  file(RELATIVE_PATH relative "${CMAKE_CURRENT_SOURCE_DIR}/../data/licenses" "${notice}")
  get_filename_component(directory "${relative}" DIRECTORY)
  set_source_files_properties("${notice}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/licenses/${directory}")
endforeach()
target_sources(PredatorUpdated PRIVATE ${PREDATOR_NOTICES})
