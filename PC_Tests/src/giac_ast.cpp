#include "giac_ast.hpp"
#include "vecteur.h"
#include <giac/giac.h>
#include <iostream>
#include <string>
#include <vector>

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

#include <giac/giac.h>

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