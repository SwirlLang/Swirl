#pragma once
#include <string>
#include <memory>
#include <optional>
#include <filesystem>
#include <unordered_map>

namespace sw {
class FileHandle {
public:
    explicit FileHandle(std::string_view file_path);

    std::string_view readAll();
    const std::filesystem::path& getPath();

private:
    std::filesystem::path m_Path;
    std::optional<std::string> m_FileContent;

    friend class FileSystem;
};


class FileSystem {
public:
    FileHandle* open(std::string_view file_path);
    FileHandle* open(const std::filesystem::path& file_path);

    FileHandle* createVirtualFile(std::string_view file_path, std::string_view content);
    FileHandle* fetchHandleFor(const std::string& file_path);

private:
    std::unordered_map<std::filesystem::path, std::unique_ptr<FileHandle>> m_FileTable;
};
}