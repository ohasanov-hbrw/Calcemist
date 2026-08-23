#include "mathio_ast_printer.h"
#include <iostream>
#include <vector>
#include <algorithm>

namespace MathIO {

// -----------------------------------------------------------------------------
// AST Structural Tree Inspector
// -----------------------------------------------------------------------------

void printAST(Node* node, const Cursor* cursor, std::string indent, bool isLast, const std::string& label) {
    if (!node) return;

    std::cout << indent << (isLast ? "└── " : "├── ");
    if (!label.empty()) {
        std::cout << label << ": ";
    }

    switch (node->type) {
        case NodeType::ROW:         std::cout << "[ROW]"; break;
        case NodeType::FRACTION:    std::cout << "[FRACTION]"; break;
        case NodeType::POWER:       std::cout << "[POWER]"; break;
        case NodeType::SUBSCRIPT:   std::cout << "[SUBSCRIPT]"; break;
        case NodeType::ROOT:        std::cout << "[ROOT]"; break;
        case NodeType::INTEGRAL:    std::cout << "[INTEGRAL]"; break;
        case NodeType::PARENTHESIS: std::cout << "[PARENTHESIS]"; break;
        case NodeType::NUMBER:      std::cout << "[NUMBER: \"" << node->token << "\"]"; break;
        case NodeType::VARIABLE:    std::cout << "[VARIABLE: \"" << node->token << "\"]"; break;
        case NodeType::OPERATOR:    std::cout << "[OPERATOR: \"" << node->token << "\"]"; break;
        case NodeType::SYMBOL:      std::cout << "[SYMBOL: \"" << node->token << "\"]"; break;
        case NodeType::FUNCTION:    std::cout << "[FUNCTION: \"" << node->token << "\"]"; break;
        default:                    std::cout << "[NODE: type=" << static_cast<int>(node->type) << "]"; break;
    }

    if (cursor) {
        if (cursor->position == node) {
            std::cout << "  <-- Cursor";
        } else if (cursor->container == node && cursor->position == nullptr && node->children[0] == nullptr) {
            std::cout << "  <-- Cursor (empty ROW)";
        }
    }
    std::cout << "\n";

    std::string childIndent = indent + (isLast ? "    " : "│   ");

    if (node->type == NodeType::ROW) {
        Node* curr = node->children[0];
        while (curr) {
            bool lastSibling = (curr->next == nullptr);
            printAST(curr, cursor, childIndent, lastSibling);
            curr = curr->next;
        }
    } else {
        int childCount = 0;
        for (size_t i = 0; i < MAX_CHILDREN; ++i) {
            if (node->children[i]) childCount++;
        }

        int printed = 0;
        for (size_t i = 0; i < MAX_CHILDREN; ++i) {
            if (node->children[i]) {
                printed++;
                std::string childLabel = "";
                if (node->type == NodeType::FRACTION) {
                    childLabel = (i == 0) ? "Numerator" : "Denominator";
                } else if (node->type == NodeType::POWER) {
                    childLabel = (i == 0) ? "Base" : "Exponent";
                } else if (node->type == NodeType::SUBSCRIPT) {
                    childLabel = (i == 0) ? "Base" : "Subscript";
                } else if (node->type == NodeType::ROOT) {
                    childLabel = (i == 0) ? "Radicand" : "Degree";
                } else if (node->type == NodeType::INTEGRAL) {
                    const char* labels[] = {"Integrand", "Variable", "Lower Limit", "Upper Limit"};
                    childLabel = labels[i];
                }
                printAST(node->children[i], cursor, childIndent, printed == childCount, childLabel);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// 2D Monospace Math Canvas Printer
// -----------------------------------------------------------------------------

struct Canvas {
    int width = 0;
    int height = 0;
    int baseline = 0;
    std::vector<std::string> grid;

    Canvas() = default;
    Canvas(int w, int h, int b = 0) 
        : width(w), height(h), baseline(b), grid(h, std::string(w, ' ')) {}

    static Canvas fromString(const std::string& str) {
        Canvas c(str.length(), 1, 0);
        c.grid[0] = str;
        return c;
    }
};

void pasteCanvas(Canvas& target, const Canvas& src, int startX, int startY) {
    for (int r = 0; r < src.height; ++r) {
        int tr = startY + r;
        if (tr < 0 || tr >= target.height) continue;
        for (int c = 0; c < src.width; ++c) {
            int tc = startX + c;
            if (tc < 0 || tc >= target.width) continue;
            target.grid[tr][tc] = src.grid[r][c];
        }
    }
}

Canvas nodeToCanvas(Node* node) {
    if (!node) return Canvas(0, 1, 0);

    switch (node->type) {
        case NodeType::ROW: {
            std::vector<Canvas> childCanvases;
            Node* curr = node->children[0];
            while (curr) {
                childCanvases.push_back(nodeToCanvas(curr));
                curr = curr->next;
            }

            if (childCanvases.empty()) return Canvas(0, 1, 0);

            int maxAbove = 0, maxBelow = 0, totalWidth = 0;
            for (const auto& c : childCanvases) {
                maxAbove = std::max(maxAbove, c.baseline);
                maxBelow = std::max(maxBelow, c.height - 1 - c.baseline);
                totalWidth += c.width;
            }

            Canvas rowCanvas(totalWidth, maxAbove + maxBelow + 1, maxAbove);
            int currentX = 0;
            for (const auto& c : childCanvases) {
                pasteCanvas(rowCanvas, c, currentX, maxAbove - c.baseline);
                currentX += c.width;
            }
            return rowCanvas;
        }

        case NodeType::NUMBER:
        case NodeType::VARIABLE:
        case NodeType::SYMBOL:
            return Canvas::fromString(node->token);

        case NodeType::OPERATOR: {
            // Add spaces around binary operators (e.g. " + "), but keep unary operators tight (e.g. "-5")
            if (node->prev != nullptr) {
                return Canvas::fromString(" " + std::string(node->token) + " ");
            } else {
                return Canvas::fromString(node->token);
            }
        }

        case NodeType::FRACTION: {
            Canvas num = nodeToCanvas(node->children[0]);
            Canvas den = nodeToCanvas(node->children[1]);

            int barLen = std::max(num.width, den.width) + 2;
            int totalHeight = num.height + 1 + den.height;
            int fractionBaseline = num.height;

            Canvas fracCanvas(barLen, totalHeight, fractionBaseline);
            fracCanvas.grid[fractionBaseline] = std::string(barLen, '-');

            pasteCanvas(fracCanvas, num, (barLen - num.width) / 2, 0);
            pasteCanvas(fracCanvas, den, (barLen - den.width) / 2, num.height + 1);

            return fracCanvas;
        }

        case NodeType::POWER: {
            Canvas base = nodeToCanvas(node->children[0]);
            Canvas exp  = nodeToCanvas(node->children[1]);

            int totalWidth  = base.width + exp.width;
            int totalHeight = base.height + exp.height;
            int powerBaseline = exp.height + base.baseline;

            Canvas powCanvas(totalWidth, totalHeight, powerBaseline);
            pasteCanvas(powCanvas, exp, base.width, 0);
            pasteCanvas(powCanvas, base, 0, exp.height);

            return powCanvas;
        }

        case NodeType::ROOT: {
            Canvas rad = nodeToCanvas(node->children[0]);

            int totalWidth  = rad.width + 3;
            int totalHeight = rad.height + 1;
            int rootBaseline = rad.baseline + 1;

            Canvas rootCanvas(totalWidth, totalHeight, rootBaseline);

            for (int x = 2; x < totalWidth; ++x) {
                rootCanvas.grid[0][x] = '_';
            }

            rootCanvas.grid[1][0] = '\\';
            rootCanvas.grid[1][1] = '/';
            for (int y = 2; y < totalHeight; ++y) {
                rootCanvas.grid[y][1] = '|';
            }

            pasteCanvas(rootCanvas, rad, 2, 1);
            return rootCanvas;
        }

        case NodeType::PARENTHESIS: {
            Canvas inner = nodeToCanvas(node->children[0]);

            if (inner.height <= 1) {
                return Canvas::fromString("(" + (inner.grid.empty() ? "" : inner.grid[0]) + ")");
            }

            int totalWidth = inner.width + 2;
            Canvas parenCanvas(totalWidth, inner.height, inner.baseline);

            for (int y = 0; y < inner.height; ++y) {
                if (y == 0) {
                    parenCanvas.grid[y][0] = '/';
                    parenCanvas.grid[y][totalWidth - 1] = '\\';
                } else if (y == inner.height - 1) {
                    parenCanvas.grid[y][0] = '\\';
                    parenCanvas.grid[y][totalWidth - 1] = '/';
                } else {
                    parenCanvas.grid[y][0] = '|';
                    parenCanvas.grid[y][totalWidth - 1] = '|';
                }
            }

            pasteCanvas(parenCanvas, inner, 1, 0);
            return parenCanvas;
        }

        case NodeType::FUNCTION: {
            std::string name = node->token;
            Canvas arg = nodeToCanvas(node->children[0]);

            Canvas nameCanvas = Canvas::fromString(name + "(");
            
            Canvas funcCanvas(name.length() + 1 + arg.width + 1, arg.height, arg.baseline);
            pasteCanvas(funcCanvas, nameCanvas, 0, arg.baseline);
            pasteCanvas(funcCanvas, arg, name.length() + 1, 0);
            funcCanvas.grid[arg.baseline][funcCanvas.width - 1] = ')';

            return funcCanvas;
        }

        case NodeType::INTEGRAL: {
            Canvas integrand = nodeToCanvas(node->children[0]);
            Canvas var       = nodeToCanvas(node->children[1]);

            bool hasLower = (node->children[2] != nullptr);
            bool hasUpper = (node->children[3] != nullptr);

            Canvas lower = hasLower ? nodeToCanvas(node->children[2]) : Canvas(0, 0, 0);
            Canvas upper = hasUpper ? nodeToCanvas(node->children[3]) : Canvas(0, 0, 0);

            std::string dx = " d" + (var.grid.empty() ? "x" : var.grid[0]);
            Canvas dxCanvas = Canvas::fromString(dx);

            int symbolHeight = std::max(3, integrand.height);
            int limitWidth   = std::max(upper.width, lower.width);

            int symX   = (limitWidth > 0) ? limitWidth / 2 : 0;
            int upperX = (limitWidth > 0) ? (limitWidth - upper.width) / 2 : 0;
            int lowerX = (limitWidth > 0) ? (limitWidth - lower.width) / 2 : 0;

            int symbolY = upper.height;
            int lowerY  = symbolY + symbolHeight;

            int integrandX = std::max(symX + 2, limitWidth + 1);
            int integrandY = symbolY + (symbolHeight - integrand.height) / 2;

            int totalWidth  = integrandX + integrand.width + dxCanvas.width;
            int totalHeight = upper.height + symbolHeight + lower.height;
            int intBaseline = integrandY + integrand.baseline;

            Canvas intCanvas(totalWidth, totalHeight, intBaseline);

            // 1. Draw upper limit
            if (hasUpper) {
                pasteCanvas(intCanvas, upper, upperX, 0);
            }

            // 2. Draw 2D Integral symbol
            intCanvas.grid[symbolY][symX] = '/';
            for (int y = 1; y < symbolHeight - 1; ++y) {
                intCanvas.grid[symbolY + y][symX] = '|';
            }
            intCanvas.grid[symbolY + symbolHeight - 1][symX] = '/';

            // 3. Draw lower limit
            if (hasLower) {
                pasteCanvas(intCanvas, lower, lowerX, lowerY);
            }

            // 4. Draw integrand & dx
            pasteCanvas(intCanvas, integrand, integrandX, integrandY);
            pasteCanvas(intCanvas, dxCanvas, integrandX + integrand.width, integrandY + integrand.baseline);

            return intCanvas;
        }

        default:
            return Canvas::fromString(node->token);
    }
}

void printMathEquation(Node* root) {
    if (!root) {
        std::cout << "(empty)\n";
        return;
    }

    Canvas canvas = nodeToCanvas(root);

    std::cout << "\n";
    for (const auto& line : canvas.grid) {
        std::cout << "  " << line << "\n";
    }
    std::cout << "\n";
}

} // namespace MathIO