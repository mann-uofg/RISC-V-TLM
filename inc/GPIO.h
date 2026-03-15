/*!
 \file GPIO.h
 \brief Basic GPIO peripheral
 \date 2026
*/
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "systemc"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <cstdint>

namespace riscv_tlm::peripherals {

    class GPIO : sc_core::sc_module {
    public:
        tlm_utils::simple_target_socket<GPIO> socket;

        explicit GPIO(sc_core::sc_module_name const &name);
        ~GPIO() override = default;

    private:
        uint32_t gpio_dir  = 0x00000000;  // direction register
        uint32_t gpio_data = 0x00000000;  // data register

        void b_transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay);
    };
}
