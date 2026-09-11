#include "giac_cursor.hpp"
#include <algorithm>

namespace MathRenderer {

AstCursor::AstCursor() : root_(nullptr) {}

void AstCursor::setRoot(const giac::gen *root) {
  root_ = root;
  reset();
}

bool AstCursor::move(Direction dir) {
  if (!root_)
    return false;

  switch (dir) {
  case Direction::RIGHT: {
    // Move to next sibling at current level
    if (path_.empty()) {
      // At root, no siblings
      return false;
    }

    size_t parent_child_count = 1;
    const giac::gen *parent_node = nullptr;

    if (path_.depth() == 1) {
      // Parent is root
      parent_node = root_;
    } else {
      // Get parent node
      CursorPath parent_path = path_;
      parent_path.pop();
      parent_node = getNodeAtPath(parent_path);
    }

    if (parent_node) {
      parent_child_count = getChildCount(parent_node);
    }

    // Check if we can increment current index
    if (path_.indices.back() + 1 < parent_child_count) {
      path_.indices.back()++;
      return validatePath();
    }
    return false;
  }

  case Direction::LEFT: {
    // Move to previous sibling or go up to parent
    if (path_.empty()) {
      return false;
    }

    if (path_.indices.back() > 0) {
      // Move to previous sibling
      path_.indices.back()--;
      return validatePath();
    } else if (path_.depth() > 1) {
      // Go up to parent (which becomes the selected node)
      path_.pop();
      return validatePath();
    }
    return false;
  }

  case Direction::DOWN: {
    // Navigate into first child
    const giac::gen *current = getCurrentNode();
    if (!current)
      return false;

    size_t child_count = getChildCount(current);
    if (child_count > 0) {
      path_.push(0); // Select first child
      return validatePath();
    }
    return false;
  }

  case Direction::UP: {
    // Navigate to parent or move within current node's children
    if (path_.empty()) {
      return false;
    }

    // If we're not at first child, move to first child (index 0)
    // Otherwise, go up to parent
    if (path_.indices.back() > 0) {
      path_.indices.back() = 0;
      return validatePath();
    } else if (path_.depth() > 1) {
      path_.pop();
      return validatePath();
    }
    return false;
  }

  default:
    return false;
  }
}

CursorState AstCursor::getState() const {
  const giac::gen *node = getCurrentNode();
  return CursorState(path_, node);
}

const giac::gen *AstCursor::getCurrentNode() const {
  return getNodeAtPath(path_);
}

bool AstCursor::isAtLeaf() const {
  const giac::gen *node = getCurrentNode();
  return !node || getChildCount(node) == 0;
}

void AstCursor::reset() {
  path_.clear();
}

bool AstCursor::setPath(const CursorPath &path) {
  if (getNodeAtPath(path)) {
    path_ = path;
    return true;
  }
  return false;
}

const giac::gen *AstCursor::getNodeAtPath(const CursorPath &path) const {
  if (!root_ || path.empty())
    return root_;

  const giac::gen *current = root_;
  for (size_t i = 0; i < path.indices.size(); ++i) {
    if (!current || current->type != giac::_SYMB)
      return nullptr;

    const giac::gen &args = current->_SYMBptr->feuille;
    if (args.type == giac::_VECT && path.indices[i] < args._VECTptr->size()) {
      current = &(*args._VECTptr)[path.indices[i]];
    } else if (path.indices[i] == 0) {
      // Single child case (non-VECT feuille)
      current = &args;
    } else {
      return nullptr; // Invalid index
    }
  }
  return current;
}

size_t AstCursor::getChildCount(const giac::gen *node) const {
  if (!node || node->type != giac::_SYMB)
    return 0;

  const giac::gen &args = node->_SYMBptr->feuille;
  if (args.type == giac::_VECT) {
    return args._VECTptr->size();
  }
  // Single child (feuille is not a vector)
  return 1;
}

bool AstCursor::validatePath() const {
  return getNodeAtPath(path_) != nullptr;
}

} // namespace MathRenderer
