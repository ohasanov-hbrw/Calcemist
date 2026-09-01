#pragma once

#include "mathio_node.h"
#include <U8g2lib.h>

namespace MathIO {

/**
 * Renders a MathIO AST as a pixel‑based mathematical expression using U8g2.
 * Supports fractions, powers, roots, integrals, functions, and parentheses.
 */
class MathRenderer {
public:
    explicit MathRenderer(U8G2* display);

    // Set the screen area where the expression will be drawn (centered).
    void setArea(int x, int y, int width, int height);

    // Render the AST rooted at `root` centered in the previously set area.
    void render(Node* root);

private:
    U8G2* u8g2;
    int areaX, areaY, areaW, areaH;

    // Fonts used for normal and sub/superscript text.
    const uint8_t* mainFont;   // e.g. u8g2_font_6x10_tf
    const uint8_t* subFont;    // e.g. u8g2_font_4x6_tf

    // Layout structure returned by computeSize().
    struct Size {
        int w;        // width in pixels
        int h;        // height in pixels
        int baseline; // distance from top to baseline
    };

    // Recursively compute sizes and store them in node->w, node->h, node->baseline.
    Size computeSize(Node* node);

    // Recursively position children relative to parent (top‑left origin).
    void computePosition(Node* node, int x, int y);

    // Recursively draw the node at its stored (x,y) position.
    void drawNode(Node* node, int offsetX, int offsetY);

    // Helper: get size for a leaf (number, variable, symbol) with a given font.
    Size getLeafSize(const char* token, const uint8_t* font);

    // Helper: draw a leaf string at baseline.
    void drawLeaf(Node* node, int x, int y, const uint8_t* font);

    // Draw primitives.
    void drawFraction(Node* node, int x, int y);
    void drawPower(Node* node, int x, int y);
    void drawRoot(Node* node, int x, int y);
    void drawIntegral(Node* node, int x, int y);
    void drawFunction(Node* node, int x, int y);
    void drawParenthesis(Node* node, int x, int y);
};

} // namespace MathIO