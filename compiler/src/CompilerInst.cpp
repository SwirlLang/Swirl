#include "CompilerInst.h"
#include "backend/LLVMBackend.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>


void CompilerInst::startLLVMCodegen() {
    Backends_t m_Backends;
    m_Backends.reserve(m_ModuleManager.size() + 1);
    m_Backends.emplace_back(new LLVMBackend(*m_MainModParser));

    for (auto& parser: m_ModuleManager | std::views::values) {
        m_Backends.emplace_back(new LLVMBackend{*parser});
        m_Backends.back()->startGeneration();
        m_Backends.back()->printIR();
        // m_ThreadPool.enqueue([&m_Backends, this] { m_Backends.back()->startGeneration(); });
    } // m_ThreadPool.wait();

    generateObjectFiles(m_Backends);
}


void CompilerInst::generateObjectFiles(Backends_t& backends) const {
    // create the `.build` directory hierarchy if it doesn't exist
    auto build_dir = m_SrcPath.parent_path() / ".build";
    if (!exists(build_dir)) {
        create_directory(build_dir);
        create_directory(build_dir / "obj");
    }


    for (const auto& [counter, backend] : llvm::enumerate(backends)) {
        llvm::legacy::PassManager pass_man;
        std::error_code ec;
        llvm::raw_fd_ostream dest(
            (build_dir / "obj" / ("output" + std::to_string(counter))).string(),
            ec,
            llvm::sys::fs::OpenFlags::OF_None
            );

        if (ec) {
            throw std::runtime_error("llvm::raw_fd_ostream failed! " + ec.message());
        }

        if (LLVMBackend::TargetMachine->addPassesToEmitFile(
            pass_man, dest, nullptr, llvm::CodeGenFileType::ObjectFile)
            ) {
            throw std::runtime_error("Target machine can't emit object file!");
            }

        pass_man.run(*backend->getLLVMModule());
        dest.flush();
    }
}


void CompilerInst::produceExecutable() {}
