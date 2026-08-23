#include "mathio_renderer.h"
#include <algorithm>
#include <cstring>
#include <Arduino.h>   // for Serial

namespace MathIO {

MathRenderer::MathRenderer(U8G2* display)
    : u8g2(display),
      mainFont(u8g2_font_6x10_tf),
      subFont(u8g2_font_4x6_tf) {}

void MathRenderer::setArea(int x, int y, int width, int height) {
    areaX = x; areaY = y; areaW = width; areaH = height;
}

void MathRenderer::render(Node* root) {
    if (!root) return;

    // 1. Compute sizes
    computeSize(root);

    // 2. Position relative to (0,0)
    computePosition(root, 0, 0);

    // 3. Overall bounding box
    int totalW = root->w;
    int totalH = root->h;

    // 4. Center in the area
    int offsetX = areaX + (areaW - totalW) / 2;
    int offsetY = areaY + (areaH - totalH) / 2;

    u8g2->setDrawColor(1);
    u8g2->setFontMode(1); // transparent background
    u8g2->setFont(mainFont);

    // 5. Draw
    drawNode(root, offsetX, offsetY);
}

MathRenderer::Size MathRenderer::getLeafSize(const char* token, const uint8_t* font) {
    if (!token || token[0] == '\0') return {0, 0, 0};
    u8g2->setFont(font);
    int w = u8g2->getStrWidth(token);
    int ascent = u8g2->getFontAscent();
    int descent = u8g2->getFontDescent();
    int h = ascent + descent;
    return {w, h, ascent};
}

MathRenderer::Size MathRenderer::computeSize(Node* node) {
    if (!node) return {0, 0, 0};

    Size result{0, 0, 0};

    switch (node->type) {
        case NodeType::NUMBER:
        case NodeType::VARIABLE:
        case NodeType::SYMBOL:
        case NodeType::OPERATOR: {
            Size s = getLeafSize(node->token, mainFont);
            node->w = s.w; node->h = s.h; node->baseline = s.baseline;
            return s;
        }

        case NodeType::ROW: {
            int totalW = 0, maxH = 0, maxBaseline = 0;
            Node* child = node->children[0];
            while (child) {
                Size cs = computeSize(child);
                totalW += cs.w;
                maxH = std::max(maxH, cs.h);
                maxBaseline = std::max(maxBaseline, cs.baseline);
                child = child->next;
            }
            if (totalW == 0) {
                maxH = 10;          // fallback
                maxBaseline = 8;
            }
            node->w = totalW;
            node->h = maxH;
            node->baseline = maxBaseline;
            return {totalW, maxH, maxBaseline};
        }

        case NodeType::FRACTION: {
            if (!node->children[0] || !node->children[1]) {
                node->w = 0; node->h = 0; node->baseline = 0;
                return {0, 0, 0};
            }
            Size num = computeSize(node->children[0]);
            Size den = computeSize(node->children[1]);
            int barLen = std::max(num.w, den.w) + 4;
            int totalH = num.h + 2 + 1 + 2 + den.h;
            int baseline = num.h + 2;
            node->w = barLen;
            node->h = totalH;
            node->baseline = baseline;
            return {barLen, totalH, baseline};
        }

        case NodeType::POWER: {
            if (!node->children[0] || !node->children[1]) {
                node->w = 0; node->h = 0; node->baseline = 0;
                return {0, 0, 0};
            }
            Size base = computeSize(node->children[0]);
            Size exp  = computeSize(node->children[1]);
            int expY = base.baseline - exp.h - 2;
            int totalH = std::max(base.h, expY + exp.h) - std::min(0, expY);
            int totalW = base.w + exp.w;
            int baseline = base.baseline;
            node->w = totalW;
            node->h = totalH;
            node->baseline = baseline;
            return {totalW, totalH, baseline};
        }

        case NodeType::FUNCTION: {
            if (!node->children[0]) {
                node->w = 0; node->h = 0; node->baseline = 0;
                return {0, 0, 0};
            }
            Size arg = computeSize(node->children[0]);
            Size name = getLeafSize(node->token, mainFont);
            int totalW = name.w + 1 + arg.w + 1;
            int totalH = std::max(name.h, arg.h);
            int baseline = std::max(name.baseline, arg.baseline);
            node->w = totalW;
            node->h = totalH;
            node->baseline = baseline;
            return {totalW, totalH, baseline};
        }

        case NodeType::PARENTHESIS: {
            if (!node->children[0]) {
                node->w = 0; node->h = 0; node->baseline = 0;
                return {0, 0, 0};
            }
            Size inner = computeSize(node->children[0]);
            int totalW = inner.w + 2;
            int totalH = std::max(inner.h, 12);
            int baseline = inner.baseline;
            node->w = totalW;
            node->h = totalH;
            node->baseline = baseline;
            return {totalW, totalH, baseline};
        }

        default: {
            if (strlen(node->token) > 0) {
                Size s = getLeafSize(node->token, mainFont);
                node->w = s.w; node->h = s.h; node->baseline = s.baseline;
                return s;
            }
            node->w = 0; node->h = 0; node->baseline = 0;
            return {0, 0, 0};
        }
    }
}

void MathRenderer::computePosition(Node* node, int x, int y) {
    if (!node) return;
    node->x = x;
    node->y = y;

    switch (node->type) {
        case NodeType::ROW: {
            int currentX = x;
            Node* child = node->children[0];
            while (child) {
                int childY = y + (node->baseline - child->baseline);
                computePosition(child, currentX, childY);
                currentX += child->w;
                child = child->next;
            }
            break;
        }

        case NodeType::FRACTION: {
            Node* num = node->children[0];
            Node* den = node->children[1];
            if (num && den) {
                int numX = x + (node->w - num->w) / 2;
                int numY = y;
                computePosition(num, numX, numY);

                int barY = y + num->h + 2;
                int denY = barY + 1 + 2;
                int denX = x + (node->w - den->w) / 2;
                computePosition(den, denX, denY);
            }
            break;
        }

        case NodeType::POWER: {
            Node* base = node->children[0];
            Node* exp  = node->children[1];
            if (base && exp) {
                int baseX = x;
                int baseY = y + (node->baseline - base->baseline);
                computePosition(base, baseX, baseY);

                int expX = x + base->w;
                int expY = y + (node->baseline - exp->baseline) - exp->h - 2;
                computePosition(exp, expX, expY);
            }
            break;
        }

        case NodeType::FUNCTION: {
            Node* arg = node->children[0];
            if (arg) {
                Size nameSize = getLeafSize(node->token, mainFont);
                int argX = x + nameSize.w + 1;
                int argY = y + (node->h - arg->h) / 2;
                computePosition(arg, argX, argY);
            }
            break;
        }

        case NodeType::PARENTHESIS: {
            Node* inner = node->children[0];
            if (inner) {
                int innerX = x + 1;
                int innerY = y + (node->h - inner->h) / 2;
                computePosition(inner, innerX, innerY);
            }
            break;
        }

        default:
            break;
    }
}

void MathRenderer::drawLeaf(Node* node, int x, int y, const uint8_t* font) {
    if (!node || node->token[0] == '\0') return;
    Serial.printf("drawLeaf: '%s' at (%d,%d)\n", node->token, x, y);
    u8g2->setFont(font);
    u8g2->drawStr(x, y + node->baseline, node->token);
}

void MathRenderer::drawFraction(Node* node, int x, int y) {
    if (!node->children[0] || !node->children[1]) return;

    // Draw numerator and denominator with the parent's offset
    drawNode(node->children[0], x, y);
    drawNode(node->children[1], x, y);

    // Draw fraction bar
    int barY = y + node->children[0]->h + 2;
    u8g2->drawHLine(x, barY, node->w);
}

void MathRenderer::drawPower(Node* node, int x, int y) {
    if (node->children[0]) drawNode(node->children[0], x, y);
    if (node->children[1]) drawNode(node->children[1], x, y);
}

void MathRenderer::drawFunction(Node* node, int x, int y) {
    if (!node->children[0]) return;

    u8g2->setFont(mainFont);
    int nameW = u8g2->getStrWidth(node->token);
    int baselineY = y + node->baseline;

    u8g2->drawStr(x, baselineY, node->token);
    u8g2->drawStr(x + nameW, baselineY, "(");
    drawNode(node->children[0], x, y);   // pass the parent offset
    int innerW = node->children[0]->w;
    u8g2->drawStr(x + nameW + 1 + innerW, baselineY, ")");
}

void MathRenderer::drawParenthesis(Node* node, int x, int y) {
    if (!node->children[0]) return;

    u8g2->setFont(mainFont);
    int baselineY = y + node->baseline;
    u8g2->drawStr(x, baselineY, "(");
    drawNode(node->children[0], x, y);   // pass the parent offset
    int innerW = node->children[0]->w;
    u8g2->drawStr(x + 1 + innerW, baselineY, ")");
}

void MathRenderer::drawNode(Node* node, int offsetX, int offsetY) {
    if (!node) return;

    int x = node->x + offsetX;
    int y = node->y + offsetY;

    switch (node->type) {
        case NodeType::NUMBER:
        case NodeType::VARIABLE:
        case NodeType::SYMBOL:
        case NodeType::OPERATOR:
            drawLeaf(node, x, y, mainFont);
            break;

        case NodeType::ROW: {
            Node* child = node->children[0];
            while (child) {
                drawNode(child, offsetX, offsetY);
                child = child->next;
            }
            break;
        }

        case NodeType::FRACTION:
            drawFraction(node, x, y);
            break;

        case NodeType::POWER:
            drawPower(node, x, y);
            break;

        case NodeType::FUNCTION:
            drawFunction(node, x, y);
            break;

        case NodeType::PARENTHESIS:
            drawParenthesis(node, x, y);
            break;

        default:
            if (strlen(node->token) > 0) {
                u8g2->setFont(mainFont);
                u8g2->drawStr(x, y + node->baseline, node->token);
            }
            break;
    }
}

} // namespace MathIO