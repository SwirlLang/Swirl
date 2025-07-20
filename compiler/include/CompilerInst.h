#pragma once
#include <print>
#include <parser/Parser.h>
#include <backend/LLVMBackend.h>
#include <managers/ModuleManager.h>

#include <utility>

namespace fs = std::filesystem;


class CompilerInst {
    ThreadPool_t  m_ThreadPool;
    SourceManager m_SourceManager;
    ErrorManager  m_ErrorManager;
    ModuleManager m_ModuleManager;

    fs::path      m_SrcPath;
    fs::path      m_OutputPath;     // path/to/executable (absolute)
    unsigned      m_BaseThreadCount = std::thread::hardware_concurrency() / 2;

    std::optional<Parser> m_MainModParser = std::nullopt;
    ErrorCallback_t       m_ErrorCallback = nullptr;

public:
    inline static std::string TargetTriple;

    explicit CompilerInst(fs::path path)
        : m_SourceManager(path)
        , m_SrcPath(std::move(path)) { }


    void setBaseThreadCount(const std::string& count) { m_BaseThreadCount = std::stoi(count); }
    static void setTargetTriple(const std::string& triple) { TargetTriple = triple; }


    void compile() {
        if (TargetTriple.empty()) {
            TargetTriple = llvm::sys::getDefaultTargetTriple();
        }

        if (m_ErrorCallback == nullptr) {
            m_ErrorCallback =
                [this](const ErrCode code, const ErrorContext& ctx) { m_ErrorManager.newErrorLocked(code, ctx); };
        }

        m_MainModParser.emplace(m_SrcPath, m_ErrorCallback, m_ModuleManager);
        m_MainModParser->parse();

        if (m_ErrorManager.errorOccurred()) {
            m_ErrorManager.flush();
            std::exit(1);
        }

        // || --- *---*   Sema   *---* --- || //
        int batch_no = 1;
        while (!m_ModuleManager.zeroVecIsEmpty()) {
            std::println("Batch-{}: ", batch_no++);

            while (const auto mod = m_ModuleManager.popZeroDepVec()) {
                mod->performSema();
                std::print("{}, ", mod->m_FilePath.string());
                // m_ThreadPool.enqueue([mod] {
                    // mod->performSema();
                // });
            }

            std::println("\n-------------");
            m_ModuleManager.swapBuffers();
            // m_ThreadPool.wait();
        }
        // *---* - *---*  *---* - *---*  *---* - *---* //

        if (m_ErrorManager.errorOccurred()) {
            m_ErrorManager.flush();
            std::exit(1);
        }

        std::vector<std::unique_ptr<LLVMBackend>> m_Backends;
        m_Backends.reserve(m_ModuleManager.size());

        for (auto& parser: m_ModuleManager | std::views::values) {
            m_Backends.emplace_back(new LLVMBackend{*parser});
            m_Backends.back()->startGeneration();
            m_Backends.back()->printIR();
            // m_ThreadPool.enqueue([&m_Backends, this] { m_Backends.back()->startGeneration(); });
        }

        LLVMBackend llvm_bk{*m_MainModParser};
        llvm_bk.startGeneration();
        llvm_bk.printIR();

        // m_ThreadPool.wait();
    }

    std::string_view getTargetTriple() const { return TargetTriple; }

    // ~CompilerInst() { m_ThreadPool.shutdown(); }
};

