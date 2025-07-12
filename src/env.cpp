namespace dz {
    void set_env(const std::string& key, const std::string& value)
    {
    #if defined(_WIN32)
        std::string full = key + "=" + value;
        if (_putenv(full.c_str()) != 0)
        {
            throw std::runtime_error("Failed to set environment variable on Windows: " + key);
        }
    #elif defined(__linux__) || defined(__APPLE__)
        if (setenv(key.c_str(), value.c_str(), 1) != 0)
        {
            throw std::runtime_error("Failed to set environment variable on POSIX system: " + key);
        }
    #else
        #error "Unsupported platform for set_env"
    #endif
    }

    std::string get_env(const std::string& key)
    {
    #if defined(_WIN32)
        size_t requiredSize = 0;
        getenv_s(&requiredSize, nullptr, 0, key.c_str());
        if (requiredSize == 0)
        {
            return "";
        }
        std::string value(requiredSize, '\0');
        getenv_s(&requiredSize, &value[0], requiredSize, key.c_str());
        if (!value.empty() && value.back() == '\0')
        {
            value.pop_back();
        }
        return value;
    #elif defined(__linux__) || defined(__APPLE__)
        const char* val = std::getenv(key.c_str());
        return val ? std::string(val) : "";
    #else
        #error "Unsupported platform for get_env"
    #endif
    }

    void append_vk_icd_filename(const std::string& swiftshader_icd_path)
    {
        std::string key = "VK_ICD_FILENAMES";
        std::string current = get_env(key);
        std::string separator =
    #if defined(_WIN32)
            ";";
    #else
            ":";
    #endif

        if (!current.empty())
        {
            if (current.find(swiftshader_icd_path) == std::string::npos)
            {
                current += separator + swiftshader_icd_path;
            }
        }
        else
        {
            current = swiftshader_icd_path;
        }

        set_env(key, current);
    }
}