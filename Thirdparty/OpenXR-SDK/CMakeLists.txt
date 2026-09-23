add_library(openxr_headers INTERFACE)
target_include_directories(openxr_headers INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src/common
)
