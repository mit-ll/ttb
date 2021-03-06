# Bomberman

**Author:**         Timothy Trippel <br>
**Affiliation:**    MIT Lincoln Laboratory <br>
**Last Updated:**   10/08/2019 <br>

Bomberman is a **ticking timebomb** (TTB) Trojan specific verification tool. It identifies suspicious state-saving components (SSCs) in a hardware design that could *potentially* be part of a TTB Trojan. Bomberman starts by assuming *all* SSCs are suspicious, and subsequently classifies each SSC as benign if it expresses values that violate a set of invariants during verification simulations. 

TTBs are comprised of SSCs that incrementally approach a triggering value. The set of invariants that define TTB SSC behavior are as follows:
1. values must never be repeated without a system reset, and 
2. all possible values must never be expressed without triggering the Trojan.

Bomberman leverages these two invariants while analyzing simulation results of a hardware design to classify whether or not an SSC is part of a TTB Trojan.

Bomberman consists of two main stages as shown in Figure 1:
1. SSC Identification
2. Simulation Analysis
<figure>
    <p align="center">
        <img src="/figures/bomberman.png" data-canonical-src="/figures/bomberman.png" width="75%"/>
        <br>
        <b>Figure 1: Bomberman Architecture</b>
    </p>
</figure>


The **SSC Identification** stage locates all SSCs in a hardware design described in Verilog. It does so by first generating a data-flow graph from the HDL using a custom Icarus Verilog (IVL) compiler back-end. The data-flow graph is encoded in the Graphviz `.dot` format, and passed to the next stage.

The **Simulation Analysis** stage parses: 1) the `.dot` file encoding the circuit data-flow graph generated by the *SSC Identification* stage, and 2) the verification simulation results, in the Value Change Dump (`.vcd`) format, generated by a hardware simulator (e.g., IVL). First, during *SSC Enumeration*, it enumerates all SSCs in the data-flow graph, marking each SSC as *suspicious*. There are two types of SSCs that are identified: *coalesced* and *distributed*. Next, during *Invariant Testing*, the values expressed by each SSC over the course of the simulation timeline are extracted from the simulation results. SSCs that violate either of the two invariants listed above are marked as *benign*.

# Directory Structure

### Circuits

The `circuits/` directory contains three example hardware designs to try out the Bomberman toolchain on. These designs are: a 128-bit AES accelerator (CNTR mode only), a UART core, and an OR1200 processor. For information on how to run the Bomberman toolchain on each hardware design, see the **E2E Bomberman Analysis of Real Circuits** section below.

### Scripts

The `scripts/` directory contains the Python scripts that implement the **Simulation Analysis** stage (Figure 1). The main script is contained in `analyze.py`.

### Tests

The `tests/` directory contains regression tests for: 1) the entire Bomberman analysis flow, contained in the `analysis_flow` sub-directory, and 2) the IVL back-end HDL data-flow graph generator, contained in the `ivl_ttb` sub-directory. 

The `analysis_flow` sub-directory contains three hardware designs and corresponding test benches to exercise the Bomberman toolchain. See the *Testing* section below for how to test bomberman on these designs.

The `ivl_ttb` sub-directory contains 62 regression tests (i.e., hardware designs) to exercise the IVL compiler back-end data-flow graph generator and verify its correctness. See the *Testing* section below for how to execute these tests.

### TGT-TTB

The `tgt-ttb/` directory contains the IVL compiler back-end data-flow graph generator that implements the *SSC Identification* stage.

# Installation

### 1. Repository Cloning

You can clone the repository using:

`git clone https://github.com/mit-ll/ttb.git`

### 2. Initialize Git Submodules

Bomberman utilizes [Icarus Verilog](https://github.com/steveicarus/iverilog) (IVL) as a submodule. Thus, the initialization of the IVL submodule must also be done as follows:

```
cd ttb/
git submodule update --init --recursive
```

### 3. Disabling optimization functions of IVL

Disabling the optimization functions of IVL is important for preserving the
input netlists structure as-is for analysis by Bomberman. To do so, you must **comment out** two blocks of code in the `ttb/iverilog/main.cc` file in the top-level IVL source code (lines 1251 and 1254-1260) submodule directory as follows:

Line 1244:

```cout << "RUNNING FUNCTORS" << endl;```

Line 1247--1253:
```
while (!net_func_queue.empty()) {
    net_func func = net_func_queue.front();
    net_func_queue.pop();
    if (verbose_flag)
        cerr<<" -F "<<net_func_to_name(func)<< " ..." <<endl;
    func(des);
}
```

***NOTE:** make sure to cross-reference the line numbers with the code snippets above since the IVL code base is actively managed and the exact line numbers are subject to change.*

### 4. Building IVL

To build IVL from source requires the following dependencies:

`autoconf`
`gperf`
`flex`
`bison`

On MacOS these can be installed with homebrew using: `brew install <package>`. After installing the dependencies, proceed to build IVL as follows: 

1. `cd iverilog`
2. comment out IVL optimization functions (see above)
3. `sh autoconf.sh`
4. `./configure --prefix=$(pwd)` 
5. `make install` 
6. `cd ..`

### 5. Building Data-Flow Graph Generator (tgt-ttb)

To compile and install into IVL for the first time:

1. `cd tgt-ttb`
2. `make all`
3. `cd ..`

To re-compile and re-install into IVL after modifications, replace step 2 (above) with: `make cleanall all`.

# Testing

### 1. Data-Flow Graph Generator

There are 62 regression tests to verify the correctness of the data-flow graph generator contained in the `tgt-ttb` IVL back-end target module. Each regression test consists of a small circuit, described in Verilog, that exercises the data-flow graph generator's ability to handle various Verilog syntax and expressions. All 62 regression tests should run, but only 61 tests should pass. This is due to a minor (known) error in the way the graph-generator handles duplicate signals in a concatenation, and will be fixed in a future release.

Each regression test generates a .dot file describing a data-flow graph from a simple test circuit. The resulting graph is automatically checked for correctness against provided *gold* examples. Additionally a PDF respresentation is generated for manual inspection of correctness, but this *requires Graphviz to be installed.*

To run all 62 regression tests use:

1. Install Graphviz: `brew install graphviz`
1. `cd tests/ivl_ttb`
2. `make test`
3. `cd ..`
4. `cd ..`

### 2. Bomberman E2E Analysis

There are 3 regression tests to verify the correctness of the entire Bomberman toolchain, from the data-flow graph generator to the simulation analysis scripts (Figure 1). Each regression tests is comprised of a small circuit design, and an associated test bench that excerises the design. The three circuits are: a simple counter (`counter/`), a D flip-flop (`d_ff/`), and a simple combinational circuit (`split/`).

***Note:*** *Bomberman's simulation analysis requires Python 2.7. You may have to modify the makefile (tests/analysis_flow/Makefile) to invoke Python 2.7, if the **python** alias on your system is mapped to Python 3.*

To run all 3 regression tests use:

1. `cd tests/analysis_flow`
2. `make all`
3. `cd ..`
4. `cd ..`

To run only a single regression test use: `make <design>`, where `<design>` is either `counter`, `d_ff`, or `split`.

# HDL Data-flow Graph Generation

Bomberman's data-flow graph generator (DFGG) can be utilized independently of Bomberman for performing various static analyses of a circuit's HDL. The DFGG is simply an Icarus Verilog (IVL) compiler back-end that generates data-flow graphs in the [Graphviz](https://www.graphviz.org/) `.dot` format. The DFGG takes the following as input:

|     | Input                  | Type             | Default    |
| --- | ---------------------- | ---------------- | ---------- |
|  1  | Clock Signal Basename  | string           | n/a        |
|  2  | Verilog Source Files   | file name(s)     | n/a        |
|  3  | Dot Output File Name   | string           | n/a        |

The DFGG can be invoked from the root repository directory (ttb/) using:

`iverilog/iverilog -o <dot output file name> -pclk=<clock basename> -t ttb <verilog source file> ...`

# E2E Bomberman Analysis of Real Circuits

There are three real-world circuit designs provided within this repository to experiment with. These designs include: a 128-bit AES accelerator ([TrustHub](https://trust-hub.org/home)), an 8-bit UART module ([OpenCores](https://opencores.org/projects/uart16550)), and an OR1200 processor CPU ([OpenCores](https://opencores.org/projects/uart16550)). To experiment analyzing each design with Bomberman, follow the steps below. 

***Note:*** *Bomberman's simulation analysis requires Python 2.7. You may have to modify the makefile (circuits/common.mk) to invoke Python 2.7, if the **python** alias on your system is mapped to Python 3. Alternatively, the makefile can be altered to invoke [PyPy](https://pypy.org/), instead of Python 2.7, to speed up computation. If you go this route, be sure to install PyPy on your system before proceeding.*

1.  `cd circuits`
2.  `cd aes`
3.  `make all LOG=1 OUT_DIR=tjfree`
4.  `cd ..`
5.  `cd uart`
6.  `make all LOG=1 OUT_DIR=tjfree`
7.  `cd ..`
8.  `cd or1200`
9.  `make all LOG=1 OUT_DIR=tjfree`
10. `cd ..`

The master Makefile (`circuits/common.mk`) that is invoked by running the above commands in each design sub-directory does three things. First, it invokes the *data-flow graph generator*, which generates a `.dot` file encoding the data-flow graph for the given hardware design. Second, *IVL* is invoked to simulate the hardware design and generate a `.vcd` file encoding the simulation trace. Third, the *simulation analysis* script is invoked to analyze the design for suspicious SSCs. The number of suspicious SSCs computed at different points throughout the simulation are output into several `.json` files.

There are several Jupyter Notebooks for plotting the results encoded in each `.json` file in the `circuits/plots` directory. Additionally, there are several scripts for running the Bomberman analysis of each design on a SLURM managed cluster, if more compute power is needed. However, each of the 3 provided designs has been tested on a 15in Macbook Pro with a 3.1 GHz Intel Core i7 processor and 16GB of DDR3 RAM, and each Bomberman analysis took less then a couple minutes to run.

# Citation
Please use this DOI number reference, published on [Zenodo](https://zenodo.org), when citing the software:    
[![DOI](https://zenodo.org/badge/222494101.svg)](https://zenodo.org/badge/latestdoi/222494101)

# Disclaimer

DISTRIBUTION STATEMENT A. Approved for public release. Distribution is unlimited.
 
<b>© 2019 MASSACHUSETTS INSTITUTE OF TECHNOLOGY</b>    
* Subject to FAR 52.227-11 – Patent Rights – Ownership by the Contractor (May 2014)  
* SPDX-License-Identifier: BSD-2-Clause    

This material is based upon work supported by the Under Secretary of Defense for Research and Engineering under Air Force Contract No. FA8702-15-D-0001. Any opinions, findings, conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Under Secretary of Defense for Research and Engineering.    

The software/firmware is provided to you on an As-Is basis
