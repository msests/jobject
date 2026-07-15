#include "Utils.h"

#include "Accessor.h"

#include <cctype>
#include <string>
#include <vector>

namespace jobject {

namespace {

std::vector<std::string> parseExpressionTokens(const std::string &expression) {
  std::vector<std::string> tokens;
  std::string currentToken;
  bool inBrackets = false;
  bool inQuotes = false;

  for (size_t i = 0; i < expression.length(); ++i) {
    const char ch = expression[i];

    if (ch == '"' && !inBrackets) {
      inQuotes = !inQuotes;
      continue;
    }

    if (inQuotes) {
      currentToken += ch;
      continue;
    }

    if (ch == '[' && !inBrackets) {
      if (!currentToken.empty()) {
        tokens.push_back(currentToken);
        currentToken.clear();
      }
      inBrackets = true;
      continue;
    }

    if (ch == ']' && inBrackets) {
      if (!currentToken.empty()) {
        tokens.push_back(currentToken);
        currentToken.clear();
      }
      inBrackets = false;
      continue;
    }

    if (ch == '.' && !inBrackets) {
      if (!currentToken.empty()) {
        tokens.push_back(currentToken);
        currentToken.clear();
      }
      continue;
    }

    if (std::isspace(static_cast<unsigned char>(ch))) {
      continue;
    }

    currentToken += ch;
  }

  if (!currentToken.empty()) {
    tokens.push_back(currentToken);
  }

  return tokens;
}

} // namespace

namespace utils {

jvalue evalValue(jvalue value, const std::string &expr) {
  const auto tokens = parseExpressionTokens(expr);
  jvalue current = value;

  for (const auto &token : tokens) {
    if (current.isNullish() || current.isUndefined()) {
      return jvalue(JUndefined{});
    }

    const auto &currentValue = current.getValue();
    const auto currentType = getValueType(currentValue);
    if (currentType == ValueType::Array) {
      bool isIndexToken = !token.empty();
      for (const auto ch : token) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
          isIndexToken = false;
          break;
        }
      }

      if (isIndexToken) {
        try {
          const auto index = static_cast<size_t>(std::stoull(token));
          current = current[index];
        } catch (const std::exception &) {
          return jvalue(JUndefined{});
        }
      } else {
        current = current[token];
      }
    } else if (currentType == ValueType::Object ||
               currentType == ValueType::String ||
               currentType == ValueType::Function ||
               currentType == ValueType::Date) {
      current = current[token];
    } else {
      return jvalue(JUndefined{});
    }
  }

  return current;
}

} // namespace utils

} // namespace jobject
