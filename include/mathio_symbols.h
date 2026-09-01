#pragma once

#include <stdint.h>

namespace MathIO {

// -----------------------------------------------------------------------------
// Special Character & Operator Symbol Constants
// -----------------------------------------------------------------------------
namespace Symbol {
    constexpr char PLUS[]        = "+";
    constexpr char MINUS[]       = "-";
    constexpr char MULTIPLY[]    = "*";  // Rendered as '×' or implied
    constexpr char DIVIDE[]      = "/";  // Rendered as '÷' or fraction
    constexpr char ASSIGN[]      = "=";
    
    // Greek / Constant Tokens
    constexpr char PI[]          = "pi";     // GIAC string rep
    constexpr char EULER[]       = "e";
    constexpr char THETA[]       = "theta";
    constexpr char INFINITY_TOK[]= "inf";

    // Supported Function Tokens
    constexpr char SIN[]         = "sin";
    constexpr char COS[]         = "cos";
    constexpr char TAN[]         = "tan";
    constexpr char ASIN[]        = "asin";
    constexpr char ACOS[]        = "acos";
    constexpr char ATAN[]        = "atan";
    constexpr char LN[]          = "ln";
    constexpr char LOG[]         = "log";
    constexpr char SQRT[]        = "sqrt";
    constexpr char ABS[]         = "abs";
}

// -----------------------------------------------------------------------------
// Keypad Command Codes for AST Insertion
// -----------------------------------------------------------------------------
enum class MathKey : uint16_t {
    NONE = 0,
    
    // Inputs
    DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4,
    DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, DIGIT_9,
    DOT, VAR_X, VAR_Y, VAR_Z,
    
    // 2D Layout Keys
    KEY_FRACTION,    // Creates FRACTION with 2 child ROWs
    KEY_POWER,       // Creates POWER with 1 child ROW for exponent
    KEY_SQRT,        // Creates ROOT with 1 child ROW for radicand
    KEY_ROOT_N,      // Creates ROOT with 2 child ROWs (radicand, degree)
    KEY_INTEGRAL,    // Creates INTEGRAL with 4 child ROWs
    KEY_PARENTHESIS, // Creates scalable PARENTHESIS with 1 child ROW
    
    // Navigation / Actions
    NAV_LEFT, NAV_RIGHT, NAV_UP, NAV_DOWN,
    ACTION_BACKSPACE, ACTION_CLEAR, ACTION_EXECUTE
};

} // namespace MathIO