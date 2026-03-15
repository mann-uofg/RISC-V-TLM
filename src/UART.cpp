/*!
 \file UART.cpp
 \brief Basic UART peripheral
 \date 2026
*/
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "UART.h"

namespace riscv_tlm::peripherals {

    UART::UART(sc_core::sc_module_name const &name) :
            sc_module(name), socket("uart_socket") {
        socket.register_b_transport(this, &UART::b_transport);
    }

    void UART::b_transport(tlm::tlm_generic_payload &trans,
                           sc_core::sc_time &delay) {
        unsigned char *ptr = trans.get_data_ptr();
        delay = sc_core::SC_ZERO_TIME;

        if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
            // TX register write — output character to terminal
            std::cout << static_cast<char>(*ptr) << std::flush;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}
