#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;
using namespace llvm;

namespace {

// Override the format string with this environment variable.
static const char *FMT_ENV_VAR = "SUMMARYPLUGIN_FMT";

// '%' conversion specifiers for use in the format string.
enum ConversionChar {
  CONV_FILE = 'F', // Filename
  CONV_LINE = 'L', // Line count (number of source lines).
};

// All results are contained in this structure.
// The TranslationVisitor populates this.
class TranslationSummary {
  std::string Filename;
  llvm::DenseSet<unsigned> LineNumbers;

public:
  TranslationSummary(StringRef InFile) : Filename(InFile) {}

  void addLineNumber(unsigned Line) { LineNumbers.insert(Line); }

  void dump(StringRef FormatString) {
    if (FormatString.size() == 0)
      return;
    std::string Buffer;
    char Last = 0;
    for (auto C : FormatString) {
      if (Last == '%') {
        switch (C) {
        case CONV_FILE:
          Buffer += Filename;
          break;
        case CONV_LINE:
          Buffer += std::to_string(LineNumbers.size());
          break;
        default:
          Buffer += C;
          break;
        }
      } else if (C == '%') {
        // Do nothing, it's probably a conversion specifier.
      } else {
        Buffer += C;
      }
      Last = C;
    }
    llvm::errs() << Buffer << "\n";
  }
};

// This class collect unique line numbers for every source line in a translation
// unit. To discover these source lines, this class is implemented as a visitor
// and the SummaryPluginConsumer will begin the walk to which this class will be
// called-back and can collect line number data at that time.
class TranslationVisitor : public RecursiveASTVisitor<TranslationVisitor> {
  unsigned getLineNumber(SourceLocation Loc) {
    auto SLoc = SM.getSpellingLoc(Loc);
    auto PLoc = SM.getPresumedLoc(SLoc);
    if (!PLoc.isValid())
      return 0;
    return PLoc.getLine();
  }

  TranslationSummary *Summary;
  const SourceManager &SM;

public:
  TranslationVisitor(const SourceManager &SM, TranslationSummary *S)
      : Summary(S), SM(SM) {}

  // Collect each (non-zero) line number.
  bool VisitStmt(Stmt *S) {
    assert(Summary && "Summary instance not found.");
    if (auto Line = getLineNumber(S->getLocStart()))
      Summary->addLineNumber(Line);
    return true;
  }
};

// This class is triggered once per translation unit,
// and is responsible for walking the AST to collect
// line data.
class SummaryPluginConsumer : public ASTConsumer {
  std::string FileName;     // Store locally.
  std::string FormatString; // Store locally.

public:
  SummaryPluginConsumer(std::string FileName, std::string FmtString)
      : FileName(FileName), FormatString(FmtString) {}
  // Walk the AST via 'Visitor' and print collected stats in 'Summary.'
  void HandleTranslationUnit(ASTContext &Ctx) override {
    auto Summary = llvm::make_unique<TranslationSummary>(FileName);
    auto Visitor = TranslationVisitor(Ctx.getSourceManager(), Summary.get());
    Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
    Summary->dump(FormatString);
  }
};

// This is the main entry point for our plugin.
// It spawns a consumer, which in-turn, will start
// a walk of the AST collecting line information.
class SummaryPluginAction : public PluginASTAction {
  std::string FormatString;

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    return make_unique<SummaryPluginConsumer>(InFile, FormatString);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &Args) override {
    // Initialize the arguments here.
    FormatString = "[%F] -- %L Lines";

    // Override any initialized args with that provided by the command line.
    for (unsigned i = 0; i < Args.size(); ++i) {
      // Collect the format string.
      if (Args[i].find("-fmt=") == 0)
        FormatString = Args[i].substr(strlen("-fmt="));
    }

    // The environment variable overrides command line provided value.
    if (const char *E = std::getenv(FMT_ENV_VAR))
      FormatString = E;

    return true;
  }

  // Note: I'd rather put all display logic here, but
  // this isn't actually called for this plugin.
  // If you call this with '-plugin' instead of '-add-plugin' then
  // I believe that EndSourceFileAction will be called, but in
  // -add-plugin, this plugin is not the 'main' action, and that is the
  // behavior we desire, because we want an object file at some point.
  // void EndSourceFileAction() override {}

  void PrintHelp(raw_ostream &ros) {
    ros << "Print compilation statistics.\n"
        << "Output can be customized by a format string argument:\n"
        << "  -fmt=\"FormatString\"\n"
        << "  Conversion Specifiers:\n"
        << "    %%F -- Display file name.\n"
        << "    %%L -- Display line number.\n";
  }
};

} // namespace

// Pass: For AST parsing.
static FrontendPluginRegistry::Add<SummaryPluginAction>
    X("summary", "display compilation details");
