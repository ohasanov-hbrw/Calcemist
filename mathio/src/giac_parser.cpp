#include "giac_parser.h"
#include "mathio_helpers.h"
#include <cctype>
#include <cstring>

namespace MathIO {

namespace {

class Parser {
public:
  explicit Parser(const std::string &src) : str(src), pos(0) {}

  Node *parse() {
    Node *row = NodePool::instance().alloc(NodeType::ROW);
    if (!row)
      return nullptr;

    parseExpression(row);
    return row;
  }

private:
  const std::string &str;
  size_t pos;

  void skipWhitespace() {
    while (pos < str.length() &&
           std::isspace(static_cast<unsigned char>(str[pos]))) {
      pos++;
    }
  }

  char peek() {
    skipWhitespace();
    return (pos < str.length()) ? str[pos] : '\0';
  }

  char get() {
    skipWhitespace();
    return (pos < str.length()) ? str[pos++] : '\0';
  }

  bool match(char expected) {
    if (peek() == expected) {
      get();
      return true;
    }
    return false;
  }

  // Handles + and - operators
  void parseExpression(Node *row) {
    skipWhitespace();

    // 1. Handle leading unary + or - (e.g., -5 in -5..5)
    if (peek() == '+' || peek() == '-') {
      char op = get();
      Node *opNode = NodePool::instance().alloc(NodeType::OPERATOR);
      if (opNode) {
        opNode->token[0] = op;
        opNode->token[1] = '\0';
        appendNode(row, opNode);
      }
    }

    // 2. Parse the primary term
    parseTerm(row);

    // 3. Parse subsequent binary addition/subtraction terms
    while (true) {
      skipWhitespace();
      char c = peek();
      if (c == '+' || c == '-') {
        get();
        Node *opNode = NodePool::instance().alloc(NodeType::OPERATOR);
        if (opNode) {
          opNode->token[0] = c;
          opNode->token[1] = '\0';
          appendNode(row, opNode);
        }
        parseTerm(row);
      } else {
        break;
      }
    }
  }

  // Handles * and / (Fractions)
  void parseTerm(Node *row) {
    parseFactor(row);

    while (true) {
      char c = peek();
      if (c == '*') {
        get();
        Node *opNode = NodePool::instance().alloc(NodeType::OPERATOR);
        if (opNode) {
          strncpy(opNode->token, "*", TOKEN_MAX_LEN);
          appendNode(row, opNode);
        }
        parseFactor(row);
      } else if (c == '/') {
        get();
        // Convert previous element or group into a FRACTION numerator
        Node *lastNode = row->children[0];
        if (lastNode) {
          while (lastNode->next)
            lastNode = lastNode->next;
        }

        Node *frac = createFraction();
        if (!frac)
          return;

        if (lastNode) {
          removeNode(row, lastNode, false);
          appendNode(frac->children[0], lastNode);
        }

        appendNode(row, frac);

        // Parse denominator into second child ROW
        parseFactor(frac->children[1]);
      } else {
        break;
      }
    }
  }

  // Handles ^ (Power)
  void parseFactor(Node *row) {
    parsePrimary(row);

    if (match('^')) {
      Node *lastNode = row->children[0];
      if (lastNode) {
        while (lastNode->next)
          lastNode = lastNode->next;
      }

      Node *powNode = createPower();
      if (!powNode)
        return;

      if (lastNode) {
        removeNode(row, lastNode, false);
        appendNode(powNode->children[0], lastNode);
      }

      appendNode(row, powNode);
      parseFactor(powNode->children[1]);
    }
  }

  // Handles Numbers, Variables, Functions, and Parentheses
  void parsePrimary(Node *row) {
    skipWhitespace();
    char c = peek();

    if (c == '(') {
      get(); // consume '('

      Node *parenRow = NodePool::instance().alloc(NodeType::ROW);
      if (!parenRow)
        return;

      parseExpression(parenRow);
      match(')');

      // Detach child chain from parenRow before freeing parenRow
      Node *child = parenRow->children[0];
      parenRow->children[0] = nullptr;

      while (child) {
        Node *next = child->next;
        child->prev = nullptr;
        child->next = nullptr;
        appendNode(row, child);
        child = next;
      }

      NodePool::instance().free(parenRow);
    } else if (std::isdigit(static_cast<unsigned char>(c))) {
      std::string num;
      // Only consume '.' if it is NOT part of a '..' range operator
      while (std::isdigit(static_cast<unsigned char>(peek())) ||
             (peek() == '.' && pos + 1 < str.length() && str[pos + 1] != '.')) {
        num += get();
      }
      Node *node = NodePool::instance().alloc(NodeType::NUMBER);
      if (node) {
        strncpy(node->token, num.c_str(), TOKEN_MAX_LEN - 1);
        appendNode(row, node);
      }
    } else if (std::isalpha(static_cast<unsigned char>(c))) {
      std::string ident;
      while (std::isalnum(static_cast<unsigned char>(peek()))) {
        ident += get();
      }

      if (ident == "sqrt" && peek() == '(') {
        get(); // consume '('
        Node *rootNode = createRoot(false);
        if (rootNode) {
          parseExpression(rootNode->children[0]);
          match(')');
          appendNode(row, rootNode);
        }
      } else if (ident == "integrate" && peek() == '(') {
        get(); // consume '('

        // Parse Argument 1: Integrand
        Node *arg1 = NodePool::instance().alloc(NodeType::ROW);
        parseExpression(arg1);

        Node *arg2 = nullptr;
        Node *arg3 = nullptr;
        Node *arg4 = nullptr;

        // Parse optional Argument 2: Variable of integration
        if (match(',')) {
          arg2 = NodePool::instance().alloc(NodeType::ROW);
          parseExpression(arg2);
        }

        // Parse optional Argument 3 & 4: Lower and Upper limits
        if (match(',')) {
          arg3 = NodePool::instance().alloc(NodeType::ROW);
          parseExpression(arg3);

          // Handle range separated by '..' or ','
          if (peek() == '.' && pos + 1 < str.length() && str[pos + 1] == '.') {
            pos += 2; // consume '..'
            arg4 = NodePool::instance().alloc(NodeType::ROW);
            parseExpression(arg4);
          } else if (match(',')) {
            arg4 = NodePool::instance().alloc(NodeType::ROW);
            parseExpression(arg4);
          }
        }

        match(')'); // consume closing ')'

        // Construct the INTEGRAL AST Node
        bool isDefinite = (arg3 != nullptr && arg4 != nullptr);
        Node *integNode = createIntegral(isDefinite);
        if (integNode) {
          // Swap allocated children with parsed argument rows
          NodePool::instance().free(integNode->children[0]);
          NodePool::instance().free(integNode->children[1]);
          integNode->children[0] = arg1;
          if (arg1)
            arg1->parent = integNode;
          integNode->children[1] = arg2;
          if (arg2)
            arg2->parent = integNode;

          if (isDefinite) {
            NodePool::instance().free(integNode->children[2]);
            NodePool::instance().free(integNode->children[3]);
            integNode->children[2] = arg3;
            if (arg3)
              arg3->parent = integNode;
            integNode->children[3] = arg4;
            if (arg4)
              arg4->parent = integNode;
          }

          appendNode(row, integNode);
        }
      } else if (peek() == '(') {
        get(); // consume '('

        Node *funcNode = NodePool::instance().alloc(NodeType::FUNCTION);
        if (funcNode) {
          strncpy(funcNode->token, ident.c_str(), TOKEN_MAX_LEN - 1);

          uint8_t childIndex = 0;
          while (peek() != ')' && peek() != '\0' && childIndex < MAX_CHILDREN) {
            Node *argRow = NodePool::instance().alloc(NodeType::ROW);
            parseExpression(argRow);

            funcNode->children[childIndex] = argRow;
            if (argRow)
              argRow->parent = funcNode;
            childIndex++;

            if (!match(','))
              break; // Stop if no comma follows
          }

          match(')'); // consume closing ')'
          appendNode(row, funcNode);
        }
      } else {
        Node *node = NodePool::instance().alloc(NodeType::VARIABLE);
        if (node) {
          strncpy(node->token, ident.c_str(), TOKEN_MAX_LEN - 1);
          appendNode(row, node);
        }
      }
    }
  }
};

} // anonymous namespace

Node *parseGIAC(const std::string &input) {
  Parser parser(input);
  return parser.parse();
}

} // namespace MathIO