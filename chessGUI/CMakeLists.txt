cmake_minimum_required(VERSION 3.14)

# Project Name
project(chessGUI)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Engine core. Pulled in automatically by the top-level build; when this module
# is configured on its own, add it directly so the `chessboard` target exists.
if(NOT TARGET chessboard)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../chessBoard
                     ${CMAKE_CURRENT_BINARY_DIR}/chessBoard)
endif()

# Include Directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add Source Files
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# # Find SDL2 and SDL2_image
# find_package(SDL2 REQUIRED)
# find_package(SDL2_image REQUIRED)

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED SDL2_image>=2.0.0)

# Include SDL2 Directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

# SDL2 may live outside the default linker search path (e.g. Homebrew under
# /opt/homebrew/lib); add the library directories pkg-config reports.
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})

# Add Executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Link SDL2, SDL2_image, and the chess engine core.
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} chessboard)

message(STATUS "SDL2 include directories: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2 libraries: ${SDL2_LIBRARIES}")
message(STATUS "SDL2_image include directories: ${SDL2_IMAGE_INCLUDE_DIRS}")
message(STATUS "SDL2_image libraries: ${SDL2_IMAGE_LIBRARIES}")

