#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_NOPARTIALARGUMENTWRAPPINGCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_NOPARTIALARGUMENTWRAPPINGCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::readability {

/// Checks for function definitions where arguments are partially wrapped to
/// multiple lines.
///
/// It enforces a style where arguments are either all on the same line as the
/// function's opening parenthesis and closing parenthesis, or the first
/// argument is on a new line after the opening parenthesis, and the closing
/// parenthesis is also on its own new line after the last argument.
///
/// Example of **incorrect** formatting (partial wrapping):
/// \code
///   int f(int x, int y, int z,
///   int w);
///
///   void g(std::string s1,
///          std::string s2, int val) {
///     // ...
///   }
/// \endcode
///
/// Examples of **correct** formatting:
/// \code
///   int f(int x, int y, int z, int w); // All on one line
///
///   int f(                             // Opening parenthesis with function name
///       int x, int y, int z,          // Arguments each on new lines
///       int w
///   );                                // Closing parenthesis on a new line
///
///   // OR
///
///   int f
///   (                                 // Opening parenthesis on its own new line
///       int x, int y, int z,
///       int w
///   );                                // Closing parenthesis on a new line
/// \endcode
class NoPartialArgumentWrappingCheck : public ClangTidyCheck {
public:
  NoPartialArgumentWrappingCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return true;
  }
};

} // namespace clang::tidy::readability

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_NOPARTIALARGUMENTWRAPPINGCHECK_H
