#pragma once
#include <cstdint>
#include <libtcc.h>
#include <string>
#include <vector>

using FunctionHandle = uint32_t;

class JitCompiler {
    TCCState* state = nullptr;
    std::vector<std::string> fns;

public:
    JitCompiler() {
        this->state = tcc_new();
    }

    class FunctionBuilder {
        JitCompiler& compiler;
        FunctionHandle fn;

    public:
        FunctionBuilder(JitCompiler& compiler, const FunctionHandle handle) : compiler(compiler), fn(handle) {}


        FunctionBuilder addInstruction(const std::string& instr) {
            this->compiler.addInstruction(fn, instr);
            return std::move(*this);
        }
    };

    FunctionBuilder create_fn(std::string proto) {
        proto.push_back('{');
        this->fns.emplace_back(proto);
        return {*this, static_cast<FunctionHandle>(this->fns.size() - 1)};
    }

    void addInstruction(const FunctionHandle fn, const std::string& instr) {
        this->fns[fn].append(instr);
    }
};
