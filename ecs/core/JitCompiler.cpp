#include "JitCompiler.hpp"
#include <memory>
#include <stdexcept>

JitCompiler::~JitCompiler() {
    for (TCCState* state : this->compiledStates) {
        tcc_delete(state);
    }
}

JitCompiler::GenericFn JitCompiler::compileMigration(const std::string& sourceCode, const std::string& funcName) {
    std::lock_guard<std::mutex> lock(this->mutex);

    TCCState* rawState = tcc_new();
    if (!rawState) {
        throw std::runtime_error("Could not create TCC state");
    }
    std::unique_ptr<TCCState, decltype(&tcc_delete)> state(rawState, &tcc_delete);

    tcc_set_output_type(state.get(), TCC_OUTPUT_MEMORY);

    if (tcc_compile_string(state.get(), sourceCode.c_str()) == -1) {
        throw std::runtime_error("TCC compilation failed for: " + sourceCode);
    }

    if (tcc_relocate(state.get()) == -1) {
        throw std::runtime_error("TCC relocation failed");
    }

    auto fn = tcc_get_symbol(state.get(), funcName.c_str());
    if (!fn) {
        throw std::runtime_error("Could not find symbol: " + funcName);
    }

    this->compiledStates.push_back(state.release());
    return fn;
}
