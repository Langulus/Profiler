cmake_minimum_required(VERSION 3.28)
project(LangulusProfiler
    VERSION         1.0.0
    DESCRIPTION     "Langulus profiler utility"
    HOMEPAGE_URL    https://langulus.com
)

# Check if this project is built as standalone, or a part of something else 
if (PROJECT_IS_TOP_LEVEL OR NOT LANGULUS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    include(LangulusUtilities.cmake)
	fetch_langulus_module(Logger GIT_TAG 2fa3c59f2ef5f9888954a5c3154201d134d6fc07)
endif()

# Build and install Tester library											
add_langulus_library(LangulusProfiler
	    $<TARGET_OBJECTS:LangulusLogger>
		source/Profiler.cpp
)

target_compile_definitions(LangulusProfiler
    PRIVATE     LANGULUS_EXPORT_ALL
)

target_include_directories(LangulusProfiler
    PUBLIC      $<TARGET_PROPERTY:LangulusLogger,INTERFACE_INCLUDE_DIRECTORIES>
				include
)

target_link_libraries(LangulusProfiler
    PUBLIC      LangulusCore
				fmt
)