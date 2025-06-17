std::filesystem::path getUserDirectoryPath() { return std::filesystem::path(getenv("HOME")); }
std::filesystem::path getProgramDirectoryPath()
{
    std::filesystem::path exePath;
#if defined(_WIN32)
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    exePath = path;
#elif defined(MACOS)
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
        exePath = path;
#elif defined(__linux__)
    exePath = std::filesystem::canonical("/proc/self/exe");
#endif
    return exePath.parent_path();
}
std::filesystem::path getProgramDataPath() { return std::filesystem::temp_directory_path(); }
std::filesystem::path getExecutableName() { return std::filesystem::path(getenv("_")).filename(); }