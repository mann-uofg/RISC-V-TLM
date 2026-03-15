/*!
 \file CPU.cpp
 \brief Main CPU class
 \author Màrius Montón
 \date August 2018
 */
// SPDX-License-Identifier: GPL-3.0-or-later
#include "CPU.h"
#include "Tracer.h"

namespace riscv_tlm {

    SC_HAS_PROCESS(CPU);

    CPU::CPU(sc_core::sc_module_name const &name, bool debug) : sc_module(name), instr_bus("instr_bus"), inst(0), default_time(10, sc_core::SC_NS) {
        perf = Performance::getInstance();
        logger = spdlog::get("my_logger");

        m_qk = new tlm_utils::tlm_quantumkeeper();
        m_qk->reset();
        mem_intf = nullptr;
        dmi_ptr_valid = false;

        irq_already_down = false;

        tracer = new Tracer("trace.csv");

        interrupt = false;

        irq_line_socket.register_b_transport(this, &CPU::call_interrupt);

        trans.set_command(tlm::TLM_READ_COMMAND);

        trans.set_data_length(4);
        trans.set_streaming_width(4); // = data_length to indicate no streaming
        trans.set_byte_enable_ptr(nullptr); // 0 indicates unused
        trans.set_dmi_allowed(false); // Mandatory initial value
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        if (!debug) {
            SC_THREAD(CPU_thread);
        }
    };

    void CPU::invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) {
        (void) start;
        (void) end;
        dmi_ptr_valid = false;
    }

    CPU::~CPU() {
        if (tracer) {
            delete tracer;
            tracer = nullptr;
        }
    }

    [[noreturn]] void CPU::CPU_thread() {
        while (true) {
            /* Capture PC before execution (correct instruction address) */
            uint32_t pc_before = getCurrentPC();

            /* Process one instruction */
            CPU_step();

            /* Log only real instructions (guard against post-ECALL zeros) */
            uint32_t instr = getCurrentINSTR();
            if (instr != 0) {
                tracer->log(pc_before, instr);
            }

            /* Process IRQ (if any) */
            cpu_process_IRQ();

            sc_core::wait(default_time);
        }
    }
}