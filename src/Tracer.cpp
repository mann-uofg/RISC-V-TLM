/*!
 \file Tracer.cpp
 \brief Instruction tracer
 \date 2026
*/
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Tracer.h"
#include <iostream>
#include <iomanip>

namespace riscv_tlm {

    Tracer::Tracer(const std::string &filename) : count(0) {
        trace_file.open(filename);
        trace_file << "count,PC,instruction\n";
        std::cout << "[Tracer] Logging to " << filename << std::endl;
    }

    Tracer::~Tracer() {
        trace_file.flush();
        trace_file.close();
        std::cout << "[Tracer] " << count
                  << " instructions logged." << std::endl;
    }

    void Tracer::log(uint32_t pc, uint32_t instr) {
        trace_file << std::dec << count++ << ","
                   << "0x" << std::hex << std::setw(8) << std::setfill('0') << pc << ","
                   << "0x" << std::hex << std::setw(8) << std::setfill('0') << instr
                   << "\n";
    }
}
