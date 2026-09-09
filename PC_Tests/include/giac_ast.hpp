#include <iostream>
#include <string>
#include <vector>
#include <giac/giac.h>

struct StackNode {
    giac::gen node;
    int depth;
};

void print_giac_ast_iterative(const giac::gen& root);

giac::gen convert_inv_to_div(const giac::gen& node);