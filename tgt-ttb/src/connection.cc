/*
File:        connection.cc
Author:      Timothy Trippel
Affiliation: MIT Lincoln Laboratory
Description:

This is an Icarus Verilog backend target module that generates a signal
dependency graph for a given circuit design. The output format is a 
Graphviz .dot file.
*/

// Standard Headers
#include <sstream>

// TTB Headers
#include <connection.h>

// ----------------------------------------------------------------------------------
// ------------------------------- Constructors -------------------------------------
// ----------------------------------------------------------------------------------
Connection::Connection():
	source_(),
	source_msb_(0),
    source_lsb_(0),
    source_id_(0),
    sink_(),
    sink_msb_(0),
    sink_lsb_(0),
    sink_id_(0) {}

Connection::Connection(Signal* source, Signal* sink):
    source_(source),
    source_msb_(source->get_msb()),
    source_lsb_(source->get_lsb()),
    source_id_(source->get_id()),
    sink_(sink),
    sink_msb_(sink->get_msb()),
    sink_lsb_(sink->get_lsb()),
    sink_id_(sink->get_id()) {}

Connection::Connection(Signal*        source,
                       Signal*        sink,
                       signal_slice_t source_slice,
                       signal_slice_t sink_slice):

    source_(source),
    source_msb_(source_slice.msb),
    source_lsb_(source_slice.lsb),
    source_id_(source->get_id()),
    sink_(sink),
    sink_msb_(sink_slice.msb),
    sink_lsb_(sink_slice.lsb),
    sink_id_(sink->get_id()) {}

// ----------------------------------------------------------------------------------
// --------------------------------- Operators --------------------------------------
// ----------------------------------------------------------------------------------

bool Connection::operator==(const Connection& conn) const {
    return ((this->source_     == conn.get_source())     &&
            (this->sink_       == conn.get_sink())       &&
            (this->source_id_  == conn.get_source_id())  &&
            (this->sink_id_    == conn.get_sink_id())    &&
            (this->source_msb_ == conn.get_source_msb()) &&
            (this->source_lsb_ == conn.get_source_lsb()) &&
            (this->sink_msb_   == conn.get_sink_msb())   &&
            (this->sink_lsb_   == conn.get_sink_lsb()));
}

bool Connection::operator!=(const Connection& conn) const {
    return !(*this == conn);
}

// ----------------------------------------------------------------------------------
// ---------------------------------- Getters ---------------------------------------
// ----------------------------------------------------------------------------------
Signal* Connection::get_source() const {
    return source_;
}

Signal* Connection::get_sink() const {
    return sink_;   
}

unsigned int Connection::get_source_id() const {
    return source_id_;
}

unsigned int Connection::get_sink_id() const {
    return sink_id_;
}

unsigned int Connection::get_source_msb() const {
    return source_msb_;
}

unsigned int Connection::get_source_lsb() const {
    return source_lsb_;
}

unsigned int Connection::get_sink_msb() const {
    return sink_msb_;
}

unsigned int Connection::get_sink_lsb() const {
    return sink_lsb_;
}

// ----------------------------------------------------------------------------------
// ------------------------------ Dot Graph Getters ---------------------------------
// ----------------------------------------------------------------------------------
string Connection::get_dot_label() const {
    stringstream ss;
    
    ss << "[";
    ss << source_msb_;
    ss << ":";
    ss << source_lsb_;
    ss << "]->[";
    ss << sink_msb_;
    ss << ":";
    ss << sink_lsb_;
    ss << "]";

    return ss.str();
}

// ----------------------------------------------------------------------------------
// ---------------------------------- Setters ---------------------------------------
// ----------------------------------------------------------------------------------

void Connection::set_sink(Signal* new_sink) {
    sink_ = new_sink;
}

// ----------------------------------------------------------------------------------
// ----------------------------------- Debug ----------------------------------------
// ----------------------------------------------------------------------------------

DEBUG_PRINT(
void Connection::debug_print() const {
    
    fprintf(DEBUG_PRINTS_FILE_PTR, "%s.%d[%d:%d] <-- %s.%d[%d:%d]\n", 
        sink_->get_fullname().c_str(),
        sink_id_,
        sink_msb_,
        sink_lsb_,
        source_->get_fullname().c_str(),
        source_id_,
        source_msb_,
        source_lsb_);
}
)
