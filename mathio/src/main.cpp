#include <iostream>
#include <string>
#include <cstring>

#include "mathio_node.h"
#include "mathio_symbols.h"
#include "mathio_helpers.h"
#include "mathio_traversal.h"
#include "mathio_ast_printer.h"
#include "giac_parser.h"


int main() {
    NodePool::instance().resetAll();

    std::string giacOutput = "((1)/(2))+(sqrt(x)^2)/integrate(sin(x*e/2)^2,x,-5..5)";
    std::cout << "Parsing GIAC String: " << giacOutput << "\n\n";

    Node* parsedRoot = MathIO::parseGIAC(giacOutput);

    MathIO::printAST(parsedRoot, nullptr, "", true, "ROOT");

    std::cout << "\n----------------------------------------\n";
    std::cout << "Re-Serialized GIAC Output: " << MathIO::serializeToGIAC(parsedRoot) << "\n";
    std::cout << "Allocated Pool Nodes:      " << NodePool::instance().getUsedCount() << " / " << MAX_NODE_POOL << "\n";
    std::cout << "----------------------------------------\n\n";

    MathIO::printMathEquation(parsedRoot);
    std::cout << "\n";



    

    return 0;
}