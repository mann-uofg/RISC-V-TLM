/*!
 \file GPIO.cpp
 \brief Basic GPIO peripheral
 \date 2026
*/
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <iomanip>
#include "GPIO.h"

namespace riscv_tlm::peripherals {

    GPIO::GPIO(sc_core::sc_module_name const &name) :
            sc_module(name), socket("gpio_socket") {
        socket.register_b_transport(this, &GPIO::b_transport);
    }

    void GPIO::b_transport(tlm::tlm_generic_payload &trans,
                           sc_core::sc_time &delay) {
        uint32_t *ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        uint64_t addr = trans.get_address();
        delay = sc_core::SC_ZERO_TIME;

        if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
            if (addr == 0x00) {
                gpio_dir = *ptr;
                std::cout << "[GPIO] Direction register set: 0x"
                          << std::hex << gpio_dir << std::endl;
            } else if (addr == 0x04) {
                gpio_data = *ptr;
                std::cout << "[GPIO] Output pins: 0x"
                          << std::hex << std::setw(8) << std::setfill('0')
                          << gpio_data << std::endl;
                for (int i = 7; i >= 0; i--) {
                    std::cout << "  Pin " << std::dec << i << ": "
                              << ((gpio_data >> i) & 1 ? "HIGH" : "LOW ")
                              << std::endl;
                }
            }
        } else if (trans.get_command() == tlm::TLM_READ_COMMAND) {
            *ptr = gpio_data;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}
