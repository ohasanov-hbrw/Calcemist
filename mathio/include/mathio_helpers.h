#pragma once

#include "mathio_node.h"

namespace MathIO {

// AST Manipulation Helpers
void appendNode(Node* row, Node* newNode);
void insertNodeBefore(Node* row, Node* target, Node* newNode);
void insertNodeAfter(Node* row, Node* target, Node* newNode);
void removeNode(Node* row, Node* node, bool freeSubtree = true);

// Factory functions for compound 2D primitives
Node* createFraction();
Node* createPower();
Node* createSubscript();
Node* createRoot(bool withDegree = false);
Node* createIntegral(bool definite = false);
Node* createParenthesis();

} // namespace MathIO