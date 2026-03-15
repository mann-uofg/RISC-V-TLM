/*!
 \file UART.h
 \brief Basic UART peripheral
 \date 2026
*/
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "systemc"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

namespace riscv_tlm::peripherals {

    class UART : sc_core::sc_module {
    public:
        tlm_utils::simple_target_socket<UART> socket;

        explicit UART(sc_core::sc_module_name const &name);
        ~UART() override = default;

    private:
        void b_transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay);
    };
}
