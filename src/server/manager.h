#pragma once

#include <string>
#include <filesystem>

#include "file.pb.h"

namespace fs_std = std::filesystem;


class fileManager
{
public:

    explicit fileManager(const std::string& root);

    void list(const std::string& path, fs::Response& response);

    void create(const std::string& path, fs::Response& response);

    void remove(const std::string& path, fs::Response& response);

private:

    bool resolvePath(const std::string& relativePath, fs_std::path& outPath);

    fs_std::path root_;
};