/*!
 \file CPU.cpp
 \brief Main CPU class
 \author Màrius Montón
 \date August 2018
 */
// SPDX-License-Identifier: GPL-3.0-or-later
#include "CPU.h"

namespace riscv_tlm {

    CPURV64::CPURV64(sc_core::sc_module_name const &name, BaseType PC, bool debug) :
            CPU(name, debug), INSTR(0) {

        register_bank = new Registers<BaseType>();
        mem_intf = new MemoryInterface();
        register_bank->setPC(PC);
        register_bank->setValue(Registers<BaseType>::sp, (Memory::SIZE / 4) - 1);

        int_cause = 0;

        instr_bus.register_invalidate_direct_mem_ptr(this,
                                                     &CPURV64::invalidate_direct_mem_ptr);

        base_inst = new BASE_ISA<BaseType>(0, register_bank, mem_intf);
        c_inst = new C_extension<BaseType>(0, register_bank, mem_intf);
        m_inst = new M_extension<BaseType>(0, register_bank, mem_intf);
        a_inst = new A_extension<BaseType>(0, register_bank, mem_intf);

        trans.set_data_ptr(reinterpret_cast<unsigned char *>(&INSTR));

        logger->info("Created CPURV64 CPU");
        std::cout << "Created CPURV64 CPU" << std::endl;
    }

    CPURV64::~CPURV64() {
        if (register_bank) {
            delete register_bank;
            register_bank = nullptr;
        }
        if (mem_intf) {
            delete mem_intf;
            mem_intf = nullptr;
        }
        if (base_inst) {
            delete base_inst;
            base_inst = nullptr;
        }
        if (c_inst) {
            delete c_inst;
            c_inst = nullptr;
        }
        if (m_inst) {
            delete m_inst;
            m_inst = nullptr;
        }
        if (a_inst) {
            delete a_inst;
            a_inst = nullptr;
        }
        // m_qk is handled by base class destructor
    }

    bool CPURV64::cpu_process_IRQ() {
        BaseType csr_temp;
        bool ret_value = false;

        if (interrupt) {
            csr_temp = register_bank->getCSR(CSR_MSTATUS);
            if ((csr_temp & MSTATUS_MIE) == 0) {
                logger->debug("{} ns. PC: 0x{:x}. Interrupt delayed", sc_core::sc_time_stamp().value(),
                              register_bank->getPC());

                return ret_value;
            }

            /* Map cause code to the corresponding mip/mie bit */
            BaseType cause_code = int_cause & 0x7FFFFFFF;
            BaseType mip_bit;
            BaseType mie_bit;
            switch (cause_code) {
                case 3:  mip_bit = MIP_MSIP; mie_bit = MIE_MSIE; break;
                case 7:  mip_bit = MIP_MTIP; mie_bit = MIE_MTIE; break;
                case 11: mip_bit = MIP_MEIP; mie_bit = MIE_MEIE; break;
                default: mip_bit = MIP_MEIP; mie_bit = MIE_MEIE; break;
            }

            /* Check per-source enable in mie register */
            BaseType mie_val = register_bank->getCSR(CSR_MIE);
            if ((mie_val & mie_bit) == 0) {
                logger->debug("{} ns. PC: 0x{:x}. Interrupt masked by mie",
                              sc_core::sc_time_stamp().value(),
                              register_bank->getPC());
                return ret_value;
            }

            csr_temp = register_bank->getCSR(CSR_MIP);

            if ((csr_temp & mip_bit) == 0) {
                csr_temp |= mip_bit;
                register_bank->setCSR(CSR_MIP, csr_temp);

                logger->debug("{} ns. PC: 0x{:x}. Interrupt!", sc_core::sc_time_stamp().value(),
                              register_bank->getPC());

                /* Save mstatus privilege stack: MPIE <- MIE, MIE <- 0, MPP <- M */
                BaseType mstatus = register_bank->getCSR(CSR_MSTATUS);
                if (mstatus & MSTATUS_MIE) {
                    mstatus |= MSTATUS_MPIE;
                } else {
                    mstatus &= ~MSTATUS_MPIE;
                }
                mstatus &= ~MSTATUS_MIE;
                /* MPP <- previous privilege mode (M-mode for now)
                 * TODO: when U/S-mode is implemented, set MPP to the
                 * privilege mode that was active before the trap */
                mstatus &= ~(0x3 << 11); /* clear MPP (bits 12:11) */
                mstatus |= (0x3 << 11);  /* 0x3 = M-mode */
                register_bank->setCSR(CSR_MSTATUS, mstatus);

                /* updated MEPC register */
                BaseType old_pc = register_bank->getPC();
                register_bank->setCSR(CSR_MEPC, old_pc);

                logger->debug("{} ns. PC: 0x{:x}. Old PC Value 0x{:x}", sc_core::sc_time_stamp().value(),
                              register_bank->getPC(),
                              old_pc);

                /* update MCAUSE register, use the cause from the interrupt source */
                register_bank->setCSR(CSR_MCAUSE, int_cause);

                /* mtval is set to 0 for standard interrupts */
                register_bank->setCSR(CSR_MTVAL, 0);

                /* set new PC address, respecting mtvec MODE */
                BaseType mtvec = register_bank->getCSR(CSR_MTVEC);
                BaseType mode = mtvec & 0x3;
                BaseType base = mtvec & ~0x3;
                BaseType new_pc;
                if (mode == 1) {
                    /* Vectored: interrupts go to BASE + 4 * cause_code */
                    new_pc = base + 4 * cause_code;
                } else {
                    /* Direct: all traps go to BASE */
                    new_pc = base;
                }
                logger->debug("{} ns. PC: 0x{:x}. NEW PC Value 0x{:x}", sc_core::sc_time_stamp().value(),
                              register_bank->getPC(),
                              new_pc);
                register_bank->setPC(new_pc);

                ret_value = true;
                interrupt = false;
                irq_already_down = false;
            }
        } else {
            if (!irq_already_down) {
                csr_temp = register_bank->getCSR(CSR_MIP);
                /* Clear the pending bit that was set during interrupt delivery.
                   Use int_cause to determine which bit to clear. */
                BaseType cause_code = int_cause & 0x7FFFFFFF;
                BaseType mip_bit;
                switch (cause_code) {
                    case 3:  mip_bit = MIP_MSIP; break;
                    case 7:  mip_bit = MIP_MTIP; break;
                    case 11: mip_bit = MIP_MEIP; break;
                    default: mip_bit = MIP_MEIP; break;
                }
                csr_temp &= ~mip_bit;
                register_bank->setCSR(CSR_MIP, csr_temp);
                irq_already_down = true;
            }
        }

        return ret_value;
    }

    bool CPURV64::CPU_step() {

        /* Get new PC value */
        if (dmi_ptr_valid) {
            /* if memory_offset at Memory module is set, this won't work */
            std::memcpy(&INSTR, dmi_ptr + register_bank->getPC(), 4);
        } else {
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            tlm::tlm_dmi dmi_data;
            trans.set_address(register_bank->getPC());
            instr_bus->b_transport(trans, delay);

            if (trans.is_response_error()) {
                SC_REPORT_ERROR("CPU base", "Read memory");
            }

            if (trans.is_dmi_allowed()) {
                dmi_ptr_valid = instr_bus->get_direct_mem_ptr(trans, dmi_data);
                if (dmi_ptr_valid) {
                    std::cout << "Get DMI_PTR " << std::endl;
                    dmi_ptr = dmi_data.get_dmi_ptr();
                }
            }
        }

        perf->codeMemoryRead();
        inst.setInstr(INSTR);
        bool breakpoint = false;

        base_inst->setInstr(INSTR);
        auto deco = base_inst->decode();

        if (deco != OP_ERROR) {
            auto PC_not_affected = base_inst->exec_instruction(inst, &breakpoint, deco);
            if (PC_not_affected) {
                register_bank->incPC();
            }

        } else {
            c_inst->setInstr(INSTR);
            auto c_deco = c_inst->decode();
            if (c_deco != OP_C_ERROR ) {
                auto PC_not_affected = c_inst->exec_instruction(inst, &breakpoint, c_deco);
                if (PC_not_affected) {
                    register_bank->incPCby2();
                }
            } else {
                m_inst->setInstr(INSTR);
                auto m_deco = m_inst->decode();
                if (m_deco != OP_M_ERROR) {
                    auto PC_not_affected = m_inst->exec_instruction(inst, m_deco);
                    if (PC_not_affected) {
                        register_bank->incPC();
                    }
                } else {
                    a_inst->setInstr(INSTR);
                    auto a_deco = a_inst->decode();
                    if (a_deco != OP_A_ERROR) {
                        auto PC_not_affected = a_inst->exec_instruction(inst, a_deco);
                        if (PC_not_affected) {
                            register_bank->incPC();
                        }
                    } else {
                        std::cout << "Extension not implemented yet" << std::endl;
                        inst.dump();
                        base_inst->NOP();
                    }
                }
            }
        }

        if (breakpoint) {
            std::cout << "Breakpoint set to true\n";
        }

        perf->instructionsInc();

        return breakpoint;
    }

    void CPURV64::call_interrupt(tlm::tlm_generic_payload &m_trans,
                              sc_core::sc_time &delay) {
        interrupt = true;
        /* Socket caller send a cause (its id) */
        memcpy(&int_cause, m_trans.get_data_ptr(), sizeof(BaseType));
        delay = sc_core::SC_ZERO_TIME;
    }

    std::uint64_t CPURV64::getStartDumpAddress() {
        return register_bank->getValue(Registers<std::uint32_t>::t0);
    }

    std::uint64_t CPURV64::getEndDumpAddress() {
        return register_bank->getValue(Registers<std::uint32_t>::t1);
    }

    uint32_t CPURV64::getCurrentPC() {
        return static_cast<uint32_t>(register_bank->getPC());
    }

    uint32_t CPURV64::getCurrentINSTR() {
        return static_cast<uint32_t>(INSTR);
    }
}