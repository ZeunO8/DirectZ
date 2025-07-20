FetchContent_Declare(iostreams
    GIT_REPOSITORY https://github.com/ZeunO8/iostreams.git
    GIT_TAG main
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(iostreams)
set_target_properties(iostreams PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(archive_static PROPERTIES DEBUG_POSTFIX "d")