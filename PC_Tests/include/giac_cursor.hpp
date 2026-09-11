#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <giac.h>
#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>



enum class Direction { LEFT, RIGHT, UP, DOWN };

namespace MathRenderer {

/**
 * Represents a position in the AST using a path of child indices.
 * Each index indicates which child to select at each level of the tree.
 */
struct CursorPath {
  std::vector<size_t> indices;

  bool empty() const { return indices.empty(); }
  size_t depth() const { return indices.size(); }
  void clear() { indices.clear(); }
  void push(size_t idx) { indices.push_back(idx); }
  void pop() {
    if (!indices.empty())
      indices.pop_back();
  }

  bool operator==(const CursorPath &other) const {
    return indices == other.indices;
  }
  bool operator!=(const CursorPath &other) const {
    return !(*this == other);
  }
};

struct CursorState {
  CursorPath path;
  const giac::gen *node; // Non-owning pointer to current node
  bool is_valid;

  CursorState() : node(nullptr), is_valid(false) {}
  CursorState(const CursorPath &p, const giac::gen *n)
      : path(p), node(n), is_valid(n != nullptr) {}
};

class AstCursor {
public:
  AstCursor();
  void setRoot(const giac::gen *root);
  const giac::gen *getRoot() const { return root_; }
  bool move(Direction dir);
  CursorState getState() const;
  const giac::gen *getCurrentNode() const;
  const CursorPath &getPath() const { return path_; }
  bool isAtRoot() const { return path_.empty(); }
  bool isAtLeaf() const;
  void reset();
  bool setPath(const CursorPath &path);
private:
  const giac::gen *root_;
  CursorPath path_;
  const giac::gen *getNodeAtPath(const CursorPath &path) const;
  size_t getChildCount(const giac::gen *node) const;

  /**
   * Validate that current path points to existing node
   */
  bool validatePath() const;
};

} // namespace MathRenderer