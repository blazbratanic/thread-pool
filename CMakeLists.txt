cmake_minimum_required(VERSION 2.8.8)
set(PROJECT_NAME_STR unit_test)
project(${PROJECT_NAME_STR} C CXX)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-Wall -ansi -Wno-deprecated -Werror -pthread -std=c++11)
endif()

if(MSVC)
    #vc 2012 fix for vararg templates
    set(MSVC_COMPILER_DEFS "-D_VARIADIC_MAX=10") 
endif() 
#-------------------
# set common include folder for module
#-------------------
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)

#-------------------
# Module source
#-------------------
add_subdirectory(${EXT_PROJECTS_DIR}/googletest)
set(GTEST_INCLUDE_DIRS ${gtest_SOURCE_DIR}/include)
set(GTEST_LIBS_DIR ${gtest_BINARY_DIR})

#-------------------
# Tests
#-------------------
file(GLOB tests ${PROJECT_SOURCE_DIR}/test/*.cpp)
foreach(test ${tests})
  include_directories(${GTEST_INCLUDE_DIRS} ${COMMON_INCLUDES})
  get_filename_component(test_name ${test} NAME_WE)
  add_executable(${test_name} ${test})
  add_dependencies(${test_name} gtest)
  add_dependencies(${test_name} gtest_main)

  target_link_libraries(${test_name} 
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GTEST_LIBS_DIR}/libgtest_main.a 
    pthread)

  if(RT_LIBRARY)
    target_link_libraries(${test} ${RT_LIBRARY} )
  endif()
  if(WIN32)
    add_test(NAME ${test} WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH} COMMAND ${test})
  else()
    add_test(NAME ${test} COMMAND ${test})
  endif()
endforeach()
