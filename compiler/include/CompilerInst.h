#pragma once
#include <parser/Parser.h>
#include <backend/LLVMBackend.h>


namespace fs = std::filesystem;
extern ModuleMap_t ModuleMap;


class CompilerInst {
    Parser        m_MainModParser;   // the parser of the module from where compilation has been initiated
    fs::path      m_OutputPath;     // path/to/executable (absolute)
    unsigned      m_BaseThreadCount = std::thread::hardware_concurrency() / 2;
    ThreadPool_t  m_ThreadPool;


public:
    explicit CompilerInst(const fs::path& path): m_MainModParser(path) {}

    void setBaseThreadCount(const unsigned count)     { m_BaseThreadCount = count; }
    void setBaseThreadCount(const std::string& count) { m_BaseThreadCount = std::stoi(count); }

    void compile() {
        m_MainModParser.parse();

        int batch_no = 1;
        while (!ModuleMap.zeroVecIsEmpty()) {
            std::println("Batch-{}: ", batch_no++);

            while (auto mod = ModuleMap.popZeroDepVec()) {
                mod->performSema();
                // std::print("{}, ", mod->m_FilePath.string());
                // m_ThreadPool.enqueue([mod] {
                //     mod->performSema();
                // });
            }

            std::println("\n-------------");
            ModuleMap.swapBuffers();
            // m_ThreadPool.wait();
        }

        m_MainModParser.performSema();

        LLVMBackend llvm_bk{m_MainModParser};
        llvm_bk.startGeneration();
        llvm_bk.printIR();
    }

    ~CompilerInst() { m_ThreadPool.shutdown(); }
};

