#pragma once

#include "mathio_node.h"
#include <string>

namespace MathIO {

// Prints the hierarchical structural AST tree with active cursor tracking
void printAST(Node* node, const Cursor* cursor = nullptr, std::string indent = "", bool isLast = true, const std::string& label = "");

// Renders and prints the AST as a formatted 2D monospace math expression
void printMathEquation(Node* root);

} // namespace MathIO