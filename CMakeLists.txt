cmake_minimum_required (VERSION 3.8)

project ("stemlib")

add_library (
	${PROJECT_NAME}
	STATIC
	"src/stemlib.cpp"
	"src/stemlib.hpp"
)

set_property (TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

set (BUILD_SHARED_LIBS OFF)
add_subdirectory ("external/poco")

target_link_libraries (${PROJECT_NAME} Poco::Net)

add_executable (
	stemchat
	"stemchat.cpp"
)

target_link_libraries (
	stemchat
	${PROJECT_NAME}
)