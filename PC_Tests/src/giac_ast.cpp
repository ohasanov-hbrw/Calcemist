#include "giac_ast.hpp"
#include "vecteur.h"
#include <giac.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace {
struct MathLayout {
  int32_t width;
  int32_t height;
  int32_t axis;
};

constexpr int32_t kMathGap = 4;
constexpr int32_t kFractionGap = 3;
constexpr int32_t kMultiplicationWidth = 7;

std::string math_text(const giac::gen &node) {
  return node.print(giac::context0);
}

MathLayout math_layout(const giac::gen &node, lgfx::LGFXBase *display) {
  const int32_t line_height = display->fontHeight();

  if (node.type != giac::_SYMB) {
    return {display->textWidth(math_text(node).c_str()), line_height,
            line_height / 2};
  }

  const giac::symbolic *symbolic = node._SYMBptr;
  const giac::gen &args = symbolic->feuille;

  if (symbolic->sommet == giac::at_pow && args.type == giac::_VECT &&
      args._VECTptr->size() == 2) {
    MathLayout base = math_layout((*args._VECTptr)[0], display);
    MathLayout exponent = math_layout((*args._VECTptr)[1], display);
    int32_t base_y = exponent.height / 2;
    return {base.width + exponent.width,
            std::max(exponent.height, base_y + base.height),
            base_y + base.axis};
  }

  if (symbolic->sommet == giac::at_plus ||
      symbolic->sommet == giac::at_prod) {
    int32_t width = 0;
    int32_t above_axis = line_height / 2;
    int32_t below_axis = line_height - above_axis;
    const giac::vecteur *terms = args.type == giac::_VECT
                                     ? args._VECTptr
                                     : nullptr;
    if (terms) {
      for (size_t i = 0; i < terms->size(); ++i) {
        MathLayout child = math_layout((*terms)[i], display);
        int32_t separator_width =
            symbolic->sommet == giac::at_plus
                ? display->textWidth("+") + kMathGap
                : kMultiplicationWidth;
        width += child.width + (i ? separator_width : 0);
        above_axis = std::max(above_axis, child.axis);
        below_axis = std::max(below_axis, child.height - child.axis);
      }
      return {width, above_axis + below_axis, above_axis};
    }
  }

  std::string op = symbolic->sommet.ptr()->print(giac::context0);
  if (op == "/" && args.type == giac::_VECT && args._VECTptr->size() == 2) {
    MathLayout numerator = math_layout((*args._VECTptr)[0], display);
    MathLayout denominator = math_layout((*args._VECTptr)[1], display);
    int32_t axis = numerator.height + kFractionGap;
    return {std::max(numerator.width, denominator.width) + 6,
            axis + 1 + kFractionGap + denominator.height, axis};
  }

  MathLayout child = math_layout(args, display);
  int32_t op_width = display->textWidth(op.c_str());
  int32_t text_axis = line_height / 2;
  int32_t axis = std::max(text_axis, child.axis);
  int32_t below_axis =
      std::max(line_height - text_axis, child.height - child.axis);
  return {op_width + child.width + display->textWidth("()"),
          axis + below_axis, axis};
}

void draw_math(lgfx::LGFXBase *display, const giac::gen &node, int32_t x,
               int32_t y) {
  const int32_t line_height = display->fontHeight();
  const MathLayout layout = math_layout(node, display);

  if (node.type != giac::_SYMB) {
    std::string text = math_text(node);
    display->drawString(text.c_str(), x, y);
    return;
  }

  const giac::symbolic *symbolic = node._SYMBptr;
  const giac::gen &args = symbolic->feuille;

  if (symbolic->sommet == giac::at_pow && args.type == giac::_VECT &&
      args._VECTptr->size() == 2) {
    const giac::gen &base = (*args._VECTptr)[0];
    const giac::gen &exponent = (*args._VECTptr)[1];
    MathLayout base_layout = math_layout(base, display);
    draw_math(display, base, x, y + layout.axis - base_layout.axis);
    draw_math(display, exponent,
              x + math_layout(base, display).width,
              y);
    return;
  }

  if (symbolic->sommet == giac::at_plus ||
      symbolic->sommet == giac::at_prod) {
    if (args.type == giac::_VECT) {
      int32_t cursor = x;
      for (size_t i = 0; i < args._VECTptr->size(); ++i) {
        const giac::gen &term = (*args._VECTptr)[i];
        MathLayout child = math_layout(term, display);
        if (i) {
          if (symbolic->sommet == giac::at_plus) {
            display->drawString("+", cursor,
                                y + layout.axis - line_height / 2);
            cursor += display->textWidth("+") + kMathGap;
          } else {
            display->fillCircle(cursor + kMultiplicationWidth / 2,
                                y + layout.axis, 1, TFT_WHITE);
            cursor += kMultiplicationWidth;
          }
        }
        draw_math(display, term, cursor,
                  y + layout.axis - child.axis);
        cursor += child.width;
      }
      return;
    }
  }

  std::string op = symbolic->sommet.ptr()->print(giac::context0);
  if (op == "/" && args.type == giac::_VECT && args._VECTptr->size() == 2) {
    MathLayout numerator = math_layout((*args._VECTptr)[0], display);
    MathLayout denominator = math_layout((*args._VECTptr)[1], display);
    int32_t fraction_width = std::max(numerator.width, denominator.width) + 6;
    draw_math(display, (*args._VECTptr)[0],
              x + (fraction_width - numerator.width) / 2, y);
    int32_t line_y = y + numerator.height + kFractionGap;
    display->drawLine(x, line_y, x + fraction_width - 1, line_y);
    draw_math(display, (*args._VECTptr)[1],
              x + (fraction_width - denominator.width) / 2,
              line_y + 1 + kFractionGap);
    return;
  }

  MathLayout child = math_layout(args, display);
  int32_t text_y = y + layout.axis - line_height / 2;
  int32_t child_x = x + display->textWidth(op.c_str());
  display->drawString(op.c_str(), x, text_y);
  display->drawString("(", child_x, text_y);
  child_x += display->textWidth("(");
  draw_math(display, args, child_x,
            y + layout.axis - child.axis);
  display->drawString(")", child_x + child.width, text_y);
}
} // namespace

void render_giac_ast(lgfx::LGFXBase *display, const giac::gen &root,
                     int32_t x, int32_t y) {
  if (!display)
    return;
  display->setTextColor(TFT_WHITE);
  display->setTextDatum(lgfx::textdatum_t::top_left);
  draw_math(display, root, x, y);
}

void print_giac_ast_iterative(const giac::gen &root) {
  std::vector<StackNode> stack;
  stack.push_back({root, 0});

  while (!stack.empty()) {
    StackNode current = stack.back();
    stack.pop_back();

    const giac::gen &node = current.node;
    int depth = current.depth;
    std::string indent(depth * 2, ' ');

    switch (node.type) {
    case giac::_INT_:
      std::cout << indent << "[INT] " << node.val << "\n";
      break;

    case giac::_DOUBLE_:
      std::cout << indent << "[REAL] " << node._DOUBLE_val << "\n";
      break;

    case giac::_IDNT:
      std::cout << indent << "[VAR] " << node.print(giac::context0) << "\n";
      break;

    case giac::_FRAC: {
      // node._FRACptr points to a giac::fraction struct
      std::cout << indent << "[FRAC] " << node._FRACptr->num << "/"
                << node._FRACptr->den << "\n";
      break;
    }

    case giac::_SYMB: {
      const giac::symbolic *symb = node._SYMBptr;
      std::string op = symb->sommet.ptr()->print(giac::context0);

      // --- SPECIAL CASE: ADDITION / MULTIPLICATION ---
      if (symb->sommet == giac::at_plus || symb->sommet == giac::at_prod) {
        std::cout << indent << "[N-ARY "
                  << (symb->sommet == giac::at_plus ? "SUM" : "PRODUCT")
                  << "]\n";

        if (symb->feuille.type == giac::_VECT) {
          const giac::vecteur *vec = symb->feuille._VECTptr;
          // Push in reverse so the first term prints first
          for (auto it = vec->rbegin(); it != vec->rend(); ++it) {
            stack.push_back({*it, depth + 1});
          }
        } else {
          // Single-argument edge case
          stack.push_back({symb->feuille, depth + 1});
        }
        break;
      } else if (symb->sommet == giac::at_pow) {
        std::cout << indent << "[N-ARY " << "POW"
                  << "]\n";

        if (symb->feuille.type == giac::_VECT) {
          const giac::vecteur *vec = symb->feuille._VECTptr;
          // Push in reverse so the first term prints first
          for (auto it = vec->rbegin(); it != vec->rend(); ++it) {
            stack.push_back({*it, depth + 1});
          }
        } else {
          // Single-argument edge case
          stack.push_back({symb->feuille, depth + 1});
        }
        break;
      } else if (op == "/") {
        std::cout << indent << "[N-ARY " << "DIVISION"
                  << "]\n";

        if (symb->feuille.type == giac::_VECT) {
          const giac::vecteur *vec = symb->feuille._VECTptr;
          // Push in reverse so the first term prints first
          for (auto it = vec->rbegin(); it != vec->rend(); ++it) {
            stack.push_back({*it, depth + 1});
          }
        } else {
          // Single-argument edge case
          stack.push_back({symb->feuille, depth + 1});
        }
        break;
      }
      // sum(1,2,3,-4,5) is AN output, not a good one, but still

      // --- DEFAULT SYMB CASE (Functions like sin, cos, pow) ---
      std::cout << indent << "[FUNC] " << op << "\n";
      stack.push_back({symb->feuille, depth + 1});
      break;
    }

    case giac::_VECT: {
      const giac::vecteur *vec = node._VECTptr;
      std::cout << indent << "[LIST/ARRAY] size=" << vec->size() << "\n";
      for (auto it = vec->rbegin(); it != vec->rend(); ++it) {
        stack.push_back({*it, depth + 1});
      }
      break;
    }

    default:
      std::cout << indent << "[NODE] " << node.print(giac::context0) << "\n";
      break;
    }
  }
}

giac::gen convert_inv_to_div(const giac::gen &node) {
  // 1. Convert rational fractions (_FRAC) like 1/2 directly into division
  if (node.type == giac::_FRAC) {
    giac::gen num = node._FRACptr->num;
    giac::gen den = node._FRACptr->den;

    giac::context context_obj;
    giac::gen div_template = giac::gen("A/B", &context_obj);
    giac::unary_function_ptr div_op = div_template._SYMBptr->sommet;

    return giac::symbolic(div_op, giac::makevecteur(num, den));
  }

  // Handle vectors
  if (node.type != giac::_SYMB) {
    if (node.type == giac::_VECT) {
      giac::vecteur new_vec;
      for (const auto &child : *node._VECTptr) {
        new_vec.push_back(convert_inv_to_div(child));
      }
      return new_vec;
    }
    return node;
  }

  const giac::symbolic *symb = node._SYMBptr;
  giac::gen op = symb->sommet;
  giac::gen args = convert_inv_to_div(symb->feuille);

  // Extract GIAC's exact division operator template
  giac::context context_obj;
  giac::gen div_template = giac::gen("A/B", &context_obj);
  giac::unary_function_ptr div_op = div_template._SYMBptr->sommet;

  // 2. Convert standalone inverse nodes: inv(X) -> 1 / X
  if (op == giac::at_inv) {
    return giac::symbolic(div_op, giac::makevecteur(giac::gen(1), args));
  }

  // 3. Look for multiplication: at_prod([...])
  if (op == giac::at_prod && args.type == giac::_VECT) {
    giac::vecteur nums;
    giac::vecteur dens;

    for (const auto &term : *args._VECTptr) {
      // Case A: inv(...) node
      if (term.type == giac::_SYMB && term._SYMBptr->sommet == giac::at_inv) {
        dens.push_back(term._SYMBptr->feuille);
      }
      // Case B: pow(term, -1)
      else if (term.type == giac::_SYMB &&
               term._SYMBptr->sommet == giac::at_pow &&
               term._SYMBptr->feuille.type == giac::_VECT &&
               term._SYMBptr->feuille._VECTptr->size() == 2 &&
               (*term._SYMBptr->feuille._VECTptr)[1] == giac::gen(-1)) {
        dens.push_back((*term._SYMBptr->feuille._VECTptr)[0]);
      }
      // Case C: Explicit division node like (1 / D) or (N / D)
      else if (term.type == giac::_SYMB && term._SYMBptr->sommet == div_op &&
               term._SYMBptr->feuille.type == giac::_VECT &&
               term._SYMBptr->feuille._VECTptr->size() == 2) {
        giac::gen div_num = (*term._SYMBptr->feuille._VECTptr)[0];
        giac::gen div_den = (*term._SYMBptr->feuille._VECTptr)[1];

        // If numerator is not 1, keep it in the product numerator
        if (div_num != giac::gen(1)) {
          nums.push_back(div_num);
        }
        dens.push_back(div_den);
      }
      // Regular factor
      else {
        nums.push_back(term);
      }
    }

    // If we found any denominators, merge into a single fraction: (nums...) / (dens...)
    if (!dens.empty()) {
      giac::gen numerator =
          nums.empty() ? giac::gen(1)
                       : ((nums.size() == 1) ? nums[0] : giac::symbolic(giac::at_prod, nums));

      giac::gen denominator =
          (dens.size() == 1) ? dens[0] : giac::symbolic(giac::at_prod, dens);

      return giac::symbolic(div_op, giac::makevecteur(numerator, denominator));
    }
  }

  // Default: reconstruct current symbolic node with transformed arguments
  return giac::symbolic(symb->sommet, args);
}
