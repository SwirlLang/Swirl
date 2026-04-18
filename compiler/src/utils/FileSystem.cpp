#include <cassert>
#include <ostream>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "utils/FileSystem.h"


sw::FileHandle::FileHandle(const std::string_view file_path): m_Path(file_path) {}

const std::filesystem::path& sw::FileHandle::getPath() {
    return m_Path;
}

std::string_view sw::FileHandle::readAll() {
    if (m_FileContent) {
        return *m_FileContent;
    }

    std::ifstream      f_stream(m_Path);
    std::ostringstream s_stream;

    s_stream << f_stream.rdbuf();
    m_FileContent = s_stream.str();
    return *m_FileContent;
}

std::unique_ptr<std::istream> sw::FileHandle::getStream() {
    if (m_FileContent.has_value()) {
        return std::make_unique<std::istringstream>(*m_FileContent);
    }

    auto file_stream = std::make_unique<std::ifstream>(m_Path);
    if (!file_stream->is_open()) {
        throw std::runtime_error("sw::FileHandle::getStream(): could not open file");
    } return file_stream;
}

sw::FileHandle* sw::FileSystem::open(const std::filesystem::path& file_path) {
    if (const auto handle = m_FileTable.find(file_path); handle != m_FileTable.end()) {
        return handle->second.get();
    }

    const auto handle = new FileHandle(file_path.string());
    m_FileTable[file_path] = std::unique_ptr<FileHandle>(handle);

    assert(handle != nullptr);
    return handle;
}

sw::FileHandle* sw::FileSystem::open(const std::string_view file_path) {
    return open(std::filesystem::path(file_path));
}


sw::FileHandle* sw::FileSystem::createVirtualFile(const std::string_view file_path, std::string_view content) {
    const auto handle = m_FileTable.find(file_path);

    if (handle == m_FileTable.end()) {
        const auto new_handle = new FileHandle(file_path);
        new_handle->m_FileContent = content;
        m_FileTable[std::string(file_path)] = std::unique_ptr<FileHandle>(new_handle);

        assert(new_handle != nullptr);
        return new_handle;
    }

    return handle->second.get();
}

sw::FileHandle* sw::FileSystem::fetchHandleFor(const std::string& file_path) {
    if (const auto handle = m_FileTable.find(file_path); handle != m_FileTable.end()) {
        return handle->second.get();
    }

    throw std::runtime_error(std::format("sw::FileSystem::fetchHandleFor: file {} not found", file_path));
}
