#include "CompilerInst.h"
#include "backend/LLVMBackend.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>


void CompilerInst::startLLVMCodegen() {
    Backends_t llvm_backends;
    llvm_backends.reserve(m_ModuleManager.size() + 1);

    for (auto& parser: m_ModuleManager | std::views::values) {
        auto* backend = llvm_backends.emplace_back(new LLVMBackend{*parser}).get();
        m_ThreadPool.async([backend, this] { backend->startGeneration(); });
    }

    llvm_backends.emplace_back(new LLVMBackend(m_MainModParser.value()));
    m_ThreadPool.wait();

    llvm_backends.back()->startGeneration();
    for (const auto& backend: llvm_backends) {
        backend->printIR();
    }

    generateObjectFiles(llvm_backends);
}


void CompilerInst::generateObjectFiles(Backends_t& backends) const {
    // create the `.build` directory hierarchy if it doesn't exist
    const auto build_dir = m_SrcPath.parent_path() / ".build";
    if (!exists(build_dir)) {
        create_directory(build_dir);
        create_directory(build_dir / "obj");
    }


    for (const auto& [counter, backend] : llvm::enumerate(backends)) {
        llvm::legacy::PassManager pass_man;
        std::error_code ec;
        llvm::raw_fd_ostream dest(
            (build_dir / "obj" / ("output_" + std::to_string(counter))).string(),
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


void CompilerInst::produceExecutable() {
    auto triple = llvm::Triple(TargetTriple);
}
