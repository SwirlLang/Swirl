#pragma once
#include <utils/utils.h>
#include <parser/Parser.h>
#include <backend/LLVMBackend.h>

class CompilerInst {
public:
    fs::path Path;
    Parser   Parser;

    explicit CompilerInst(const fs::path& path): Path(path), Parser(path) {}

    void parse() {
        Parser.parse();
    }

    /// SemA = semantic analysis
    void startSemA() {

    }

    void addDependency(CompilerInst* mod) {
        std::unique_lock lock(m_Mutex);
        m_Dependencies.push_back(mod);
    }

    const
    std::vector<CompilerInst*>& getDependencies() const {
        std::shared_lock lock(m_Mutex);
        return m_Dependencies;
    }

private:
    std::vector<CompilerInst*> m_Dependencies;
    std::shared_mutex    m_Mutex;
};
