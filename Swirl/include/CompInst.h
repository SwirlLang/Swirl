#pragma once
#include <filesystem>
#include <fstream>
#include <parser/Parser.h>

/// Represents a compiler instance for a single translation unit
// class CompInst {
//     std::string m_Source;
//     Parser m_Parser;
//
// public:
//     explicit CompInst(const std::filesystem::path& sw_file) {
//         std::ifstream file;
//         if (!file.is_open())
//             throw std::runtime_error("CompInst::CompInst: file is not open");
//
//         file.close();
//     }
//
// };
