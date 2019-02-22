 /*
File:        propagate_lpm.cc
Author:      Timothy Trippel
Affiliation: MIT Lincoln Laboratory
Description:

This function propagtes an output nexus connected to 
an LPM device. There are many different types of LPMs
that can be encountered (see ivl_target.h) and each
type must be handled individually. 
*/

// Standard Headers
#include <cassert>
#include <cstdio>

// TTB Headers
#include "ttb.h"
#include "error.h"

// ----------------------------------------------------------------------------------
// ------------------------------- Helper Functions ---------------------------------
// ----------------------------------------------------------------------------------
const char* SignalGraph::get_lpm_type_as_string(ivl_lpm_t lpm) {
    switch(ivl_lpm_type(lpm)) {
        case IVL_LPM_ABS:
            return "IVL_LPM_ABS";
        case IVL_LPM_ADD:
            return "IVL_LPM_ADD";
        case IVL_LPM_ARRAY:
            return "IVL_LPM_ARRAY";
        case IVL_LPM_CAST_INT:
            return "IVL_LPM_CAST_INT";
        case IVL_LPM_CAST_INT2:
            return "IVL_LPM_CAST_INT2";
        case IVL_LPM_CAST_REAL:
            return "IVL_LPM_CAST_REAL";
        case IVL_LPM_CONCAT:
            return "IVL_LPM_CONCAT";
        case IVL_LPM_CONCATZ:
            return "IVL_LPM_CONCATZ";
        case IVL_LPM_CMP_EEQ:
            return "IVL_LPM_CMP_EEQ";
        case IVL_LPM_CMP_EQX:
            return "IVL_LPM_CMP_EQX";
        case IVL_LPM_CMP_EQZ:
            return "IVL_LPM_CMP_EQZ";
        case IVL_LPM_CMP_EQ:
            return "IVL_LPM_CMP_EQ";
        case IVL_LPM_CMP_GE:
            return "IVL_LPM_CMP_GE";
        case IVL_LPM_CMP_GT:
            return "IVL_LPM_CMP_GT";
        case IVL_LPM_CMP_NE:
            return "IVL_LPM_CMP_NE";
        case IVL_LPM_CMP_NEE:
            return "IVL_LPM_CMP_NEE";
        case IVL_LPM_DIVIDE:
            return "IVL_LPM_DIVIDE";
        case IVL_LPM_FF:
            return "IVL_LPM_FF";
        case IVL_LPM_MOD:
            return "IVL_LPM_MOD";
        case IVL_LPM_MULT:
            return "IVL_LPM_MULT";
        case IVL_LPM_MUX:
            return "IVL_LPM_MUX";
        case IVL_LPM_PART_VP:
            return "IVL_LPM_PART_VP";
        case IVL_LPM_PART_PV:
            return "IVL_LPM_PART_PV";
        case IVL_LPM_POW:
            return "IVL_LPM_POW";
        case IVL_LPM_RE_AND:
            return "IVL_LPM_RE_AND";
        case IVL_LPM_RE_NAND:
            return "IVL_LPM_RE_NAND";
        case IVL_LPM_RE_NOR:
            return "IVL_LPM_RE_NOR";
        case IVL_LPM_RE_OR:
            return "IVL_LPM_RE_OR";
        case IVL_LPM_RE_XNOR:
            return "IVL_LPM_RE_XNOR";
        case IVL_LPM_RE_XOR:
            return "IVL_LPM_RE_XOR";
        case IVL_LPM_REPEAT:
            return "IVL_LPM_REPEAT";
        case IVL_LPM_SFUNC:
            return "IVL_LPM_SFUNC";
        case IVL_LPM_SHIFTL:
            return "IVL_LPM_SHIFTL";
        case IVL_LPM_SHIFTR:
            return "IVL_LPM_SHIFTR";
        case IVL_LPM_SIGN_EXT:
            return "IVL_LPM_SIGN_EXT";
        case IVL_LPM_SUB:
            return "IVL_LPM_SUB";
        case IVL_LPM_SUBSTITUTE:
            return "IVL_LPM_SUBSTITUTE";
        case IVL_LPM_UFUNC:
            return "IVL_LPM_UFUNC";
        default:
            return "UNKNOWN";
    }
}

void SignalGraph::track_lpm_connection_slice(unsigned int      msb, 
                                             unsigned int      lsb,
                                             slice_node_type_t node_type){

    // Set signal slice info
    SliceInfo slice_info = {msb, lsb, node_type};

    // Check that slice-info stack is empty
    // (it should never grow beyond a size of 1)
    assert(signal_slices_.size() <= 1 && 
        "ERROR: slice info stack NOT empty (LPM-CONCAT).\n");

    // push slice info to a stack
    signal_slices_.push_back(slice_info);
}

// ----------------------------------------------------------------------------------
// ------------------------------- LPM Device Types ---------------------------------
// ----------------------------------------------------------------------------------
void SignalGraph::process_lpm_basic(ivl_lpm_t    lpm, 
                                    ivl_signal_t sink_signal, 
                                    string       ws) {

    // Device Input Nexus
    ivl_nexus_t input_nexus = NULL;

    // Propagate input(s)
    // Iterate over data input nexi
    for (unsigned int i = 0; i < ivl_lpm_size(lpm); i++) {

        // Get data input nexus
        input_nexus = ivl_lpm_data(lpm, i);

        // Propagate data input nexus
        propagate_nexus(input_nexus, sink_signal, ws + "    ");
    }    
}

void SignalGraph::process_lpm_part_select(ivl_lpm_t    lpm, 
                                          ivl_signal_t sink_signal, 
                                          string       ws) {

    // Device Nexuses
    ivl_nexus_t input_nexus = NULL;
    ivl_nexus_t base_nexus  = NULL;

    // Get input pin (0) nexus for part-select LPM device
    input_nexus = ivl_lpm_data(lpm, LPM_PART_SELECT_INPUT_NEXUS_INDEX);

    // Get base pin (1) nexus for part-select LPM device, which
    // may be NULL if not used is non-constant base is used.
    base_nexus = ivl_lpm_data(lpm, LPM_PART_SELECT_BASE_NEXUS_INDEX);
    if (base_nexus) {
        Error::not_supported_error("non-constant base for LPM part select device.");
    }

    // Get MSB and LSB of slice
    unsigned int msb = ivl_lpm_base(lpm) + ivl_lpm_width(lpm) - 1;
    unsigned int lsb = ivl_lpm_base(lpm);

    // Determine LPM type and track connection slice 
    if (ivl_lpm_type(lpm) == IVL_LPM_PART_VP) {
        // part select: vector to part (VP: part select in rval)
        track_lpm_connection_slice(msb, lsb, SOURCE);
    } else if (ivl_lpm_type(lpm) == IVL_LPM_PART_PV) {
        // part select: part to vector (PV: part select in lval)
        track_lpm_connection_slice(msb, lsb, SINK);
    } else {
        Error::unknown_part_select_lpm_type_error(ivl_lpm_type(lpm));
    }

    // Propagate nexus
    propagate_nexus(input_nexus, sink_signal, ws + "    ");
}

void SignalGraph::process_lpm_concat(ivl_lpm_t    lpm, 
                                     ivl_signal_t sink_signal, 
                                     string       ws) {

    // Device Input Nexus
    ivl_nexus_t input_nexus = NULL;

    // Device Input Nexus Pointer
    ivl_nexus_ptr_t input_nexus_ptr = NULL;

    // Device Input Signal
    ivl_signal_t source_signal = NULL;

    // Track bit slices
    // While it is undocumented in ivl_target.h, empiracally it
    // seems the concat inputs are always tracked from LSB->MSB.
    unsigned int current_msb         = 0;
    unsigned int current_lsb         = 0;

    // Iterate over input nexi
    for (unsigned int i = 0; i < ivl_lpm_size(lpm); i++) {
        
        // Get input nexus
        input_nexus = ivl_lpm_data(lpm, i);

        // Each input nexus should only have 2 nexus pointers:
        // 1) one to the LPM device and 2) a (non-local) signal.
        printf("%sNum nexus ptrs: %d\n", ws.c_str(), ivl_nexus_ptrs(input_nexus));
        // assert(ivl_nexus_ptrs(input_nexus) == 2 && 
            // "NOT-SUPPORTED: LPM-CONCAT device input \
            // nexus with more than 2 nexus pointers.\n");

        // Find input nexus ptr to a signal object
        for (unsigned int j = 0; j < ivl_nexus_ptrs(input_nexus); j++) {
            // Get input nexus pointer
            input_nexus_ptr = ivl_nexus_ptr(input_nexus, j);

            // Inputs should only be SIGNAL or LPM objects
            if ((source_signal = ivl_nexus_ptr_sig(input_nexus_ptr))) {
                // Compute current MSB of sink signal slice
                current_msb = current_lsb + ivl_signal_width(source_signal) - 1;

                // Track connection slice information
                track_lpm_connection_slice(current_msb, current_lsb, SINK);

                // Propagate input nexus
                propagate_nexus(input_nexus, sink_signal, ws + "    ");

                // Update current LSB of sink signal slice
                current_lsb += ivl_signal_width(source_signal);
                break;

            } else if ((ivl_nexus_ptr_lpm(input_nexus_ptr)) != lpm) {
                Error::not_supported_error("LPM-CONCAT input device type.");
            }
        }
    }
}

void SignalGraph::process_lpm_mux(ivl_lpm_t    lpm, 
                                  ivl_signal_t sink_signal, 
                                  string       ws) {

    // Device Input Nexus
    ivl_nexus_t input_nexus = NULL;
    
    // Propagate SELECT input(s)
    input_nexus = ivl_lpm_select(lpm);

    // Propagate select input nexus
    propagate_nexus(input_nexus, sink_signal, ws + "    ");

    // Propagate DATA input(s)
    process_lpm_basic(lpm, sink_signal, ws);
}

// ----------------------------------------------------------------------------------
// --------------------------- Main LPM Progation Switch ----------------------------
// ----------------------------------------------------------------------------------
void SignalGraph::propagate_lpm(ivl_lpm_t    lpm, 
                                ivl_nexus_t  sink_nexus, 
                                ivl_signal_t sink_signal, 
                                string       ws) {

    // Add connections
    switch (ivl_lpm_type(lpm)) {
        case IVL_LPM_CONCAT:
        case IVL_LPM_CONCATZ:
            // ivl_lpm_q() returns the output nexus. If the 
            // output nexus is NOT the same as the root nexus, 
            // then we do not propagate because this nexus is an 
            // to input to an LPM, not an output.

            if (ivl_lpm_q(lpm) == sink_nexus) {
                process_lpm_concat(lpm, sink_signal, ws);
            }

            break;
        case IVL_LPM_MUX:
            // ivl_lpm_q() returns the output nexus. If the 
            // output nexus is NOT the same as the root nexus, 
            // then we do not propagate because this nexus is an 
            // to input to an LPM, not an output.

            if (ivl_lpm_q(lpm) == sink_nexus) {
                process_lpm_mux(lpm, sink_signal, ws);
            }

            break;
        case IVL_LPM_PART_VP:
        case IVL_LPM_PART_PV:

            // ivl_lpm_q() returns the output nexus. If the 
            // output nexus is NOT the same as the root nexus, 
            // then we do not propagate because this nexus is an 
            // to input to an LPM, not an output.
            if (ivl_lpm_q(lpm) == sink_nexus) {
                process_lpm_part_select(lpm, sink_signal, ws);
            }

            break;
        case IVL_LPM_ADD:
        case IVL_LPM_CMP_EEQ:
        case IVL_LPM_CMP_EQX:
        case IVL_LPM_CMP_EQZ:
        case IVL_LPM_CMP_EQ:
        case IVL_LPM_CMP_GE:
        case IVL_LPM_CMP_GT:
        case IVL_LPM_CMP_NE:
        case IVL_LPM_CMP_NEE:
        case IVL_LPM_DIVIDE:
        case IVL_LPM_MULT:
        case IVL_LPM_POW:
        case IVL_LPM_RE_AND:
        case IVL_LPM_RE_NAND:
        case IVL_LPM_RE_NOR:
        case IVL_LPM_RE_OR:
        case IVL_LPM_RE_XNOR:
        case IVL_LPM_RE_XOR:
        case IVL_LPM_SHIFTL:
        case IVL_LPM_SHIFTR:
        case IVL_LPM_SUB:
            // ivl_lpm_q() returns the output nexus. If the 
            // output nexus is NOT the same as the root nexus, 
            // then we do not propagate because this nexus is an 
            // to input to an LPM, not an output.

            if (ivl_lpm_q(lpm) == sink_nexus) {
                process_lpm_basic(lpm, sink_signal, ws);
            }

            break;
        case IVL_LPM_ABS:
            Error::not_supported_error("LPM device type (IVL_LPM_ABS)");
            break;
        case IVL_LPM_ARRAY:
            Error::not_supported_error("LPM device type (IVL_LPM_ARRAY)");
            break;
        case IVL_LPM_CAST_INT:
            Error::not_supported_error("LPM device type (IVL_LPM_CAST_INT)");
            break;
        case IVL_LPM_CAST_INT2:
            Error::not_supported_error("LPM device type (IVL_LPM_CAST_INT2)");
            break;
        case IVL_LPM_CAST_REAL:
            Error::not_supported_error("LPM device type (IVL_LPM_CAST_REAL)");
            break;
        case IVL_LPM_FF:
            Error::not_supported_error("LPM device type (IVL_LPM_FF)");
            break;
        case IVL_LPM_MOD:
            Error::not_supported_error("LPM device type (IVL_LPM_MOD)");
            break;
        case IVL_LPM_REPEAT:
            Error::not_supported_error("LPM device type (IVL_LPM_REPEAT)");
            break;
        case IVL_LPM_SFUNC:
            Error::not_supported_error("LPM device type (IVL_LPM_SFUNC)");
            break;
        case IVL_LPM_SIGN_EXT:
            Error::not_supported_error("LPM device type (IVL_LPM_SIGN_EXT)");
            break;
        case IVL_LPM_SUBSTITUTE:
            Error::not_supported_error("LPM device type (IVL_LPM_SUBSTITUTE)");
            break;
        case IVL_LPM_UFUNC:
            Error::not_supported_error("LPM device type (IVL_LPM_UFUNC)");
            break;
        default:
            Error::not_supported_error("LPM device type (UNKNOWN)");
            break;
    }
}