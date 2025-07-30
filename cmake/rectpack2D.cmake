message(STATUS "FetchContent: rectpack2D")

FetchContent_Declare(rectpack
  GIT_REPOSITORY https://github.com/TeamHypersomnia/rectpack2D.git
  GIT_TAG master
  GIT_SHALLOW TRUE)
FetchContent_GetProperties(rectpack)
if(NOT rectpack_POPULATED)
  FetchContent_Populate(rectpack)
endif()

message(STATUS "rectpack_SOURCE_DIR: ${rectpack_SOURCE_DIR}")