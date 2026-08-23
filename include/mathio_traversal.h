#pragma once

#include "mathio_node.h"
#include "mathio_symbols.h"
#include <string>

namespace MathIO {

// -----------------------------------------------------------------------------
// 1. Generic AST Node Visitor Callback
// -----------------------------------------------------------------------------
using NodeVisitor = void(*)(Node* node, int depth, void* userData);

// Depth-first preorder traversal across the entire MathIO tree
inline void traverse(Node* node, NodeVisitor visitor, int depth = 0, void* userData = nullptr) {
    if (!node) return;

    // Visit current node
    visitor(node, depth, userData);

    // 1. Traverse structural children subtrees
    for (uint8_t i = 0; i < MAX_CHILDREN; ++i) {
        if (node->children[i]) {
            traverse(node->children[i], visitor, depth + 1, userData);
        }
    }

    // 2. Traverse sibling chain if current node is inside a ROW
    if (node->type != NodeType::ROW && node->next != nullptr) {
        traverse(node->next, visitor, depth, userData);
    }
}

// -----------------------------------------------------------------------------
// 2. Serialization: Convert 2D MathIO AST to 1D GIAC Expression String
// -----------------------------------------------------------------------------
std::string serializeToGIAC(Node* node) {
    if (!node) return "";

    std::string result;

    switch (node->type) {
        case NodeType::ROW: {
            Node* curr = node->children[0];
            while (curr) {
                result += serializeToGIAC(curr);
                curr = curr->next;
            }
            break;
        }
        case NodeType::NUMBER:
        case NodeType::VARIABLE:
        case NodeType::OPERATOR:
        case NodeType::SYMBOL:
            result = node->token;
            break;

        case NodeType::FRACTION:
            result = "((" + serializeToGIAC(node->children[0]) + ")/(" + serializeToGIAC(node->children[1]) + "))";
            break;

        case NodeType::POWER:
            result = "((" + serializeToGIAC(node->children[0]) + ")^(" + serializeToGIAC(node->children[1]) + "))";
            break;

        case NodeType::ROOT:
            result = "sqrt(" + serializeToGIAC(node->children[0]) + ")";
            break;

        case NodeType::PARENTHESIS:
            result = "(" + serializeToGIAC(node->children[0]) + ")";
            break;

        case NodeType::FUNCTION: {
            result = std::string(node->token) + "(";
            for (int i = 0; i < MAX_CHILDREN && node->children[i]; ++i) {
                if (i > 0) result += ",";
                result += serializeToGIAC(node->children[i]);
            }
            result += ")";
            break;
        }

        case NodeType::INTEGRAL: {
            result = "integrate(" + serializeToGIAC(node->children[0]);
            if (node->children[1]) result += "," + serializeToGIAC(node->children[1]);
            if (node->children[2] && node->children[3]) {
                result += "," + serializeToGIAC(node->children[2]) + ".." + serializeToGIAC(node->children[3]);
            }
            result += ")";
            break;
        }

        default:
            break;
    }

    return result;
}

} // namespace MathIO