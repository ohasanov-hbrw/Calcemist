#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <giac.h>
#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

struct StackNode {
    giac::gen node;
    int depth;
};

void print_giac_ast_iterative(const giac::gen& root);

void render_giac_ast(lgfx::LGFXBase *display, const giac::gen &root,
                     int32_t x, int32_t y);

giac::gen convert_inv_to_div(const giac::gen& node);
