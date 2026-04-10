#pragma once
#include <vector>
#include <string>
#include <libtcc.h>
#include <mutex>

class JitCompiler {
public:
    static JitCompiler& instance() {
        static JitCompiler inst;
        return inst;
    }

    using GenericFn = void*;

    GenericFn compileMigration(const std::string& sourceCode, const std::string& funcName);

private:
    JitCompiler() = default;
    ~JitCompiler();

    std::mutex mutex;
    std::vector<TCCState*> compiledStates;
};
