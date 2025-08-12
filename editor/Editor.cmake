set(DZ_EDIT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/Entry.cpp
)

add_executable(DirectZEditor ${DZ_EDIT_SOURCES})

target_compile_features(DirectZEditor PRIVATE cxx_std_20)

list(APPEND DirectZ_EXES DirectZEditor)

set_target_properties(DirectZEditor PROPERTIES DEBUG_POSTFIX "d")

target_include_directories(DirectZEditor PRIVATE ${DIRECTZ_INCLUDE_DIRS})
target_include_directories(DirectZEditor PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(DirectZEditor PRIVATE DirectZ ${DIRECTZ_LIBRARIES})