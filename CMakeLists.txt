cmake_minimum_required (VERSION 3.0)
project (xstudio)

# ImGui dir for compiling with project
set(IMGUI_DIR "deps/imgui")
set(SPV_REFLECT_DIR "deps/SPIRV-Reflect")

find_package(LLVM REQUIRED CONFIG)
find_package(Vulkan REQUIRED)
find_package(Eigen3 REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(llvm_libs support core irreader)

file(GLOB_RECURSE XSTUDIO_HPP "src/*.hpp")
file(GLOB_RECURSE XSTUDIO_CPP "src/*.cpp")
file(GLOB IMGUI_CPP "${IMGUI_DIR}/**.cpp")
file(GLOB SPV_REFLECT_CPP "${SPV_REFLECT_DIR}/spirv_reflect.c")
file(GLOB SPV_REFLECT_HPP "${SPV_REFLECT_DIR}/spirv_reflect.h")

list(APPEND XS_COMPILE_DEFINITIONS UNICODE _UNICODE EIGEN_HAS_STD_INVOKE_RESULT)
if (WIN32)
	list(APPEND XS_COMPILE_DEFINITIONS WINDOWS NOMINMAX VK_USE_PLATFORM_WIN32_KHR _CONSOLE)
endif(WIN32)

add_executable(xstudio WIN32 ${XSTUDIO_CPP} ${SPV_REFLECT_CPP}) #${IMGUI_CPP})
target_compile_definitions(xstudio PUBLIC ${XS_COMPILE_DEFINITIONS})
target_include_directories(xstudio PUBLIC ${IMGUI_DIR} ${SPV_REFLECT_DIR})
target_link_libraries(xstudio PRIVATE ${llvm_libs} Vulkan::Vulkan PUBLIC Eigen3::Eigen)
set_property(TARGET xstudio PROPERTY CXX_STANDARD 20)
