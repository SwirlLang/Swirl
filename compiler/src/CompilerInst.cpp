#include "CompilerInst.h"

#include "backend/LLVMBackend.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <lld/Common/Driver.h>

#ifdef __linux__

LLD_HAS_DRIVER(elf)
#define SW_LLD_DRIVER_NAMESPACE elf
#define SW_LLD_DRIVER_NAME  "ld.lld"

#elif _WIN32

LLD_HAS_DRIVER(coff)
#define SW_LLD_DRIVER_NAMESPACE coff
#define SW_LLD_DRIVER_NAME  "lld-link"

#endif


void CompilerInst::addPackageEntry(const std::string_view package, const bool is_project) {
    // when true, do not try to parse the author-name and package-version from the path
    if (is_project) {
        const auto alias = package.substr(package.find_last_of(':') + 1);
        const auto path  = package.substr(0, package.size() - alias.size() - 1);

        assert(!PackageTable.contains(std::string(alias)));
        PackageTable[std::string(alias)] = PackageInfo{.package_root = fs::path(path)};
        return;
    }

    const auto alias = package.substr(package.find_last_of(':') + 1);
    const auto path  = package.substr(0, package.size() - alias.size() - 1);

    assert(!PackageTable.contains(std::string(alias)));
    PackageTable[std::string(alias)] = PackageInfo{.package_root = fs::path(path)};
}


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

    std::string runtime_filename{"runtime_"};

    switch (triple.getOS()) {
        case llvm::Triple::Linux:
            runtime_filename += "linux_";
            break;
        case llvm::Triple::Darwin:
            runtime_filename += "darwin_";
            break;
        case llvm::Triple::Win32:
            runtime_filename += "windows_";
            break;
        default: throw std::runtime_error(
            "CompilerInst::produceExecutable: Unknown platform!");
    }

    switch (triple.getArch()) {
        case llvm::Triple::x86:
            runtime_filename += "x86";
            break;
        case llvm::Triple::x86_64:
            runtime_filename += "x64";
            break;
        case llvm::Triple::aarch64:
            runtime_filename += "aarch64";
            break;
        default: throw std::runtime_error("Unknown architecture!");
    }

    // add the extension, `.o` for linux and mac, `.obj` for windows
    runtime_filename += triple.getOS() == llvm::Triple::Win32 ? ".obj" : ".o";

    // the relative path to the swirl runtime
    const auto runtime_path = fs::path("..") / "lib" / runtime_filename;

    // a DriverDef is composed of a "flavor" and a `link` "callback"
    lld::DriverDef platform_driver = {
        triple.getOS() == llvm::Triple::Linux
            ? lld::Gnu : (
                triple.getOS() == llvm::Triple::Win32
                ? lld::WinLink
                : lld::Darwin)
        , &lld::SW_LLD_DRIVER_NAMESPACE::link
    };

    // accumulate the runtime files
    std::vector<std::string> sw_runtime{3};
    sw_runtime.push_back(runtime_path.string());
    if (triple.getOS() == llvm::Triple::Win32) {
        sw_runtime.emplace_back("kernel32.lib");
        sw_runtime.emplace_back("shell32.lib");
        sw_runtime.emplace_back("/subsystem:console");
        sw_runtime.emplace_back("/entry:_start");
        sw_runtime.emplace_back("/STACK:4194304");
        
    }

    // accumulate the linker arguments
    std::vector args{SW_LLD_DRIVER_NAME};

    // push all the runtime files
    for (auto& arg : sw_runtime) {
        args.push_back(arg.c_str());
    }

    // iterate over all object files of the build directory and push their paths to the vector
    for (const auto& file : fs::directory_iterator(build_dir / "obj")) {
        args.push_back((new std::string(file.path().string()))->c_str());
    } args.push_back(triple.getOS() == llvm::Triple::Win32 ? "/OUT:" : "-o");

    // compute the output path
    if (m_OutputPath.empty())
        m_OutputPath = m_SrcPath.parent_path() / ".build" / (m_SrcPath
            .filename()
            .replace_extension()
            .string() + (triple.getOS() == llvm::Triple::Win32 ? ".exe" : ".out")
            );

    // push the output path to the vector
    args.push_back(m_OutputPath.string().c_str());

    // do the final ritual
    lld::lldMain(args, llvm::outs(), llvm::errs(), {platform_driver});
}
