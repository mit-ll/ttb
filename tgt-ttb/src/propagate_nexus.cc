/*
File:        propagate_nexus.cc
Author:      Timothy Trippel
Affiliation: MIT Lincoln Laboratory
Description:

This function propagtes a root signal through a nexus. A nexus
is how IVL connects all data-structures (signals, logic-devices,
LPMs, and constant ). Think of a nexus like a pin, connecting
nets and devices together. Each nexus is associated with a 
single IVL data-structure, where data-structures can have
multiple nexis associated with them. Nexi contain muliplte 
nexus pointers that point to other nexus. Every nexus contains 
a nexus pointer to itself, and the rest are pointers to all the 
IVL data-structures it connects. 
*/

// Standard Headers

// TTB Headers
#include <ttb_typedefs.h>
#include <tracker.h>
#include <error.h>

void Tracker::propagate_nexus(ivl_nexus_t nexus, Signal* sink_signal, string ws) {
    
    // Nexus Pointer
    ivl_nexus_ptr_t nexus_ptr = NULL;

    // Connected Objects
    ivl_signal_t    source_signal    = NULL;
    ivl_net_logic_t source_logic     = NULL;
    ivl_lpm_t       source_lpm       = NULL;
    ivl_net_const_t source_constant  = NULL;

    // Processed non-local signal flag
    bool proccessed_signal = false;

    // Check if nexus already explored and mark nexus as explored
    if (explored_nexi_.insert(nexus).second) {

        // Iterate over Nexus pointers in Nexus
        for (unsigned int nexus_ind = 0; nexus_ind < ivl_nexus_ptrs(nexus); nexus_ind++) {

            nexus_ptr = ivl_nexus_ptr(nexus, nexus_ind);
            DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, "%sNexus %d", ws.c_str(), nexus_ind);)

            // Determine type of Nexus
            if ((source_signal = ivl_nexus_ptr_sig(nexus_ptr))){

                // BASE-CASE: nexus target object is a SIGNAL
                DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, " -- SIGNAL -- %s (%s)\n", 
                    sg_->get_signal_from_ivl_signal(source_signal)->get_fullname().c_str(), 
                    get_signal_port_type_as_string(source_signal));)

                // Check that source and sink signals are different
                if (sink_signal->get_ivl_signal() != source_signal) {

                    // Set ID of (potential) source signal (arrayed signals)
                    sg_->get_signal_from_ivl_signal(source_signal)->set_id(ivl_nexus_ptr_pin(nexus_ptr));

                    // Process (potential) source signal
                    proccessed_signal |= propagate_signal(source_signal, sink_signal, ws);
                }

            } else if ((source_logic = ivl_nexus_ptr_log(nexus_ptr))) {
                
                // Nexus target object is a LOGIC
                DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, " -- LOGIC -- %s\n", get_logic_type_as_string(source_logic));)
                if (!proccessed_signal) {
                    propagate_logic(source_logic, nexus, sink_signal, ws);
                }

            } else if ((source_lpm = ivl_nexus_ptr_lpm(nexus_ptr))) {
                
                // Nexus target object is a LPM
                DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, " -- LPM -- %s\n", get_lpm_type_as_string(source_lpm));)
                if (!proccessed_signal) {
                    // ivl_lpm_q() returns the output nexus. If the 
                    // output nexus is NOT the same as the root nexus, 
                    // then we do not propagate because this nexus is an 
                    // to input to an LPM, not an output.
                    if (ivl_lpm_q(source_lpm) == nexus) {
                        propagate_lpm(source_lpm, sink_signal, ws);
                    }
                }

            } else if ((source_constant = ivl_nexus_ptr_con(nexus_ptr))) {
                
                // Nexus target object is a CONSTANT
                DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, " -- CONSTANT -- %s\n", get_const_type_as_string(source_constant));)
                if (!proccessed_signal) {
                    propagate_constant(source_constant, sink_signal, ws);
                }

            } else {
                
                // Nexus target object is UNKNOWN
                Error::unknown_nexus_type();

            }
        }

        // Remove nexus from set
        explored_nexi_.erase(nexus);

    } else {

        // Iterate over Nexus pointers in Nexus
        for (unsigned int nexus_ind = 0; nexus_ind < ivl_nexus_ptrs(nexus); nexus_ind++) {

            nexus_ptr = ivl_nexus_ptr(nexus, nexus_ind);
            DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, "%sNexus %d", ws.c_str(), nexus_ind);)

            // Only propagate signal nexus
            if ((source_signal = ivl_nexus_ptr_sig(nexus_ptr))){

                // BASE-CASE: nexus target object is a SIGNAL
                DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, " -- SIGNAL -- %s (%s)\n", 
                    sg_->get_signal_from_ivl_signal(source_signal)->get_fullname().c_str(), 
                    get_signal_port_type_as_string(source_signal));)

                // Check if sink signal is the same as the source signal.
                // If yes, this is a loopback assignment. We need to propagate back one
                // level, updating the source signal and source ID/slice information.
                if (sink_signal == sg_->get_signal_from_ivl_signal(source_signal)) {

                    // Get sink/source signal slices
                    signal_slice_t loopback_source_slice = sink_signal->get_source_slice(sg_->get_signal_from_ivl_signal(source_signal));
                    signal_slice_t loopback_sink_slice   = sink_signal->get_sink_slice(sink_signal);

                    // Check that looback has mis-matching sink/source slices
                    if (loopback_source_slice.msb != loopback_sink_slice.msb &&
                        loopback_source_slice.lsb != loopback_sink_slice.lsb) {
                        
                        DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, "%sLoopback signal found. Correcting source to %s\n",
                            ws.c_str(),
                            sink_signal->get_fullname().c_str());)

                        // Set ID of (potential) source signal (arrayed signals)
                        sg_->get_signal_from_ivl_signal(source_signal)->set_id(ivl_nexus_ptr_pin(nexus_ptr));

                        // Process (potential) source signal
                        proccessed_signal |= propagate_signal(source_signal, sink_signal, ws);

                        // Reset Slices
                        sink_signal->reset_source_slice();

                    } else {

                        // Error
                        Error::infinite_loopback_assignment(sink_signal);
                    }
                }

            } else {

                // This nexus has already been explored, do not propagate non-signal loopback assignments
                DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, " -- WARNING: skipping non-signal loopback...\n");)
            }
        }
    }
}
