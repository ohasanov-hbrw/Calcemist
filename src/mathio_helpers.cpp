#include "mathio_helpers.h"

namespace MathIO {

void appendNode(Node* row, Node* newNode) {
    if (!row || !newNode || row->type != NodeType::ROW) return;

    newNode->parent = row;
    newNode->next = nullptr;

    if (!row->children[0]) {
        // First node in the ROW
        row->children[0] = newNode;
        newNode->prev = nullptr;
    } else {
        // Traverse to the end of the sibling list
        Node* curr = row->children[0];
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = newNode;
        newNode->prev = curr;
    }
}

void insertNodeBefore(Node* row, Node* target, Node* newNode) {
    if (!row || !newNode || row->type != NodeType::ROW) return;

    newNode->parent = row;

    if (!target || target == row->children[0]) {
        // Prepend to front of ROW
        newNode->prev = nullptr;
        newNode->next = row->children[0];
        if (row->children[0]) {
            row->children[0]->prev = newNode;
        }
        row->children[0] = newNode;
    } else {
        newNode->prev = target->prev;
        newNode->next = target;
        if (target->prev) {
            target->prev->next = newNode;
        }
        target->prev = newNode;
    }
}

void insertNodeAfter(Node* row, Node* target, Node* newNode) {
    if (!row || !newNode || row->type != NodeType::ROW) return;
    if (!target) {
        appendNode(row, newNode);
        return;
    }

    newNode->parent = row;
    newNode->prev = target;
    newNode->next = target->next;

    if (target->next) {
        target->next->prev = newNode;
    }
    target->next = newNode;
}

void removeNode(Node* row, Node* node, bool freeSubtree) {
    if (!row || !node || row->type != NodeType::ROW) return;

    // Unlink from sibling chain
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        // Removing head of row
        row->children[0] = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    }

    node->parent = nullptr;
    node->prev = nullptr;
    node->next = nullptr;

    if (freeSubtree) {
        NodePool::instance().free(node);
    }
}

// -----------------------------------------------------------------------------
// Compound Factory Helpers
// -----------------------------------------------------------------------------

Node* createFraction() {
    Node* frac = NodePool::instance().alloc(NodeType::FRACTION);
    if (!frac) return nullptr;

    Node* numRow = NodePool::instance().alloc(NodeType::ROW);
    Node* denRow = NodePool::instance().alloc(NodeType::ROW);

    frac->children[0] = numRow;
    frac->children[1] = denRow;
    if (numRow) numRow->parent = frac;
    if (denRow) denRow->parent = frac;

    return frac;
}

Node* createPower() {
    Node* pow = NodePool::instance().alloc(NodeType::POWER);
    if (!pow) return nullptr;

    Node* baseRow = NodePool::instance().alloc(NodeType::ROW);
    Node* expRow  = NodePool::instance().alloc(NodeType::ROW);

    pow->children[0] = baseRow;
    pow->children[1] = expRow;
    if (baseRow) baseRow->parent = pow;
    if (expRow)  expRow->parent  = pow;

    return pow;
}

Node* createSubscript() {
    Node* sub = NodePool::instance().alloc(NodeType::SUBSCRIPT);
    if (!sub) return nullptr;

    Node* baseRow = NodePool::instance().alloc(NodeType::ROW);
    Node* subRow  = NodePool::instance().alloc(NodeType::ROW);

    sub->children[0] = baseRow;
    sub->children[1] = subRow;
    if (baseRow) baseRow->parent = sub;
    if (subRow)  subRow->parent  = sub;

    return sub;
}

Node* createRoot(bool withDegree) {
    Node* root = NodePool::instance().alloc(NodeType::ROOT);
    if (!root) return nullptr;

    Node* radRow = NodePool::instance().alloc(NodeType::ROW);
    root->children[0] = radRow;
    if (radRow) radRow->parent = root;

    if (withDegree) {
        Node* degRow = NodePool::instance().alloc(NodeType::ROW);
        root->children[1] = degRow;
        if (degRow) degRow->parent = root;
    }

    return root;
}

Node* createIntegral(bool definite) {
    Node* integ = NodePool::instance().alloc(NodeType::INTEGRAL);
    if (!integ) return nullptr;

    Node* integrandRow = NodePool::instance().alloc(NodeType::ROW);
    Node* varRow       = NodePool::instance().alloc(NodeType::ROW);

    integ->children[0] = integrandRow;
    integ->children[1] = varRow;
    if (integrandRow) integrandRow->parent = integ;
    if (varRow)       varRow->parent       = integ;

    if (definite) {
        Node* lowerRow = NodePool::instance().alloc(NodeType::ROW);
        Node* upperRow = NodePool::instance().alloc(NodeType::ROW);
        integ->children[2] = lowerRow;
        integ->children[3] = upperRow;
        if (lowerRow) lowerRow->parent = integ;
        if (upperRow) upperRow->parent = integ;
    }

    return integ;
}

Node* createParenthesis() {
    Node* paren = NodePool::instance().alloc(NodeType::PARENTHESIS);
    if (!paren) return nullptr;

    Node* innerRow = NodePool::instance().alloc(NodeType::ROW);
    paren->children[0] = innerRow;
    if (innerRow) innerRow->parent = paren;

    return paren;
}

} // namespace MathIO