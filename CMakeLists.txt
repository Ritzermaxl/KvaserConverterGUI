cmake_minimum_required(VERSION 3.12)
project(ConverterGUI)
set(CMAKE_CXX_STANDARD 17)

# Include directory for cyaml
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/cyaml)

# Build hello_imgui
# =================
# 1/  Option 1: if you added hello_imgui as a subfolder, you can add it to your project with:
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/hello_imgui)
    add_subdirectory(external/hello_imgui)
endif()

# 2/  Option 2: simply fetch hello_imgui during the build
if (NOT TARGET hello_imgui)
    message(STATUS "Fetching hello_imgui")
    include(FetchContent)
    FetchContent_Declare(hello_imgui GIT_REPOSITORY https://github.com/pthom/hello_imgui.git GIT_TAG master)
    FetchContent_MakeAvailable(hello_imgui)
endif()


# Build your app
# ==============
set(SOURCE_FILES
    ./src/main.cpp
    ./src/converter.cpp
    ./src/converter.h
)

hello_imgui_add_app(ConverterGUI ${SOURCE_FILES})


set(CANLIB_INCLUDE_DIR "C:/Program Files (x86)/Kvaser/Canlib/INC")
set(CANLIB_LIB_DIR_X86 "C:/Program Files (x86)/Kvaser/Canlib/Lib/MS")
set(CANLIB_LIB_DIR_X64 "C:/Program Files (x86)/Kvaser/Canlib/Lib/x64")

include_directories(${CANLIB_INCLUDE_DIR})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit system
    target_link_directories(${PROJECT_NAME} PRIVATE ${CANLIB_LIB_DIR_X64})
    target_link_libraries(${PROJECT_NAME} PRIVATE canlib32.lib kvmlib.lib kvlclib.lib)
else()
    # 32-bit system
    target_link_directories(${PROJECT_NAME} PRIVATE ${CANLIB_LIB_DIR_X86})
    target_link_libraries(${PROJECT_NAME} PRIVATE canlib32.lib kvmlib.lib kvlclib.lib)
endif()

# Link cyaml to your app
#target_link_libraries(hello_world PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcyaml.a)

# hello_imgui_add_app is a helper function, similar to cmake's "add_executable"
# Usage:
# hello_imgui_add_app(app_name file1.cpp file2.cpp ...)
#
# Features: 
# * It will automatically link the target to the required libraries (hello_imgui, OpenGl, glad, etc)
# * It will embed the assets (for desktop, mobile, and emscripten apps)
# * It will perform additional customization (app icon and name on mobile platforms, etc)

# Now you can build your app with
#     mkdir build && cd build && cmake .. && cmake --build .
