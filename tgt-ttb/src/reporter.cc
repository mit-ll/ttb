/*
File:        dot_graph.cc
Author:      Timothy Trippel
Affiliation: MIT Lincoln Laboratory
Description:

This is an Icarus Verilog backend target module that generates a signal
dependency graph for a given circuit design. The output format is a 
Graphviz .dot file.
*/

// Standard Headers
#include <cassert>
#include <ctime>

// TTB Headers
#include "reporter.h"

Reporter::Reporter(): file_path_(NULL), file_ptr_(NULL){}

Reporter::Reporter(const char* p): file_ptr_(NULL){
	set_file_path(p);
}

void Reporter::set_file_path(const char* p){
	file_path_ = p;
}

const char* Reporter::get_file_path(){
	return file_path_;
}

void Reporter::init(const char* init_message){
	// Open output file or print to STDOUT
	if (file_path_ == NULL){
		file_ptr_ = stdout;
	} else {
		open_file();
	}

	print_message(init_message);

	// Record start time
	start_time_ = clock();
}

void Reporter::print_message(const char* message){
	// Check that file has been opened for writing report
	assert(file_ptr_ != NULL && "ERROR: reporter file ptr is NULL.\n");

	// Print init message
	fprintf(file_ptr_, "-----------------------------\n");
	fprintf(file_ptr_, "%s\n", message);
}

void Reporter::root_scopes(ivl_scope_t* scopes, unsigned int num_scopes){
	// Check that file has been opened for writing report
	assert(file_ptr_ != NULL && "ERROR: reporter file ptr is NULL.\n");

	// Print number of scopes
	fprintf(file_ptr_, "Found %d top-level module(s):\n", num_scopes);
	
	for (unsigned int i = 0; i < num_scopes; i++){	
		// Check scope is of type "module"
		assert(ivl_scope_type(scopes[i]) == IVL_SCT_MODULE && "UNSUPPORTED: Verilog scope type.\n");

		// Print scope name
		fprintf(file_ptr_, "	%s\n", ivl_scope_name(scopes[i]));
	}
}

void Reporter::num_signals(vector<ivl_signal_t> signals){
	// Check that file has been opened for writing report
	assert(file_ptr_ != NULL && "ERROR: reporter file ptr is NULL.\n");

	return;
}

void Reporter::end(){
	// Check that file has been opened for writing report
	assert(file_ptr_ != NULL && "ERROR: reporter file ptr is NULL.\n");

	// Record current execution time
	execution_time_ = (clock() - start_time_) / (double) CLOCKS_PER_SEC;
	fprintf(file_ptr_, "\nExecution Time: %f (s)\n", execution_time_);
	fprintf(file_ptr_, "-----------------------------\n");

	// Close output file
	fclose(file_ptr_);
}

FILE* Reporter::get_file_ptr(){
	return file_ptr_;
}

void Reporter::open_file(){
	file_ptr_ = fopen(file_path_, "w");
	if (!file_ptr_) {
		printf("ERROR: Could not open file %s\n", file_path_ ? file_path_ : "stdout");
		exit(-1);
	}
}

void Reporter::close_file(){
	if (file_ptr_ != stdout){
		fclose(file_ptr_);
	}
	file_ptr_ = NULL;
}