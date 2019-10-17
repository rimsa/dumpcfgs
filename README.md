# dumpcfgs

dumpcfgs is a tool to dump (extract) control flow graphs (CFGs) statically
from binary programs using the DynInst (https://www.dyninst.org/dyninst) API.
It supports the output of CFGs in two formats: CFGgrind
(https://github.com/rimsa/CFGgrind) for a easily parseable output and DOT
(https://www.graphviz.org) for a human readable output.

## Building

Download spack from github and load its environment.

    $ git clone https://github.com/spack/spack.git
    $ . spack/share/spack/setup-env.sh

Install dyninst with spack.

    $ spack install dyninst

Load dyninst, tbb and boost from spack. This step is responsible to sets
the correct paths for include and library files.

    $ spack load dyninst
    $ spack load tbb
    $ spack load boost

Download dumpcfgs and build with make.

    $ git clone https://github.com/rimsa/dumpcfgs
    $ cd dumpcfgs
    $ make

## Running

By default, dumpcfgs extract the CFGs using CFGgrind format - no options are
required.

    $ ./dumpcfgs /bin/sleep > sleep.cfgs

For DOT output, use the "-p dot" option.

    $ ./dumpcfgs -p dot /bin/sleep > sleep.dots

Note that this output contains all the CFGs in a single file.
For a dump of a specific CFG, use its address after the program in the
command line arguments:

    $ ./dumpcfgs -p dot /bin/sleep 0x401540 > cfg-0x401540.dot

Afterwards, you can generate the control flow graph using the graphviz tool dot
as a PNG (examble below) or a PDF (using -Tpdf) for improved resolution.

    $ dot -Tpng -o cfg-0x401540.png cfg-0x401540.dot

<p align="center">
    <img src="extras/cfg-0x401540.png?raw=true">
</p>

## Troubleshooting

If the module program is not available when loading dyninst, install
environment-modules. It may require a logout and a login to a new
terminal for program to be available.

    $ spack load dyninst
    modules: command not found
    $ sudo apt-get install environment-modules

If the program outputs a message to set variable DYNINSTAPI_RT_LIB,
search for it in the $LIBRARY_PATH and update the variable accordingly.

    $ ./dumpcfgs /bin/sleep
    --SERIOUS-- #101: Environment variable DYNINSTAPI_RT_LIB has not been defined
    ...
    $ export DYNINSTAPI_RT_LIB="$(find ${LIBRARY_PATH//:/ } -name libdyninstAPI_RT.so)"
