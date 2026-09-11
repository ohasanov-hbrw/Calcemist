#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <giac.h>
#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>
#include "giac_cursor.hpp"

struct StackNode {
    giac::gen node;
    int depth;
};



struct MathFonts {
  const lgfx::IFont* main;
  const lgfx::IFont* exponent;
  const lgfx::IFont* nested_exponent;

  const lgfx::IFont* get(int32_t level) const {
    if (level == 0) return main;
    if (level == 1) return exponent;
    return nested_exponent;
  }
};


void print_giac_ast_iterative(const giac::gen& root);

void render_giac_ast(lgfx::LGFXBase *display, MathFonts fontsinput,
                     const giac::gen &root, const MathRenderer::AstCursor &cursor,
                     int32_t x, int32_t y);

giac::gen convert_inv_to_div(const giac::gen& node);
