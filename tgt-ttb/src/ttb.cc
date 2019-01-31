/*
File:        ttb.cc
Author:      Timothy Trippel
Affiliation: MIT Lincoln Laboratory
Description:

This is an Icarus Verilog backend target module that generates a signal
dependency graph for a given circuit design. The output format is a 
Graphviz .dot file.
*/

// Standard Headers
#include <cassert>
#include <cstdio>

// TTB Headers
#include "ttb.h"
#include "dot_graph.h"
#include "reporter.h"
#include "error.h"

// Returns true if the signal was generated by the IVL
// compiler as an output from a constant net object
ivl_net_const_t is_const_local_sig(ivl_signal_t sig){
	ivl_nexus_t 	nexus     = ivl_signal_nex(sig, 0);
	ivl_nexus_ptr_t nexus_ptr = NULL;
	ivl_net_const_t con       = NULL;

	// Check if local signal is connected to a constant
	for(unsigned i = 0; i < ivl_nexus_ptrs(nexus); i++){
		nexus_ptr = ivl_nexus_ptr(nexus, i);
		if ((con = ivl_nexus_ptr_con(nexus_ptr))){
			return con;
		}
	}

	return con;
}

void find_signals(ivl_scope_t scope, sig_name_map_t& signals_to_names, sig_map_t& signals){
	// Recurse into sub-modules
	for (unsigned int i = 0; i < ivl_scope_childs(scope); i++){
		find_signals(ivl_scope_child(scope, i), signals_to_names, signals);		
	}

	// Scope is a base scope
	unsigned int num_signals = ivl_scope_sigs(scope);
	for (unsigned int i = 0; i < num_signals; i++){
		// Check if signal already exists in map
		Error::check_signal_exists_in_map(signals, ivl_scope_sig(scope, i));
		signals[ivl_scope_sig(scope, i)]          = NULL;
		signals_to_names[ivl_scope_sig(scope, i)] = ivl_signal_name(ivl_scope_sig(scope, i));
	} 
}

void find_all_signals(ivl_scope_t* scopes, unsigned int num_scopes, sig_name_map_t& signals_to_names, sig_map_t& signals){
	for (unsigned int i = 0; i < num_scopes; i++){
		find_signals(scopes[i], signals_to_names, signals);
	}
}

void find_all_connections(sig_map_t& signals){
	return;
}

// *** "Main"/Entry Point *** of iverilog target
int target_design(ivl_design_t des) {
	ivl_scope_t*   roots     = 0;    // root scopes of the design
	unsigned       num_roots = 0;    // number of root scopes of the design
	Reporter       reporter;         // reporter object (prints messages)
	DotGraph       dg;               // dot graph object
	sig_name_map_t signals_to_names; // signal graph (adjacency list)
	sig_map_t      signals;          // signal graph (adjacency list)

	// Initialize reporter checking objects
	reporter = Reporter();
	reporter.init(LAUNCH_MESSAGE);

	// Initialize graphviz dot graph
	dg = DotGraph(ivl_design_flag(des, "-o"));
	dg.init_graph();

	// Get root scopes (top level modules) of design
	reporter.print_message(SCOPE_EXPANSION_MESSAGE);
	ivl_design_roots(des, &roots, &num_roots);
	Error::check_scope_types(roots, num_roots);
	reporter.root_scopes(roots, num_roots);

	// Find all critical signals and dependencies in the design
	find_all_signals(roots, num_roots, signals_to_names, signals);
	reporter.num_signals(signals);
	reporter.signal_names(signals);

	// Find signal-to-signal connections
	find_all_connections(signals);

	// Close graphviz dot file
	dg.save_graph();

	// Report total execution time
	reporter.end();

	// Stop timer
	// duration = (clock() - start) / (double) CLOCKS_PER_SEC;
	// printf("\nExecution Time: %f (s)\n", duration);
	// printf("-----------------------------\n");

	return 0;
}
