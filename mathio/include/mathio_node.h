#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Handle ESP32 PSRAM macro fallback
#ifndef EXT_RAM_ATTR
#define EXT_RAM_ATTR
#endif

// Forward declaration of LovyanGFX font types
namespace lgfx {
    struct IFont;
}

// -----------------------------------------------------------------------------
// 1. Node Classification
// -----------------------------------------------------------------------------
enum class NodeType : uint8_t {
    // Basic Text / Leaf Terminals
    NUMBER,       // "123", "3.14"
    VARIABLE,     // "x", "y", "theta"
    OPERATOR,     // "+", "-", "*", "="
    
    // Containers
    ROW,          // Holds a horizontal sequence of nodes linked by next/prev
    FUNCTION,     // "sin", "cos", "ln" -> child[0] = Argument (usually a ROW)
    SYMBOL,

    // 2D Layout Primitives
    FRACTION,     // child[0] = Numerator, child[1] = Denominator
    POWER,        // child[0] = Base,      child[1] = Exponent
    SUBSCRIPT,    // child[0] = Base,      child[1] = Subscript
    ROOT,         // child[0] = Radicand,  child[1] = Degree (optional)
    PARENTHESIS,  // child[0] = Inner Content
    INTEGRAL      // child[0] = Integrand, child[1] = Variable, child[2] = Lower, child[3] = Upper
};

constexpr uint8_t MAX_CHILDREN = 4;
constexpr uint8_t TOKEN_MAX_LEN = 12;

// -----------------------------------------------------------------------------
// 2. Core AST Node Struct
// -----------------------------------------------------------------------------
struct Node {
    NodeType type;
    char token[TOKEN_MAX_LEN]; // String payload for NUMBER, VARIABLE, OPERATOR, FUNCTION

    // Navigation & Tree Hierarchy
    Node* parent;
    Node* prev;  // Previous sibling in a ROW
    Node* next;  // Next sibling in a ROW
    Node* children[MAX_CHILDREN]; // Subtree roots for structural regions

    // Layout Metrics (Calculated during bottom-up layout pass)
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    int16_t baseline; // Alignment axis relative to top of bounding box

    void reset() {
        type = NodeType::NUMBER;
        token[0] = '\0';
        parent = nullptr;
        prev = nullptr;
        next = nullptr;
        for (uint8_t i = 0; i < MAX_CHILDREN; ++i) {
            children[i] = nullptr;
        }
        x = y = w = h = baseline = 0;
    }
};

// -----------------------------------------------------------------------------
// 3. Layout & Font Context
// -----------------------------------------------------------------------------
struct RenderContext {
    const lgfx::IFont* mainFont = nullptr; // e.g. &fonts::Font0 or &fonts::Font2
    const lgfx::IFont* subFont  = nullptr; // e.g. &fonts::TomThumb or smaller font
    
    uint8_t lineThickness = 1; // Pixels for fraction bars, root overlines
    uint8_t fracPadding   = 2; // Horizontal padding on fraction lines
    uint8_t parenPadding  = 2; // Outer padding for scalable brackets
};

// -----------------------------------------------------------------------------
// 4. Cursor Navigation State
// -----------------------------------------------------------------------------
struct Cursor {
    Node* container; // The ROW node or text node the cursor is currently inside
    Node* position;  // Active node inside container (nullptr means end of row)
    bool visible;
};

// -----------------------------------------------------------------------------
// 5. PSRAM Node Pool Allocator
// -----------------------------------------------------------------------------
constexpr size_t MAX_NODE_POOL = 256;

class NodePool {
public:
    static NodePool& instance() {
        static NodePool pool;
        return pool;
    }

    Node* alloc(NodeType type) {
        for (size_t i = 0; i < MAX_NODE_POOL; ++i) {
            if (!used[i]) {
                used[i] = true;
                pool[i].reset();
                pool[i].type = type;
                return &pool[i];
            }
        }
        return nullptr; // Out of pool slots
    }

    void free(Node* node) {
        if (!node) return;

        // Recursively free structural children
        for (uint8_t i = 0; i < MAX_CHILDREN; ++i) {
            if (node->children[i]) {
                free(node->children[i]);
            }
        }

        // Recursively free sibling chain if node is a ROW
        if (node->type == NodeType::ROW && node->next) {
            free(node->next);
        }

        size_t index = node - pool;
        if (index < MAX_NODE_POOL) {
            used[index] = false;
        }
    }

    void resetAll() {
        memset(used, 0, sizeof(used));
    }

    // --- ADD THIS METHOD ---
    size_t getUsedCount() const {
        size_t count = 0;
        for (size_t i = 0; i < MAX_NODE_POOL; ++i) {
            if (used[i]) count++;
        }
        return count;
    }

private:
    EXT_RAM_ATTR Node pool[MAX_NODE_POOL];
    bool used[MAX_NODE_POOL] = {false};
};