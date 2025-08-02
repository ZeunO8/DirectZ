

namespace dz {
    std::filesystem::path getUserDirectoryPath() { return std::filesystem::path(getenv("HOME")); }
#if !defined(MACOS) && !defined(IOS)
    std::filesystem::path getProgramDirectoryPath()
    {
        std::filesystem::path exePath;
#if defined(_WIN32)
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        exePath = path;
#elif defined(__linux__)
        exePath = std::filesystem::canonical("/proc/self/exe");
#endif
        return exePath.parent_path();
    }
#else
    std::filesystem::path getProgramDirectoryPath();
#endif
    std::filesystem::path getProgramDataPath() { return std::filesystem::temp_directory_path(); }
    std::filesystem::path getExecutableName() { return std::filesystem::path(getenv("_")).filename(); }
}