/*
DISTRIBUTION STATEMENT A. Approved for public release. Distribution is unlimited.
 
This material is based upon work supported by the Under Secretary of Defense for 
Research and Engineering under Air Force Contract No. FA8702-15-D-0001. Any 
opinions, findings, conclusions or recommendations expressed in this material 
are those of the author(s) and do not necessarily reflect the views of the 
Under Secretary of Defense for Research and Engineering.
 
© 2019 Massachusetts Institute of Technology.
 
MIT Proprietary, Subject to FAR52.227-11 Patent Rights - Ownership by the contractor (May 2014)
 
The software/firmware is provided to you on an As-Is basis
 
Delivered to the U.S. Government with Unlimited Rights, as defined in DFARS 
Part 252.227-7013 or 7014 (Feb 2014). Notwithstanding any copyright notice, 
U.S. Government rights in this work are defined by DFARS 252.227-7013 or 
DFARS 252.227-7014 as detailed above. Use of this work other than as specifically 
authorized by the U.S. Government may violate any copyrights that exist in this work.
*/

/*
File:        ttb.cc
Author:      Timothy Trippel
Affiliation: MIT Lincoln Laboratory
Description:

This is an Icarus Verilog backend target module that generates a signal
dependency graph for a given circuit design. The output format is a 
Graphviz .dot file.
*/

// ------------------------------------------------------------
// ------------------------- Includes -------------------------
// ------------------------------------------------------------

// Standard Headers
#include <cassert>
#include <fstream>
#include <limits>
#include <algorithm>

// IVL API Header
#include <ivl_target.h>

// TTB Headers
#include <ttb_typedefs.h>
#include <ttb.h>
#include <signal.h>
#include <signal_graph.h>
#include <tracker.h>
#include <reporter.h>
#include <error.h>

// ------------------------------------------------------------
// -------------------------- Other ---------------------------
// ------------------------------------------------------------

string Tracker::get_file_line(ivl_statement_t statement) {

    // String on line: <line_num>
    string line_string;

    // Create file stream object
    fstream file(ivl_stmt_file(statement));

    // Go to beginning of the file
    file.seekg(ios::beg);

    // Seek to line number of interest
    for(unsigned int i = 0; i < ivl_stmt_lineno(statement) - 1; i++){
        file.ignore(numeric_limits<streamsize>::max(),'\n');
    }

    // Set line string
    getline(file, line_string);

    return line_string;
}

void Tracker::print_statement_hdl(ivl_statement_t statement, string ws) {
    
    string source_code_line = Tracker::get_file_line(statement);

    // Print HDL file and line number of event
    fprintf(DEBUG_PRINTS_FILE_PTR, "%sHDL file: %s; line: %u\n", 
        ws.c_str(), 
        ivl_stmt_file(statement),
        ivl_stmt_lineno(statement));

    // Print HDL code of event
    fprintf(DEBUG_PRINTS_FILE_PTR, "%sHDL code line: %s\n", 
        ws.c_str(), 
        source_code_line.c_str());
}

void Tracker::print_string_list(vector<string> str_list) {

    // define iterator
    vector<string>::iterator it = str_list.begin();

    fprintf(DEBUG_PRINTS_FILE_PTR, "String List:\n");

    // iterate over list items
    while (it != str_list.end()) {

        // print list item
        fprintf(DEBUG_PRINTS_FILE_PTR, "%s\n", (*it).c_str());

        // increment iterator
        it++;
    }
}

vector<string> Tracker::tokenize_string(string s, char delimeter) {
    
    size_t position = 0;
    string token;
    vector<string> tokens;

    // Tokenize full name string by delimeter
    while ((position = s.find(delimeter)) != string::npos) {
        token = s.substr(0, position);
        tokens.push_back(token);
        s.erase(0, position + 1);
    }

    // Push remaining string to token list
    tokens.push_back(s);

    return tokens;
}

vector<string> Tracker::remove_string_from_list(vector<string> str_list, string str2remove) {

    // Iterate over list
    for(unsigned int i = 0; i < str_list.size(); i++) {
        
        // Check if string is to be removed
        if(str_list[i] == str2remove) {
            str_list.erase(str_list.begin() + i);
            --i;
        }
    }

    return str_list;
}

vector<string> Tracker::get_event_sensitivity_list(string hdl_code_line) {

    // Remove Trailing ')'
    hdl_code_line.erase(
        remove(hdl_code_line.begin(), hdl_code_line.end(), ')'), 
        hdl_code_line.end());

    // Split HDL code line by '('
    vector<string> signals_list = Tracker::tokenize_string(hdl_code_line, '(');

    // Pop the front half
    signals_list.erase(signals_list.begin());

    // Remove key words from signals list
    signals_list = Tracker::remove_string_from_list(signals_list, string("posedge"));
    signals_list = Tracker::remove_string_from_list(signals_list, string("negedge"));
    signals_list = Tracker::remove_string_from_list(signals_list, string("or"));
    signals_list = Tracker::remove_string_from_list(signals_list, string("and"));

    // Last token (substring) is the basename
    return signals_list;
}

string Tracker::get_event_signal_basename(string fullname) {

    // Tokenize full name string by '.' delimeter
    vector<string> tokens = tokenize_string(fullname, '.');

    // Last token (substring) is the basename
    return tokens.back();
}

vector<string> Tracker::convert_fullnames_to_basenames(vector<string> signals_list) {

    // Signal basenames list
    vector<string> signal_basenames_list;

    // Iterate over list
    for(unsigned int i = 0; i < signals_list.size(); i++) {
        
        // Convert signal fullname to basename
        signal_basenames_list.push_back(
            Tracker::get_event_signal_basename(signals_list[i]));
    }

    return signal_basenames_list;
}

// ------------------------------------------------------------
// -------------- CMD Line Arguments Processing ---------------
// ------------------------------------------------------------

cmd_args_map_t* process_cmd_line_args(ivl_design_t des) {

    // Create map to hold cmd line args
    cmd_args_map_t* cmd_args = new cmd_args_map_t();

    // Process output filename
    string output_filename = string(ivl_design_flag(des, OUTPUT_FILENAME_FLAG));
    if (!output_filename.empty()) {
        (*cmd_args)[OUTPUT_FILENAME_FLAG] = output_filename; 
    } else {
        Error::not_supported("output filename is required input.\n");
    } 

    // Process CLK signal basename
    string clk_basename = string(ivl_design_flag(des, CLK_BASENAME_FLAG));
    if (!clk_basename.empty()) {
        (*cmd_args)[CLK_BASENAME_FLAG] = clk_basename; 
    } else {
        Error::not_supported("CLK signal name is required input.\n");
    } 

    // Process signals to ignore filepath
    string ignore_filepath = string(ivl_design_flag(des, IGNORE_FILEPATH_FLAG));
    if (!ignore_filepath.empty()) {
        (*cmd_args)[IGNORE_FILEPATH_FLAG] = ignore_filepath; 
    }
    
    // Process ignore constants flag
    string ignore_consts = string(ivl_design_flag(des, IGNORE_CONSTANTS_FLAG));
    if (!ignore_consts.empty()) {
        (*cmd_args)[IGNORE_CONSTANTS_FLAG] = ignore_consts;
    }

    return cmd_args;
}

// ------------------------------------------------------------
// ----------------- Continuous HDL Processing ----------------
// ------------------------------------------------------------

void Tracker::find_continuous_connections() {

    // Signal object pointer
    Signal* current_signal = NULL;

    // (Current) sink signal nexus
    ivl_nexus_t sink_nexus = NULL;

    // Get signals adjacency list
    sig_map_t signals_map = sg_->get_signals_map();

    // Create a signals map iterator
    sig_map_t::iterator it = signals_map.begin();
    
    // Set slice tracking flags
    enable_slicing();

    // Iterate over all signals in adjacency list
    while (it != signals_map.end()) {  

        // Get Signal object
        current_signal = it->second;

        // Reset Signal slices
        current_signal->reset_slices();

        // Only find connections to non-ivl-generated signals
        if (!current_signal->is_ivl_generated()) {

            // Print signal name and port type
            DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, "%s (%s):\n", 
                current_signal->get_fullname().c_str(),
                get_signal_port_type_as_string(current_signal->get_ivl_signal()));)

            // fprintf(DEBUG_PRINTS_FILE_PTR, "Dims = %d, Base = %d; Count = %d; Swapped = %d\n", 
            //     ivl_signal_dimensions(it->first),
            //     ivl_signal_array_base(it->first), 
            //     ivl_signal_array_count(it->first),
            //     ivl_signal_array_addr_swapped(it->first));
            
            // Iterate over signal nexi
            for (unsigned int i = ivl_signal_array_base(it->first); i < ivl_signal_array_count(it->first); i++) {
                
                // Get signal nexus
                // There is exactly one nexus for each WORD of a signal.
                sink_nexus = ivl_signal_nex(it->first, i);

                // Check nexus is not NULL
                if (sink_nexus) {

                    // Set sink signal ID to nexus index (arrayed)
                    current_signal->set_id(i);

                    // Reset explored nexi set
                    explored_nexi_.clear();

                    // Propagate the nexus
                    propagate_nexus(sink_nexus, it->second, WS_TAB);

                    // Check all explored nexi removed from set
                    assert(explored_nexi_.size() == 0 &&
                        "ERROR-Tracker::find_continuous_connections: nexi left unprocessed.\n");
                    
                } else {

                    // Nexus is NULL --> skip it
                    DEBUG_PRINT(fprintf(DEBUG_PRINTS_FILE_PTR, "%sskipping (NULL) nexus for signal word %d\n", WS_TAB, i);)
                }
            }
        }

        // Reset Signal slices
        current_signal->reset_slices();

        // Increment the iterator
        it++;
    }

    // Clear slice tracking flags
    disable_slicing();
}

// ------------------------------------------------------------
// ----------------- Procedural HDL Processing ----------------
// ------------------------------------------------------------

int find_procedural_connections(ivl_process_t process, void* t) {
    
    // Cast type for Tracker
    Tracker* tracker = (Tracker*) t;

    // Launch procedural logic processing
    return tracker->process_process(process);
}

void launch_ivl_design_process(ivl_design_t design, Tracker* t) {
    int result = 0;

    // Processes are initial, always, or final blocks
    // Goes through all assignments in process blocks.
    result = ivl_design_process(design, find_procedural_connections, t);
    if (result != 0) {
        Error::processing_procedural_connections();
    }
}

// ------------------------------------------------------------
// -------------- Initialize Constant ID Counter --------------
// ------------------------------------------------------------

unsigned int Signal::const_id = 0;

// ------------------------------------------------------------
// -------------- IVL Target Entry-point ("main") -------------
// ------------------------------------------------------------
int target_design(ivl_design_t design) {
    ivl_scope_t*    roots     = 0;    // root scopes of the design
    unsigned        num_roots = 0;    // number of root scopes of the design
    cmd_args_map_t* cmd_args  = NULL; // command line args
    Reporter*       reporter  = NULL; // reporter object (prints messages)
    SignalGraph*    sg        = NULL; // signal graph object
    Tracker*        tracker   = NULL; // connection tracker object

    // Create reporter processing object
    reporter = new Reporter(REPORTER_PRINTS_FILE_PTR);
    
    // Start timer
    reporter->start_task(LAUNCH_MESSAGE);

    // Get IVL design flags (CMD-line args)
    reporter->start_task(CONFIGS_MESSAGE);
    cmd_args = process_cmd_line_args(design);
    reporter->configurations(cmd_args);
    reporter->end_task();

    // Create & Initialize SignalGraph
    reporter->start_task(INITIALIZE_SIG_GRAPH_MESSAGE);
    sg = new SignalGraph(cmd_args);
    reporter->end_task();

    // Create & Initialize Tracker
    reporter->start_task(INITIALIZE_SIG_GRAPH_MESSAGE);
    tracker = new Tracker(cmd_args, sg);
    reporter->end_task();
    
    // Get root scopes (top level modules) of design
    reporter->start_task(SCOPE_EXPANSION_MESSAGE);
    ivl_design_roots(design, &roots, &num_roots);
    Error::check_scope_types(roots, num_roots);
    reporter->root_scopes(roots, num_roots);
    reporter->end_task();

    // Find all SIGNALS in the design
    reporter->start_task(SIGNAL_ENUM_MESSAGE);
    sg->find_all_signals(roots, num_roots);
    reporter->num_signals(sg->get_num_signals());
    reporter->signal_names(sg->get_signals_map());
    reporter->end_task();

    // Find CONTINUOUS signal-to-signal CONNECTIONS
    reporter->start_task(COMB_CONNECTION_ENUM_MESSAGE);
    tracker->find_continuous_connections();
    reporter->end_task();

    // Find PROCEDURAL signal-to-signal CONNECTIONS
    reporter->start_task(BEHAVE_CONNECTION_ENUM_MESSAGE);
    launch_ivl_design_process(design, tracker);
    reporter->end_task();

    // Process connections through local (IVL-generated) signals
    reporter->start_task(LOCAL_CONNECTION_OPT_MESSAGE);
    sg->process_local_connections(WS_TAB);
    reporter->end_task();

    // Write signal nodes to dot graph and save
    reporter->start_task(SIGNAL_SAVING_MESSAGE);
    sg->write_signals_to_dot_graph();
    sg->save_dot_graph();
    reporter->end_task();

    // Report stats/total execution time
    reporter->print_message(FINAL_STATS_MESSAGE);
    reporter->graph_stats(sg);
    reporter->end_task();
    reporter->line_separator();
    
    // Delete Objects
    DEBUG_DESTRUCTORS(reporter->start_task(DESTROY_MESSAGE);)
    delete(cmd_args);
    delete(tracker);
    delete(sg);
    DEBUG_DESTRUCTORS(reporter->end_task();)
    DEBUG_DESTRUCTORS(reporter->line_separator();)
    delete(reporter);

    return 0;
}
