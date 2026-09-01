#pragma once

#include <string>
#include "mathio_node.h"

namespace MathIO {

// Parses a 1D GIAC output string and returns the root ROW Node of a 2D AST.
// Returns nullptr if parsing fails.
Node* parseGIAC(const std::string& input);

} // namespace MathIO