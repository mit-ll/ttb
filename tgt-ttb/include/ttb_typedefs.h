/*
File:        ttb_typedefs.h
Author:      Timothy Trippel
Affiliation: MIT Lincoln Laboratory
Description:

This is an Icarus Verilog backend target module that generates a signal
dependency graph for a given circuit design. The output format is a 
Graphviz .dot file.
*/

#ifndef __TTB_TYPEDEFS_HEADER__
#define __TTB_TYPEDEFS_HEADER__

// Standard Headers
#include <vector>
#include <map>

using namespace std;

// IVL API Header
#include  <ivl_target.h>

// Signal name to signal object map
typedef map<ivl_signal_t, const char*> sig_name_map_t;

// Signal graph adjaceny list
typedef map<ivl_signal_t, vector<ivl_signal_t>> sig_map_t;

#endif
