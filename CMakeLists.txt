cmake_minimum_required(VERSION 3.12)


# This is your project statement. You should always list languages;
# Listing the version is nice here since it sets lots of useful variables
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)

set (CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)
# set (CMAKE_CXX_STANDARD 20)

include_directories(include thirdparty/imgui)

# Adding something we can run - Output name matches target name
add_executable(MyExample
                    # IMGUI
                    thirdparty/imgui/imgui.cpp
                    thirdparty/imgui/imgui_widgets.cpp
                    thirdparty/imgui/imgui_tables.cpp
                    thirdparty/imgui/imgui_draw.cpp
                    thirdparty/imgui/imgui_demo.cpp
                    thirdparty/imgui/backends/imgui_impl_sdl3.cpp
                    thirdparty/imgui/backends/imgui_impl_vulkan.cpp

                    source/video/Renderer.cpp
                    source/video/VmaUsage.cpp
                    source/video/Buffer.cpp
                    source/main.cpp)

include(FetchContent)

# define a function for adding git dependencies
function(include_dependency libName gitURL gitTag)
    # setup the declare
    FetchContent_Declare(${libName}
            GIT_REPOSITORY ${gitURL}
            GIT_TAG        ${gitTag}
            GIT_SHALLOW    TRUE
            GIT_PROGRESS   TRUE
    )

    FetchContent_MakeAvailable(${libName})
endfunction()

# add SDL3 support
find_package(SDL3 QUIET)
if (NOT SDL3_FOUND)
    message(STATUS "Getting SDL3 from Github")
    include_dependency(SDL3 https://github.com/libsdl-org/SDL.git release-3.2.0)
else()
    message(STATUS "Using local SDL3")
endif()

include_dependency(fetch_vk_bootstrap https://github.com/charles-lunarg/vk-bootstrap ffa964552f8bdf2c41e8e9a81a80637c39dd7afc)

include_dependency(fetch_vma https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator 1d8f600fd424278486eade7ed3e877c99f0846b1)

find_package(Vulkan REQUIRED)

target_link_libraries(MyExample vk-bootstrap::vk-bootstrap SDL3::SDL3 Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)


