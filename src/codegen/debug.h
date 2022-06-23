#pragma once
#include <string>
#include <functional>
#include "pochivm.h"
struct indent_guard {
  inline static int indent = 0;
  int original_indent;
  static void reset() {
    indent = 0;
  }
  indent_guard() : original_indent(indent++) {}
  ~indent_guard() { indent = original_indent; }
};
void debugPrint(std::string s, bool newline = true);
std::function<void(const PochiVM::AstNodeBase *stmt)> pochiEmitter(std::stringstream &ss);
std::string pochiToStr(const PochiVM::AstNodeBase *stmt);