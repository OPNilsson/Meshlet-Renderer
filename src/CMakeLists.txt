cmake_minimum_required(VERSION 3.19)

set(vcpkg "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${vcpkg}")
  set(CMAKE_TOOLCHAIN_FILE "${vcpkg}"
      CACHE FILEPATH "CMake toolchain file")
  message(STATUS "vcpkg toolchain found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SUPPRESS_REGENERATION true)

project(Jinsoku VERSION 1.0)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(Lua REQUIRED)
find_package(imgui CONFIG REQUIRED)


set(METIS_DIR extern/METIS)
aux_source_directory(${METIS_DIR} METIS_SRC_LIST)

aux_source_directory(${PROJECT_SOURCE_DIR}/extern/GEL/CGLA CGLA_SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/extern/GEL/Geometry GEO_SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/extern/GEL/HMesh MSH_SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/extern/GEL/Util UTL_SRC_LIST)

include_directories(/extern/GEL/GL/include)
aux_source_directory(/extern/GEL/GL GL_SRC_LIST)
aux_source_directory(/extern/GEL/GLGraphics GLG_SRC_LIST)
add_library(GEL STATIC ${CGLA_SRC_LIST} ${GEO_SRC_LIST} ${MSH_SRC_LIST} ${UTL_SRC_LIST} ${GL_SRC_LIST} ${GLG_SRC_LIST})

find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")

include_directories(
    "D:/Documents/SDK/VCPKG/installed/x64-windows/include"
	${Vulkan_INCLUDE_DIRS}
	${glm_INCLUDE_DIRS} 
	${glfw_INCLUDE_DIRS} 
	${STB_INCLUDE_DIRS}
	${tinyobjloader_INCLUDE_DIRS}
	${LUA_INCLUDE_DIR}
    ./extern
	)

    link_directories(
        "D:/Documents/SDK/VCPKG/installed/x64-windows/lib"
    )

INSTALL (TARGETS GEL)

# Install Header Files
install(DIRECTORY ${PROJECT_SOURCE_DIR}/extern/GEL
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
    )

set(HEADERS
    jinsoku.h
    presenter.h
    inputHandler.h
    )

set(SOURCES
    main.cpp
    presenter.cpp
    inputHandler.cpp
    )

# change to individual files
file(GLOB VK_SOURCE_FILES ${PROJECT_SOURCE_DIR}/jsvk/*.cpp 
						${PROJECT_SOURCE_DIR}/jsvk/*.hpp 
						${PROJECT_SOURCE_DIR}/jsvk/*.h)

set(EXE_NAME Jinsoku)

# Make the library.
#add_executable(${EXE_NAME} ${SOURCES} ${HEADERS} ${cmake_options})
add_executable(${EXE_NAME} ${SOURCES} ${HEADERS} ${VK_SOURCE_FILES} ${VR_SOURCE_FILES} ${cmake_options})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EXE_NAME})

# Requires glfw3 lib for target platform
target_link_libraries(
	${EXE_NAME}
	PRIVATE
    	${Vulkan_LIBRARY}
	glm::glm
	glfw
	tinyobjloader::tinyobjloader
	${LUA_LIBRARIES}
	imgui::imgui
    GEL
    )

set_target_properties(
    ${EXE_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${PROJECT_SOURCE_DIR}/benchmarks)
        #$<TARGET_FILE_DIR:${EXE_NAME}>/benchmarks)

add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/jsvk/shaders"
        $<TARGET_FILE_DIR:${EXE_NAME}>/jsvk/shaders)