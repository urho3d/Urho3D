#
# Copyright (c) 2008-2020 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Define target name
set (TARGET_NAME Urho3D)

if ((APPLE AND NOT ARM) OR WIN32)
    
    set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
    include (FindUrho3D)
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
    # Add tests framework
    # https://github.com/google/googletest/blob/master/googletest/README.md
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

    # Download and unpack googletest at configure time
    configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )

    if(result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )

    if(result)
        message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                    EXCLUDE_FROM_ALL)

    # The gtest/gtest_main targets carry header search path
    # dependencies automatically when using CMake 2.8.11 or
    # later. Otherwise we have to add them here ourselves.
    if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
    endif()

    include(CTest)



    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
    # Add tests target
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
    
    # Define source files 
    # Taken from sample cmake files - I believe it setup some magic inside
    define_source_files (EXTRA_H_FILES ${COMMON_SAMPLE_H_FILES})
    define_dependency_libs (Urho3D)
   
    # Tests sources
    file(GLOB_RECURSE PROJECT_TEST_SOURCES 
        ${CMAKE_SOURCE_DIR}/Source/Tests/*.*
    )
    
    add_executable(${TARGET_NAME}Tests ${PROJECT_TEST_SOURCES})
    add_dependencies(${TARGET_NAME}Tests gtest)
    
    define_dependency_libs (${TARGET_NAME}Tests)
    target_link_libraries (${TARGET_NAME}Tests ${ABSOLUTE_PATH_LIBS} ${LIBS})

    message(URHO3D_LIBRARIES)
    message(${URHO3D_LIBRARIES})

    # Include Urho3D headers files is weird... please fix it!
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../include)
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../include/Urho3D/ThirdParty)

    # Include google test and goolge mock frameworks
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/../googletest-src/googletest/include)
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/../googletest-src/googlemock/include)

    # Link testing framework
    target_link_libraries(${TARGET_NAME}Tests gtest_main)
    target_link_libraries(${TARGET_NAME}Tests gmock_main)
    target_link_libraries(${TARGET_NAME}Tests gmock_main)

endif()
