#pragma once
#include <print>
#include <utility>
#include <filesystem>
#include <unordered_set>
#include <llvm/Support/ThreadPool.h>

#include "parser/Parser.h"
#include "backend/LLVMBackend.h"
#include "managers/ModuleManager.h"


namespace fs = std::filesystem;
using ThreadPool = llvm::StdThreadPool;

class CompilerInst {
    ThreadPool    m_ThreadPool;
    SourceManager m_SourceManager;
    ErrorManager  m_ErrorManager;
    ModuleManager m_ModuleManager;

    fs::path      m_SrcPath;
    fs::path      m_OutputPath;     // path/to/executable (absolute)
    unsigned      m_BaseThreadCount = std::thread::hardware_concurrency() / 2;

    std::optional<Parser> m_MainModParser = std::nullopt;
    ErrorCallback_t       m_ErrorCallback = nullptr;

    using Backends_t = std::vector<std::unique_ptr<LLVMBackend>>;

    /// creates an LLVMBackend instance for all parsers and initiates LLVM Module creation
    void startLLVMCodegen();

    /// links all object files and dependencies together to produce the final binary
    void produceExecutable();

    /// generates object files for all the modules
    void generateObjectFiles(Backends_t&);


public:
    inline static std::string TargetTriple;
    inline static std::unordered_set<std::string> LinkTargets;

    explicit CompilerInst(fs::path path)
        : m_SourceManager(path)
        , m_SrcPath(std::move(path)) { }


    void setBaseThreadCount(const std::string& count) {
        m_BaseThreadCount = static_cast<unsigned>(std::stoi(count));
    }


    void compile() {
        if (TargetTriple.empty()) {
            TargetTriple = llvm::sys::getDefaultTargetTriple();
        }

        if (m_ErrorCallback == nullptr) {
            m_ErrorCallback =
                [this](const ErrCode code, const ErrorContext& ctx) { m_ErrorManager.newErrorLocked(code, ctx); };
        }

        m_MainModParser.emplace(m_SrcPath, m_ErrorCallback, m_ModuleManager);
        m_MainModParser->toggleIsMainModule();
        m_ModuleManager.setMainModParser(&(*m_MainModParser));

        m_MainModParser->parse();

        // check for parser errors and abort if present
        if (m_ErrorManager.errorOccurred()) {
            m_ErrorManager.flush();
            std::exit(1);
        }

        // || --- *---*   Sema   *---* --- || //
        int batch_no = 1;
        while (!m_ModuleManager.zeroVecIsEmpty()) {
            std::println("Batch-{}: ", batch_no++);

            while (const auto mod = m_ModuleManager.popZeroDepVec()) {
                std::print("{}, ", mod->m_FilePath.string());
                m_ThreadPool.async([mod] {
                    mod->performSema();
                });
            }

            std::println("\n-------------");
            m_ModuleManager.swapBuffers();
            m_ThreadPool.wait();
        } m_ThreadPool.wait();

        // *---* - *---*  *---* - *---*  *---* - *---* //
        // check for Sema errors and abort if present
        if (m_ErrorManager.errorOccurred()) {
            m_ErrorManager.flush();
            std::exit(1);
        }

        startLLVMCodegen();
    }

    static void setTargetTriple(const std::string& triple) { TargetTriple = triple; }
    static std::string_view getTargetTriple() { return TargetTriple; }
    static void appendLinkTarget(std::string_view target) { LinkTargets.emplace(target); }

    // ~CompilerInst() { m_ThreadPool.shutdown(); }
};

