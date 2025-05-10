#include "NoPartialArgumentWrappingCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang::tidy::readability {

NoPartialArgumentWrappingCheck::NoPartialArgumentWrappingCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

void NoPartialArgumentWrappingCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(
          hasAnyParameter(parmVarDecl()),
          isDefinition(),
          unless(isImplicit())
      ).bind("func_decl"), this);
}

void NoPartialArgumentWrappingCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("func_decl");
  if (!FD) return;

  if (FD->isInvalidDecl() || FD->getBeginLoc().isInvalid())
      return;

  SourceManager &SM = *Result.SourceManager;
  const LangOptions &LO = getLangOpts();

  if (FD->getBeginLoc().isMacroID() || FD->getEndLoc().isMacroID())
      return;

  unsigned NumParams = FD->getNumParams();

  FunctionTypeLoc FuncTyLoc = FD->getFunctionTypeLoc();
  if (!FuncTyLoc) 
      return;

  FunctionProtoTypeLoc FPTL = FuncTyLoc.getAs<FunctionProtoTypeLoc>();
  if (FPTL.isNull())
      return;

  SourceLocation LParenLoc = FPTL.getLParenLoc();
  SourceLocation RParenLoc = FPTL.getRParenLoc();

  if (LParenLoc.isInvalid() || RParenLoc.isInvalid() ||
      LParenLoc.isMacroID() || RParenLoc.isMacroID())
    return;

  bool IsAnyPartOfParamsInMacro = false;
  for (unsigned i = 0; i < NumParams; ++i) {
      const ParmVarDecl* PVD = FD->getParamDecl(i);
      if (PVD->getSourceRange().getBegin().isMacroID() ||
          PVD->getSourceRange().getEnd().isMacroID()) {
          IsAnyPartOfParamsInMacro = true;
          break;
      }
  }
  
  if (FD->isVariadic()) {
    if (NumParams > 0) {
      const ParmVarDecl *LastParam = FD->getParamDecl(NumParams - 1);
      SourceLocation AfterLastParam = LastParam->getEndLoc().getLocWithOffset(1);
      
      if (AfterLastParam.isValid() && !AfterLastParam.isMacroID()) {
        CharSourceRange EllipsisRange = CharSourceRange::getCharRange(AfterLastParam, RParenLoc);
        StringRef EllipsisText = Lexer::getSourceText(EllipsisRange, SM, LO);
        
        if (EllipsisText.contains("...")) {
          SourceLocation EllipsisEnd = RParenLoc.getLocWithOffset(-1);
          if (EllipsisEnd.isMacroID()) {
            IsAnyPartOfParamsInMacro = true;
          }
        }
      }
    }
  }
  
  if (IsAnyPartOfParamsInMacro) {
      return;
  }

  unsigned LParenLine = SM.getSpellingLineNumber(LParenLoc);
  unsigned RParenLine = SM.getSpellingLineNumber(RParenLoc);

  const ParmVarDecl *FirstParam = FD->getParamDecl(0);
  const ParmVarDecl *LastParam = FD->getParamDecl(NumParams - 1);
  
  unsigned FirstParamLine = SM.getSpellingLineNumber(FirstParam->getBeginLoc());
  unsigned LastParamLine = SM.getSpellingLineNumber(LastParam->getEndLoc());

  bool AllParamsOnSingleLineWithParens = (LParenLine == FirstParamLine);
  if (AllParamsOnSingleLineWithParens) {
    for (unsigned i = 0; i < NumParams; ++i) {
        if (SM.getSpellingLineNumber(FD->getParamDecl(i)->getBeginLoc()) != LParenLine) {
            AllParamsOnSingleLineWithParens = false;
            break;
        }
    }
  }
  
  if (AllParamsOnSingleLineWithParens) {
      AllParamsOnSingleLineWithParens = (LParenLine == RParenLine);
  }
  
  if (AllParamsOnSingleLineWithParens && FD->isVariadic()) {
    if (SM.getSpellingLineNumber(LastParam->getEndLoc()) != RParenLine) {
      AllParamsOnSingleLineWithParens = false;
    }
  }

  bool IsProperMultiLineLayout = (FirstParamLine > LParenLine) &&
                                (RParenLine > LastParamLine);
  
  if (IsProperMultiLineLayout && FD->isVariadic()) {
    if (RParenLine <= LastParamLine) {
      IsProperMultiLineLayout = false;
    }
  }

  if (AllParamsOnSingleLineWithParens || IsProperMultiLineLayout)
    return;

  auto Diag = diag(FD->getLocation(), "Function arguments are partially wrapped. Arguments "
                                    "should either all be on the same line with parentheses, "
                                    "or the first argument on a new line after '(', and ')' "
                                    "on a new line after the last argument.");

  // --- Start of QuickFix ---
  // (IsAnyPartOfParamsInMacro should have led to an early return if true)

  StringRef IndentOfLParenLineText = Lexer::getIndentationForLine(SM.getSpellingLoc(LParenLoc), SM);
  
  std::string ParamIndentStr = IndentOfLParenLineText.str() + "    ";
  std::string RParenIndentStr = IndentOfLParenLineText.str();

  std::string FixedArgsString = "\n";
  for (unsigned i = 0; i < NumParams; ++i) {
    const ParmVarDecl *PVD = FD->getParamDecl(i);
    CharSourceRange ParamCharRange = CharSourceRange::getTokenRange(PVD->getSourceRange());
    StringRef ParamText = Lexer::getSourceText(ParamCharRange, SM, LO);

    if (ParamText.empty()) { 
        return; 
    }

    FixedArgsString += ParamIndentStr;
    FixedArgsString += ParamText.str();
    if (i < NumParams - 1 || FD->isVariadic()) {
      FixedArgsString += ",";
    }
    FixedArgsString += "\n";
  }

  if (FD->isVariadic()) {
    FixedArgsString += ParamIndentStr;
    FixedArgsString += "..."; 
    FixedArgsString += "\n";
  }

  FixedArgsString += RParenIndentStr;

  SourceLocation StartOfReplace = LParenLoc.getLocWithOffset(1); 
  SourceLocation EndOfReplace = RParenLoc;

  if (StartOfReplace.isInvalid() || EndOfReplace.isInvalid() || 
      SM.isBeforeInTranslationUnit(EndOfReplace, StartOfReplace)) {
      return;
  }
  
  Diag << FixItHint::CreateReplacement(CharSourceRange::getCharRange(StartOfReplace, EndOfReplace), FixedArgsString);
}

} // namespace clang::tidy::readability
