#include "CompilerInst.h"

#include "backend/LLVMBackend.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <lld/Common/Driver.h>

#ifdef __linux__
LLD_HAS_DRIVER(elf)
    #define SW_LLD_DRIVER_NAMESPACE elf
    #define SW_LLD_DRIVER_NAME "ld.lld"
    #define SW_LLD_FLAVOR lld::Gnu
#elif _WIN32
    #ifdef _MSC_VER
LLD_HAS_DRIVER(coff)
        #define SW_LLD_DRIVER_NAMESPACE coff
        #define SW_LLD_DRIVER_NAME "lld-link"
        #define SW_LLD_FLAVOR lld::WinLink
    #else
LLD_HAS_DRIVER(mingw)
        #define SW_LLD_DRIVER_NAMESPACE mingw
        #define SW_LLD_DRIVER_NAME "ld.lld"
        #define SW_LLD_FLAVOR lld::MinGW
    #endif
#endif

void CompilerInst::addPackageEntry(const std::string_view package, const bool is_project) {
    // when true, do not try to parse the author-name and package-version from the path
    if (is_project) {
        const auto alias = package.substr(package.find_last_of(':') + 1);
        const auto path = package.substr(0, package.size() - alias.size() - 1);

        assert(!PackageTable.contains(std::string(alias)));
        PackageTable[std::string(alias)] = PackageInfo{.package_root = fs::path(path)};
        return;
    }

    const auto alias = package.substr(package.find_last_of(':') + 1);
    const auto path = package.substr(0, package.size() - alias.size() - 1);

    assert(!PackageTable.contains(std::string(alias)));
    PackageTable[std::string(alias)] = PackageInfo{.package_root = fs::path(path)};
}

void CompilerInst::startLLVMCodegen() {
    Backends_t llvm_backends;
    llvm_backends.reserve(m_ModuleManager.size());

    for (Parser* parser : m_ModuleManager) {
        auto* backend = llvm_backends.emplace_back(new LLVMBackend{*parser}).get();
        m_ThreadPool.enqueue([backend] { backend->startGeneration(); });
    }
    m_ThreadPool.wait();

    if (debug_mode) {
        for (const auto& backend : llvm_backends) {
            backend->printIR();
        }
    }

    generateObjectFiles(llvm_backends);
}

void CompilerInst::generateObjectFiles(Backends_t& backends) {
    // create the `.build` directory hierarchy if it doesn't exist
    const auto build_dir = m_SrcPath.parent_path() / ".build";
    if (!exists(build_dir)) {
        create_directory(build_dir);
    }

    if (!exists(build_dir / "obj")) {
        create_directory(build_dir / "obj");
    }

    // clear all previous object files
    for (const auto& entry : fs::directory_iterator(build_dir / "obj")) {
        fs::remove(entry);
    }

    for (const auto& [counter, backend] : llvm::enumerate(backends)) {
        llvm::legacy::PassManager pass_man;
        std::error_code ec;
        llvm::raw_fd_ostream dest((build_dir / "obj" / ("output_" + std::to_string(counter))).string(), ec,
                                  llvm::sys::fs::OpenFlags::OF_None);

        if (ec) {
            throw std::runtime_error("llvm::raw_fd_ostream failed! " + ec.message());
        }

        if (LLVMBackend::TargetMachine->addPassesToEmitFile(pass_man, dest, nullptr,
                                                            llvm::CodeGenFileType::ObjectFile)) {
            throw std::runtime_error("Target machine can't emit object file!");
        }

        pass_man.run(*backend->getLLVMModule());
        dest.flush();
    }
    produceExecutable();
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
    default:
        throw std::runtime_error("CompilerInst::produceExecutable: Unknown platform!");
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
    default:
        throw std::runtime_error("Unknown architecture!");
    }

    // add the extension, `.o` for linux and mac, `.obj` for windows
    runtime_filename += triple.getOS() == llvm::Triple::Win32 ? ".obj" : ".o";

    // path to the swirl runtime stored in the lib folder of parent directory to source file
    const auto runtime_path = (m_SrcPath.parent_path() / "lib" / runtime_filename).string();

    // decide driver flavor based on toolchain
    const bool is_win = triple.getOS() == llvm::Triple::Win32;
    const bool is_linux = triple.getOS() == llvm::Triple::Linux;

    // a DriverDef is composed of a "flavor" and a `link` "callback"
    lld::DriverDef platform_driver = {SW_LLD_FLAVOR, &lld::SW_LLD_DRIVER_NAMESPACE::link};

    // toolchain-specific linker args (kept as strings for lifetime)
    std::vector<std::string> args{SW_LLD_DRIVER_NAME};
    std::vector<std::string> toolchain_args;

    if (is_win) {
        args.push_back(runtime_path);

        // iterate over all object files of the build directory and push their paths to the vector
        for (const auto& file : fs::directory_iterator(build_dir / "obj")) {
            args.push_back(file.path().string());
        }

        args.emplace_back((m_SrcPath.parent_path() / "lib" / "libkernel32.a").string());
        args.emplace_back((m_SrcPath.parent_path() / "lib" / "libshell32.a").string());
        args.push_back("/subsystem:console");
        args.push_back("/entry:_start");
        args.push_back("/STACK:8388607");

    } else if (is_linux) {
        args.push_back("-static");
        args.push_back("-L" + ((m_SrcPath.parent_path() / "lib")).string());
        args.push_back((m_SrcPath.parent_path() / "lib" / "crt1.o").string());
        args.push_back((m_SrcPath.parent_path() / "lib" / "crti.o").string());

        // iterate over all object files of the build directory and push their paths to the vector
        for (const auto& file : fs::directory_iterator(build_dir / "obj")) {
            args.push_back(file.path().string());
        }

        args.push_back("--start-group");
        args.push_back("-lc");
        args.push_back("--end-group");
        args.push_back((m_SrcPath.parent_path() / "lib" / "crtn.o").string());
    }

    // push toolchain-specific args (e.g., -L<mingw>/lib)
    for (auto& a : toolchain_args) {
        args.push_back(a);
    }

    // compute the output path
    if (m_OutputPath.empty()) {
        m_OutputPath = m_SrcPath.parent_path() / ".build" /
                       (m_SrcPath.filename().replace_extension().string() + (is_win ? ".exe" : ""));
    }

// push the output path to the vector
#ifdef _MSC_VER
    // For MSVC, combine /OUT: with the path
    args.push_back(std::format("/OUT:{}", m_OutputPath.string()));
#else
    args.push_back("-o");
    args.push_back(m_OutputPath.string());
#endif

    for (auto& lib : LinkTargets) {
        // Check if this is a full path to a library
        if (lib.starts_with('/')) {
            // If it's a full path, add it directly without -l
            args.push_back(lib);
        } else if (is_win) {
            args.push_back(std::format("{}.lib", lib));
        } else {
            args.push_back(std::format("-l{}", lib));
        }
    }

    std::vector<const char*> llvm_args;
    for (const auto& s : args) {
        llvm_args.push_back(s.c_str());
    }

    // do the final ritual
    lld::lldMain(llvm_args, llvm::outs(), llvm::errs(), {platform_driver});
    if (run_exe) {

#ifdef WIN32
        system(std::format("{}", m_OutputPath.string()).c_str());
#else
        system(std::format("./{}", m_OutputPath.string()).c_str());
#endif
    }
}
