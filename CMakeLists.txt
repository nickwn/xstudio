cmake_minimum_required (VERSION 3.0)
project (xstudio)

set(IMGUI_DIR "deps/imgui")
set(SPV_REFLECT_DIR "deps/SPIRV-Reflect")
set(SPV_TOOLS_DIR "deps/SPIRV-Tools")
set(SPV_DIR "deps/glslang/")
set(VMA_DIR "deps/VulkanMemoryAllocator/src")

find_package(Eigen3 CONFIG REQUIRED)

file(GLOB_RECURSE XSTUDIO_CPP "src/*.cpp")
file(GLOB_RECURSE XSTUDIO_C "src/*.c")
set(SPV_REFLECT_CPP "${SPV_REFLECT_DIR}/spirv_reflect.c")

list(APPEND XS_COMPILE_DEFINITIONS UNICODE _UNICODE EIGEN_HAS_STD_INVOKE_RESULT) 
if (WIN32)
	list(APPEND XS_COMPILE_DEFINITIONS WINDOWS NOMINMAX VK_USE_PLATFORM_WIN32_KHR _CONSOLE YY_NO_UNISTD_H)
endif(WIN32)

message("$ENV{VULKAN_SDK}")
add_executable(xstudio WIN32 ${XSTUDIO_CPP} ${XSTUDIO_C} ${SPV_REFLECT_CPP}) 
target_compile_definitions(xstudio PUBLIC ${XS_COMPILE_DEFINITIONS})
target_include_directories(xstudio PUBLIC "$ENV{VULKAN_SDK}/include" ${VMA_DIR} ${SPV_REFLECT_DIR} ${SPV_DIR})
target_link_directories(xstudio PUBLIC "$ENV{VULKAN_SDK}/Lib")
target_link_libraries(xstudio PRIVATE vulkan-1 SPIRV-Toolsd SPIRV-Tools-optd SPIRV-Tools-sharedd PUBLIC Eigen3::Eigen) 
set_property(TARGET xstudio PROPERTY CXX_STANDARD 20) 
