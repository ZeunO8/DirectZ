std::filesystem::path getProgramDirectoryPath()
{
    std::filesystem::path exePath;
#if defined(_WIN32)
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
        exePath = path;
#endif
    return exePath.parent_path();
}