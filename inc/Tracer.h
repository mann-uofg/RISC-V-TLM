/*!
 \file Tracer.h
 \brief Instruction tracer
 \date 2026
*/
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <fstream>
#include <cstdint>
#include <string>

namespace riscv_tlm {

    class Tracer {
    public:
        explicit Tracer(const std::string &filename);
        ~Tracer();

        void log(uint32_t pc, uint32_t instr);

    private:
        std::ofstream trace_file;
        uint64_t count;
    };
}
