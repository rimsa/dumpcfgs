# dumpcfgs

dumpcfgs is a tool to dump (extract) control flow graphs (CFGs) statically
from binary programs using the DynInst (https://www.dyninst.org/dyninst) API.
It supports the output of CFGs in two formats: CFGgrind
(https://github.com/rimsa/CFGgrind) for an easily parseable output and DOT
(https://www.graphviz.org) for a human readable output.

## Building

Download spack from github and load its environment.

    $ git clone https://github.com/spack/spack.git
    $ . spack/share/spack/setup-env.sh

Install dyninst with spack with intel-tbb library.

    $ spack install dyninst ^intel-tbb

Load boost, intel-tbb and dyninst using spack. This step is responsible to set
the correct paths to include and library files.

    $ spack load boost
    $ spack load intel-tbb
    $ spack load dyninst

Download dumpcfgs and build with make.

    $ git clone https://github.com/rimsa/dumpcfgs
    $ cd dumpcfgs
    $ make

## Running

By default, dumpcfgs extracts the CFGs using th CFGgrind format - thus, no extra options
are required for this case.

    $ ./dumpcfgs /bin/sleep > sleep.cfgs

For DOT output, use the "-p dot" option.

    $ ./dumpcfgs -p dot /bin/sleep > sleep.dots

Note that this output contains all the CFGs in a single file.
For a dump of a specific CFG, use its address after the program in the
command line arguments:

    $ ./dumpcfgs -p dot /bin/sleep 0x401540 > cfg-0x401540.dot

Afterwards, you can generate the control flow graph using the graphviz tool dot
as a PNG (as exemplified below) or as a PDF (using -Tpdf instead) for improved 
resolution.

    $ dot -Tpng -o cfg-0x401540.png cfg-0x401540.dot

<p align="center">
    <img src="extras/cfg-0x401540.png?raw=true">
</p>

## Troubleshooting

If the module program is not available when loading dyninst, install
environment-modules. It may require a logout and a login to a new
terminal for this program to be available.

    $ spack load dyninst
    modules: command not found
    $ sudo apt-get install environment-modules

If the program outputs a message to set variable DYNINSTAPI_RT_LIB, search for
it in dyninst's spack library and update this variable accordingly.

    $ ./dumpcfgs /bin/sleep
    --SERIOUS-- #101: Environment variable DYNINSTAPI_RT_LIB has not been defined
    ...
    $ export DYNINSTAPI_RT_LIB=$(find $(spack location -i dyninst) -name 'libdyninstAPI_RT.so')

