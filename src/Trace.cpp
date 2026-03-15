/*!
 \file Trace.cpp
 \brief Basic TLM-2 Trace module
 \author Màrius Montón
 \date September 2018
*/
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "Trace.h"

namespace riscv_tlm::peripherals {

    SC_HAS_PROCESS(Trace);

    Trace::Trace(sc_core::sc_module_name const &name) :
            sc_module(name), socket("socket") {
        socket.register_b_transport(this, &Trace::b_transport);
    }

    Trace::~Trace() {
    }

    void Trace::b_transport(tlm::tlm_generic_payload &trans,
                            sc_core::sc_time &delay) {
        unsigned char *ptr = trans.get_data_ptr();
        delay = sc_core::SC_ZERO_TIME;

        // Print directly to stdout instead of xterm
        std::cout << static_cast<char>(*ptr) << std::flush;

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}
