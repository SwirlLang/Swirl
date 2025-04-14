#pragma once
#include <utils/utils.h>
#include <parser/Parser.h>
#include <backend/LLVMBackend.h>

class Module {
public:
    fs::path Path;
    Parser   Parser;

    explicit Module(const fs::path& path): Path(path), Parser(path) {}

    void parse() {
        Parser.parse();
    }

    /// SemA = semantic analysis
    void startSemA() {

    }

    void addDependency(Module* mod) {
        std::unique_lock lock(m_Mutex);
        m_Dependencies.push_back(mod);
    }

    const
    std::vector<Module*>& getDependencies() const {
        std::shared_lock lock(m_Mutex);
        return m_Dependencies;
    }

private:
    std::vector<Module*> m_Dependencies;
    std::shared_mutex    m_Mutex;
};
