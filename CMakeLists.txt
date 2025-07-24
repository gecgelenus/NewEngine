cmake_minimum_required(VERSION 3.16)

project(Engine CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(PROPERTIES FOLDER "include")
set(PROJECT_ROOT ".") # This refers to the top-level source directory where CMakeLists.txt resides.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SHADER_DIR "${PROJECT_ROOT}/shaders/src") # Source directory for raw shaders

# Define the *output* directory for compiled shaders within the build tree.
# CMAKE_BINARY_DIR is the root of your build directory (e.g., C:/MyProjects/Engine/build)
set(SHADER_COMPILED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/../shaders/bin")

# Create the output directory for compiled shaders
file(MAKE_DIRECTORY "${SHADER_COMPILED_OUTPUT_DIR}")


find_program(GLSLANG_VALIDATOR glslangValidator
    HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
    DOC "Path to glslangValidator executable from Vulkan SDK"
)

if (NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found! Please ensure Vulkan SDK is installed and its 'bin' directory is in your PATH or VULKAN_SDK environment variable is set.")
endif()

message(STATUS "Found glslangValidator: ${GLSLANG_VALIDATOR}")

file(GLOB_RECURSE SHADER_SOURCES
    "${SHADER_DIR}/*.vert"
    "${SHADER_DIR}/*.frag"
    "${SHADER_DIR}/*.comp"
)

# Loop through each shader source file and add a custom command to compile it
set(COMPILED_SHADERS "")
foreach(SHADER_FILE ${SHADER_SOURCES})
    # Get the base name (e.g., 'shader.vert' -> 'shader.vert')
    get_filename_component(SHADER_BASENAME ${SHADER_FILE} NAME)

    # Determine the output SPIR-V file name (e.g., 'shader.vert.spv')
    # Use SHADER_COMPILED_OUTPUT_DIR for the output
    set(OUTPUT_SPV_FILE "${SHADER_COMPILED_OUTPUT_DIR}/${SHADER_BASENAME}.spv")

    # Add a custom command for compilation
    add_custom_command(
        OUTPUT "${OUTPUT_SPV_FILE}"
        COMMAND "${GLSLANG_VALIDATOR}"
            -V # Output SPIR-V
            -o "${OUTPUT_SPV_FILE}" # Output file
            "${SHADER_FILE}" # Input shader file
        DEPENDS "${SHADER_FILE}" # If shader file changes, recompile
        COMMENT "Compiling shader: ${SHADER_FILE}"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS "${OUTPUT_SPV_FILE}")
endforeach()

# Add a custom target that depends on all compiled shaders
# This ensures that all shaders are compiled before the main executable is built.
add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})


file(GLOB_RECURSE SOURCES
    "${PROJECT_ROOT}/src/*.cpp"
    "${PROJECT_ROOT}/src/imgui/*.cpp" # imgui source files
)

find_package(Vulkan REQUIRED)

add_subdirectory(${PROJECT_ROOT}/deps/glfw-3.4)
add_subdirectory(${PROJECT_ROOT}/deps/glm)

include_directories(
    ${Vulkan_INCLUDE_DIRS}
    ${PROJECT_ROOT}/include
    ${PROJECT_ROOT}/deps/colorLog/
    ${PROJECT_ROOT}/deps/resourceLoaders/
    ${PROJECT_ROOT}/include/imgui
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE _LOG_ALL)

target_link_libraries(${PROJECT_NAME}
    ${Vulkan_LIBRARIES}
    glfw
)

# If your executable needs to know where the compiled shaders are at runtime,
# you might want to add this directory to its RPATH or provide it as a build variable.
# For example, to make the directory available in your C++ code:
# target_compile_definitions(${PROJECT_NAME} PRIVATE SHADER_BINARY_PATH="${SHADER_COMPILED_OUTPUT_DIR}")