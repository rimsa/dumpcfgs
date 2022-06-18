/*

   Dump (extract) CFGs statically from binary programs using DynInst API.

   Copyright (C) 2019, Andrei Rimsa (andrei@cefetmg.br)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_process.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_function.h"
#include "BPatch_flowGraph.h"
#include "BPatch_point.h"
#include "BPatch_basicBlock.h"
#include "CFG.h"

#include <set>
#include <list>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cassert>

enum OutputMode {
    CFG_OUTPUT,
    DOT_OUTPUT
};

struct {
    enum OutputMode mode;
    bool dependencies;
    char* binary;
    Dyninst::Address faddr;
} config = { CFG_OUTPUT, false, 0, 0 };

void getBlockProperties(BPatch_function* funct, BPatch_basicBlock* block, std::set<Dyninst::ParseAPI::Block*>& succs,
        std::set<Dyninst::ParseAPI::Function*>& calls, bool& exit, bool& halt) {
    assert(block != 0);

    succs.clear();
    calls.clear();
    exit = false;
    halt = false;

    std::vector<Dyninst::ParseAPI::Function*> funcs;
    Dyninst::ParseAPI::Block* apiBlock = Dyninst::ParseAPI::convert(block);

    std::set<Dyninst::ParseAPI::Edge*> targets = apiBlock->targets();
    for (std::set<Dyninst::ParseAPI::Edge*>::iterator it = targets.begin(), ed = targets.end();
            it != ed; ++it) {
        Dyninst::ParseAPI::Edge* edge = *it;
        assert(edge != 0);

        Dyninst::ParseAPI::Block* to = edge->trg();

        switch (edge->type()) {
            case Dyninst::ParseAPI::CALL:
                funcs.clear();
                to->getFuncs(funcs);
                for (std::vector<Dyninst::ParseAPI::Function*>::iterator it2 = funcs.begin(), ed2 = funcs.end();
                        it2 != ed2; it2++) {
                    Dyninst::ParseAPI::Function* call = *it2;
                    if (call->retstatus() == Dyninst::ParseAPI::NORETURN)
                        halt = true;

                    calls.insert(call);
                }

                break;
            case Dyninst::ParseAPI::RET:
                exit = true;
                break;
            default:
                if (to->start() != 0 && to->start() != (Dyninst::Address) -1) {
                    if (funct->findPoint(to->start()))
                        succs.insert(to);
                    else {
                        funcs.clear();
                        to->getFuncs(funcs);
                        for (std::vector<Dyninst::ParseAPI::Function*>::iterator it2 = funcs.begin(), ed2 = funcs.end();
                                it2 != ed2; it2++)
                            calls.insert(*it2);
                    }
                }

                break;
        }
    }

    if (succs.size() == 0 && !exit && !halt) {
        Dyninst::ParseAPI::Function* apiFunction = Dyninst::ParseAPI::convert(funct);
        assert(apiFunction != 0);

        if (apiFunction->retstatus() == Dyninst::ParseAPI::NORETURN)
            halt = true;
        else
            exit = true;
    }
}

void dumpCFG(BPatch_function* funct, enum OutputMode mode) {
    int unknown = 1;
    bool has_exit = false, has_halt = false;
    assert(funct != 0);

    BPatch_flowGraph* fg = funct->getCFG();
    assert(fg != 0);

    bool complete = fg->containsDynamicCallsites();
    Dyninst::Address faddr = (Dyninst::Address) funct->getBaseAddr();
    std::string fname = funct->getName();

    std::cout << std::hex;
    switch (mode) {
        case CFG_OUTPUT:
            std::cout << "[cfg 0x" << faddr
                      << " \"" << fname << "\" " << (complete ? "true" : "false") << "]" << std::endl;
            break;
        case DOT_OUTPUT:
            std::cout << std::hex;
            std::cout << "digraph \"0x" << faddr << "\" {" << std::endl;
            std::cout << "  label = \"0x" << faddr << " (" << funct->getName() << ")\"" << std::endl;
            std::cout << "  labelloc = \"t\"" << std::endl;
            std::cout << "  node[shape=record]" << std::endl;
            std::cout << std::endl;
            std::cout << "  Entry [label=\"\",width=0.3,height=0.3,shape=circle,fillcolor=black,style=filled]" << std::endl;
            break;
        default:
            assert(0);
            break;
    }

    std::set<BPatch_basicBlock*> blocks;
    fg->getAllBasicBlocks(blocks);
    for (std::set<BPatch_basicBlock *>::iterator block_iter = blocks.begin(),
         block_end = blocks.end(); block_iter != block_end; ++block_iter) {
        BPatch_basicBlock* block = *block_iter;
        Dyninst::Address baddr = block->getStartAddress();
        size_t bsize = block->size();

        std::cout << std::hex;
        switch (mode) {
            case CFG_OUTPUT:
                std::cout << "[node 0x" << faddr << " 0x" << baddr;

                std::cout << std::dec;
                std::cout << " " << bsize << " [";
                break;
            case DOT_OUTPUT:
                std::cout << "  \"0x" << baddr << "\" [label=\"{" << std::endl;
                std::cout << "    0x" << baddr << " [" << std::dec << bsize << "]\\l" << std::endl;
                std::cout << "    | [instrs]\\l" << std::endl;
                break;
            default:
                assert(0);
        }

        std::vector<std::pair<Dyninst::InstructionAPI::Instruction, Dyninst::Address> > instrs;
        block->getInstructions(instrs);
        bool dynamic = false;
        for (std::vector<std::pair<Dyninst::InstructionAPI::Instruction, Dyninst::Address> >::iterator instr_iter = instrs.begin(),
             instr_end = instrs.end(); instr_iter != instr_end; ++instr_iter) {
            Dyninst::InstructionAPI::Instruction& instr = instr_iter->first;
            Dyninst::Address& iaddr = instr_iter->second;

            switch (mode) {
                case CFG_OUTPUT:
                    if (instr_iter != instrs.begin())
                        std::cout << " ";
                    std::cout << instr.size();
                    break;
                case DOT_OUTPUT:
                    std::cout << "    &nbsp;&nbsp;0x" << std::hex << iaddr << " \\<+" << std::dec << instr.size() << "\\>: " << instr.format() << "\\l" << std::endl;
                    break;
                default:
                    assert(0);
            }

            BPatch_point* point = block->findPoint(iaddr);
            assert(point);
            dynamic |= point->isDynamic();
        }

        std::cout << std::hex;
        switch (mode) {
            case CFG_OUTPUT:
                std::cout << "] [";
                break;
            case DOT_OUTPUT:
                break;
            default:
                assert(0);
        }

        bool exit, halt;
        std::set<Dyninst::ParseAPI::Block*> succs;
        std::set<Dyninst::ParseAPI::Function*> calls;
        getBlockProperties(funct, block, succs, calls, exit, halt);

        if (calls.size() > 0) {
            if (mode)
                std::cout << "     | [calls]\\l" << std::endl;

            for (std::set<Dyninst::ParseAPI::Function*>::iterator iter_call = calls.begin(), end_call = calls.end();
                iter_call != end_call; ++iter_call) {
                switch (mode) {
                    case CFG_OUTPUT:
                        if (iter_call != calls.begin())
                            std::cout << " ";

                        std::cout << "0x" << (*iter_call)->addr();
                        break;
                    case DOT_OUTPUT:
                        std::cout << "     &nbsp;&nbsp;0x" << (*iter_call)->addr() << " (" << (*iter_call)->name() << ")\\l" << std::endl;
                        break;
                    default:
                        assert(0);
                }
            }
        }

        switch (mode) {
            case CFG_OUTPUT:
                std::cout << "] [] " << (dynamic ? "true" : "false") << " [";
                break;
            case DOT_OUTPUT:
                std::cout << "  }\"]" << std::endl;

                if (dynamic) {
                    std::cout << "  \"Unknown" << unknown << "\" [label=\"?\", shape=none]" << std::endl;
                    std::cout << "  \"0x" << baddr << "\" -> \"Unknown" << unknown << "\" [style=dashed]" << std::endl;
                    unknown++;
                }

                if (block->isEntryBlock())
                    std::cout << "  Entry -> \"0x" << baddr << "\"" << std::endl;

                break;
            default:
                assert(0);
        }

        bool outgoing = false;
        for (std::set<Dyninst::ParseAPI::Block*>::iterator succ_iter = succs.begin(),
                 succ_end = succs.end(); succ_iter != succ_end; ++succ_iter) {
            Dyninst::ParseAPI::Block* succ = *succ_iter;

            switch (mode) {
                case CFG_OUTPUT:
                    if (succ_iter != succs.begin())
                        std::cout << " ";
                    std::cout << "0x" << succ->start();
                    break;
                case DOT_OUTPUT:
                    std::cout << "  \"0x" << baddr << "\" -> \"0x" << succ->start() << "\"" << std::endl;
                    break;
                default:
                    assert(0);
            }

            outgoing = true;
        }

        if (exit) {
            if (!has_exit) {
                if (mode == DOT_OUTPUT)
                    std::cout << "  Exit [label=\"\",width=0.3,height=0.3,shape=circle,fillcolor=black,style=filled,peripheries=2]" << std::endl;

                has_exit = true;
            }

            switch (mode) {
                case CFG_OUTPUT:
                    if (outgoing)
                        std::cout << " ";
                    std::cout << "exit";
                    break;
                case DOT_OUTPUT:
                    std::cout << "  \"0x" << baddr << "\" -> Exit" << std::endl;
                    break;
                default:
                    assert(0);
            }

            outgoing = true;
        }

        if (halt) {
            if (!has_halt) {
                if (mode == DOT_OUTPUT)
                    std::cout << "  Halt [label=\"\",width=0.3,height=0.3,shape=square,fillcolor=black,style=filled,peripheries=2]" << std::endl;

                has_halt = true;
            }

            switch (mode) {
                case CFG_OUTPUT:
                    if (outgoing)
                        std::cout << " ";
                    std::cout << "halt";
                    break;
                case DOT_OUTPUT:
                    std::cout << "  \"0x" << baddr << "\" -> Halt" << std::endl;
                    break;
                default:
                    assert(0);
            }

            outgoing = true;
        }

        assert(outgoing);

        if (mode == CFG_OUTPUT)
            std::cout << "]]" << std::endl;
    }

    if (mode == DOT_OUTPUT)
        std::cout << "}" << std::endl;

//    assert(has_exit || has_halt);
}

void usage(char* progname) {
    std::cout << "Usage: " << progname << " <Options> [Binary Program] <Function Address>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "   -p  Mode   Print mode, where" << std::endl;
    std::cout << "                 cfg: CFGgrind format [default]" << std::endl;
    std::cout << "                 dot: graphviz DOT format" << std::endl;
    std::cout << "   -d         Dump shared dependencies" << std::endl;
    std::cout << std::endl;

    exit(1);
}

void readoptions(int argc, char* argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, ":p:d")) != -1) {
        switch (opt) {
            case 'p':
                if (strcasecmp(optarg, "cfg") == 0)
                    config.mode = CFG_OUTPUT;
                else if (strcasecmp(optarg, "dot") == 0)
                    config.mode = DOT_OUTPUT;
                else
                    throw std::string("Invalid mode: ") + optarg;

                break;
            case 'd':
                config.dependencies = true;
                break;
            default:
                throw std::string("Invalid option: ") + (char) optopt;
        }
    }

    if (optind >= argc)
        usage(argv[0]);

    config.binary = argv[optind++];

    if (optind < argc)
        config.faddr = (Dyninst::Address) strtol(argv[optind++], NULL, 16);

    if (optind < argc)
       throw std::string("Unknown extra option: ") + argv[optind];
}

int main(int argc, char* argv[]) {
    BPatch bpatch;

    try {
        readoptions(argc, argv);
    } catch (const std::string& e) {
        std::cerr << e << std::endl;
        exit(1);
    }

    BPatch_addressSpace* app = bpatch.openBinary(config.binary, config.dependencies);
    if (!app)
        return 1;

    BPatch_image* appImage = app->getImage();
    assert(appImage != 0);

    std::vector<BPatch_function*>* functs = appImage->getProcedures(true);
    assert(functs != 0);

    for (std::vector<BPatch_function *>::iterator functs_it = functs->begin(),
         functs_end = functs->end(); functs_it != functs_end; ++functs_it) {
        if (config.faddr == 0 || config.faddr == (Dyninst::Address) (*functs_it)->getBaseAddr())
            dumpCFG(*functs_it, config.mode);
    }

    return 0;
}
