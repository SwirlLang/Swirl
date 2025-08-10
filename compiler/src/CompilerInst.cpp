#include "CompilerInst.h"

#include "backend/LLVMBackend.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>


void CompilerInst::startLLVMCodegen() {
    Backends_t llvm_backends;
    llvm_backends.reserve(m_ModuleManager.size());

    std::vector<std::shared_future<void>> backend_futures;
    backend_futures.reserve(m_ModuleManager.size());

    for (Parser* parser : m_ModuleManager) {
        auto* backend = llvm_backends.emplace_back(new LLVMBackend{*parser}).get();
        // backend->startGeneration();
         backend_futures.emplace_back(m_ThreadPool.async([backend] {
            backend->startGeneration();
        }));
    } m_ThreadPool.wait();

    assert(llvm_backends.size() == backend_futures.size());
    for (const auto& [i, backend] : llvm::enumerate(llvm_backends)) {
        backend_futures.at(i).get();
        backend->printIR();
    }

    generateObjectFiles(llvm_backends);
}


void CompilerInst::generateObjectFiles(Backends_t& backends) {
    // create the `.build` directory hierarchy if it doesn't exist
    const auto build_dir = m_SrcPath.parent_path() / ".build";
    if (!exists(build_dir)) {
        create_directory(build_dir);
        create_directory(build_dir / "obj");
    }

    // clear all previous object files
    for (const auto& entry : fs::directory_iterator(build_dir / "obj")) {
        fs::remove(entry);
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
    } produceExecutable();
}


void CompilerInst::produceExecutable() {
    const auto triple = llvm::Triple(TargetTriple);
    const auto build_dir = m_SrcPath.parent_path() / ".build";

    if (m_OutputPath.empty())
        m_OutputPath = m_SrcPath.parent_path() / ".build" / (m_SrcPath
            .filename()
            .replace_extension()
            .string() + ".out"
            );

    std::string object_files_args;  // accumulates the absolute object-files' paths
    for (const auto& file : fs::directory_iterator(build_dir / "obj")) {
        object_files_args.append(" " + file.path().string());
    }

    std::string command = std::format("cc -o {} {}", m_OutputPath.string(), object_files_args);
    if (triple.isOSWindows()) {
        if (triple.getEnvironment() == llvm::Triple::MSVC) {
            // handle the peculiar case of MSVC
            command = std::format(
                "cl.exe /Fe:{} {}",
                m_OutputPath.string(),
                object_files_args
            );
        }
    }

    llvm::outs() << "Issuing command: " << command << "\n";
    std::system(command.c_str());
}
