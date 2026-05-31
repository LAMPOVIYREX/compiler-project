#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <getopt.h>
#include "lexer/Scanner.hpp"
#include "utils/ErrorReporter.hpp"
#include "preprocessor/PreprocessorFrontend.hpp"
#include "parser/Parser.hpp"
#include "parser/ASTPrettyPrinter.hpp"
#include "parser/ASTDotGenerator.hpp"
#include "parser/ASTJsonGenerator.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "ir/IRGenerator.hpp"
#include "ir/SSABuilder.hpp"
#include "codegen/X86Generator.hpp"
#include "ir/LoopOptimizer.hpp"
#include "ir/Optimizer.hpp"
#include <cstdlib>
#include <unistd.h>

using namespace minicompiler;


enum class Command {
    LEX,
    PREPROCESS,
    PARSE,
    CHECK,
    IR,
    SSA,
    CODEGEN,
    COMPILE,
    HELP
};

struct Options {
    Command command = Command::HELP;
    std::string filename;
    ASTFormat format = ASTFormat::TEXT;
    std::string outputFile;
    bool verbose = false;
    bool stats = false;
    bool json = false;
    bool semantic = false;
    bool optimize = false;
    bool warnings = false;
    bool wall = false;
    bool werror = false;
    bool wnoUnused = false;
    std::string configFile;   // путь к файлу конфигурации
    std::string errorFormat = "default";  // формат вывода ошибок: default, json, ide
    std::string colorMode = "auto"; // --color
    bool asmOnly = false;      // -S
    bool objectOnly = false;   // -c
    bool preprocessOnly = false; // -E
    int optLevel = 0;          // -O0..3
    std::string target;        // --target
    bool debug = false;        // -g
    std::string includeDir;    // -I
    std::string libraryDir;    // -L
    std::string library;       // -l
};
void printCompilerErrors(const ErrorReporter& errorReporter, const Options& opts) {
    if (!errorReporter.hasErrors()) return;
    
    if (opts.errorFormat == "json") {
        std::cerr << errorReporter.getErrorsAsJson();
    } else if (opts.errorFormat == "ide") {
        std::cerr << errorReporter.getErrorsAsIDE();
    } else {
        errorReporter.printErrors();
    }
}
void printUsage(const char* programName) {
    std::cout << "MiniCompiler - Educational Compiler for MiniLang\n";
    std::cout << "================================================\n\n";
    std::cout << "Usage: " << programName << " <command> [options] <file>\n\n";
    
    std::cout << "Commands:\n";
    std::cout << "  lex <file>              Tokenize source file (Sprint 1)\n";
    std::cout << "  preprocess <file>       Run preprocessor only (Sprint 1)\n";
    std::cout << "  parse <file>            Parse and build AST (Sprint 2)\n";
    std::cout << "  check <file>            Semantic analysis only (Sprint 3)\n";
    std::cout << "  ir <file>               Generate Intermediate Representation (Sprint 4)\n";
    std::cout << "  ssa <file>              Generate SSA form IR (Sprint 4)\n";
    std::cout << "  codegen <file>          Generate x86-64 assembly (Sprint 5)\n";
    std::cout << "  compile <file>          Full compilation (all stages)\n";
    std::cout << "  help                    Show this help message\n\n";
    
    std::cout << "Options:\n";
    std::cout << "  -S                    Generate assembly only\n";
    std::cout << "  -c                    Compile to object file\n";
    std::cout << "  -E                    Preprocess only\n";
    std::cout << "  -O<level>             Set optimization level (0-3)\n";
    std::cout << "  -g                    Generate debug info (not implemented)\n";
    std::cout << "  -I<dir>               Add include directory (not implemented)\n";
    std::cout << "  -L<dir>               Add library directory (not implemented)\n";
    std::cout << "  -l<lib>               Link with library (not implemented)\n";
    std::cout << "  --target <arch>       Set target architecture (not implemented)\n";
    std::cout << "  --format <text|dot|json>  Output format (default: text)\n";
    std::cout << "  --output <file>           Output file (default: stdout)\n";
    std::cout << "  --verbose                 Show detailed information\n";
    std::cout << "  --stats                   Show compilation statistics\n";
    std::cout << "  --warnings                Enable warnings\n";
    std::cout << "  --Wall                   Enable all warnings\n";
    std::cout << "  --Werror                 Treat warnings as errors\n";
    std::cout << "  --Wno-unused             Suppress unused variable warnings\n\n";
    std::cout << "  --config <file>          Specify configuration file (default .minirc)\n";
    std::cout << "  --error-format <default|json|ide>   Error output format (default: default)\n";

    std::cout << "Examples:\n";
    std::cout << "  " << programName << " lex examples/hello.src\n";
    std::cout << "  " << programName << " parse examples/factorial.src --format dot --output ast.dot\n";
    std::cout << "  " << programName << " check examples/factorial.src --verbose\n";
    std::cout << "  " << programName << " ir examples/factorial.src\n";
    std::cout << "  " << programName << " ssa examples/factorial.src\n";
    std::cout << "  " << programName << " codegen examples/factorial.src\n";
    std::cout << "  " << programName << " compile examples/structs.src --stats\n";
}

Options parseOptions(int argc, char* argv[]) {
    Options opts;
    
    if (argc < 2) {
        return opts;
    }
    
    std::string cmd = argv[1];
    
    if (cmd == "lex") {
        opts.command = Command::LEX;
    } else if (cmd == "preprocess") {
        opts.command = Command::PREPROCESS;
    } else if (cmd == "parse") {
        opts.command = Command::PARSE;
    } else if (cmd == "check") {
        opts.command = Command::CHECK;
        opts.semantic = true;
    } else if (cmd == "ir") {
        opts.command = Command::IR;
    } else if (cmd == "ssa") {
        opts.command = Command::SSA;
    } else if (cmd == "codegen") {
        opts.command = Command::CODEGEN;
    } else if (cmd == "compile") {
        opts.command = Command::COMPILE;
    } else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        opts.command = Command::HELP;
        return opts;
    } else if (cmd == "--version" || cmd == "-V") {
        std::cout << "minicompiler 1.0.0\n";
        std::cout << "Compiler for MiniLang\n";
        std::cout << "Target: x86_64-linux-gnu\n";
        std::cout << "Built: " << __DATE__ << "\n";
        exit(0);
    } else {
        std::cerr << "Error: Unknown command '" << cmd << "'\n";
        opts.command = Command::HELP;
        return opts;
    }
    
    // Parse options for commands that support them
    if (opts.command == Command::PARSE || opts.command == Command::CHECK || 
        opts.command == Command::IR || opts.command == Command::SSA ||
        opts.command == Command::CODEGEN || opts.command == Command::COMPILE) {
        
        static struct option long_options[] = {
            {"format", required_argument, 0, 'f'},
            {"output", required_argument, 0, 'o'},
            {"verbose", no_argument, 0, 'v'},
            {"stats", no_argument, 0, 's'},
            {"optimize", no_argument, 0, 'O'},
            {"warnings", no_argument, 0, 'W'},
            {"Wall", no_argument, 0, 1000},
            {"Werror", no_argument, 0, 1001},
            {"Wno-unused", no_argument, 0, 1002},
            {"config", required_argument, 0, 1003},
            {"error-format", required_argument, 0, 2000},
            {"target", required_argument, 0, 't'},
            {"debug", no_argument, 0, 'g'},
            {"include", required_argument, 0, 'I'},
            {"library-dir", required_argument, 0, 'L'},
            {"link", required_argument, 0, 'l'},
            {"color", required_argument, 0, 256},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c;
        
        // Skip first two arguments (program name and command)
        optind = 2;
        
        while ((c = getopt_long(argc, argv, "f:o:vsWScE:O:I:L:l:t:g", long_options, &option_index)) != -1) {
            switch (c) {
                case 'f':
                    if (std::string(optarg) == "dot") {
                        opts.format = ASTFormat::DOT;
                    } else if (std::string(optarg) == "json") {
                        opts.format = ASTFormat::JSON;
                    } else {
                        opts.format = ASTFormat::TEXT;
                    }
                    break;
                case 'o':
                    opts.outputFile = optarg;
                    break;
                case 'v':
                    opts.verbose = true;
                    break;
                case 's':
                    opts.stats = true;
                    break;
                case 'W':
                    opts.warnings = true;
                    break;
                case 1000:
                    opts.wall = true;
                    opts.warnings = true; // включает все предупреждения
                    break;
                case 1001: 
                    opts.werror = true;
                    break;
                case 1002: 
                    opts.wnoUnused = true;
                    break;
                case 1003: // --config
                    opts.configFile = optarg;
                    break;
                case 2000: // --error-format
                    opts.errorFormat = optarg;
                    break;
                case 'S':
                    opts.asmOnly = true;
                    break;
                case 'c':
                    opts.objectOnly = true;
                    break;
                case 'E':
                    opts.preprocessOnly = true;
                    break;
                case 'O':
                    if (optarg) {
                        opts.optLevel = std::atoi(optarg);
                    }
                    opts.optimize = true;
                    break;
                case 't':
                    opts.target = optarg;
                    break;
                case 'g':
                    opts.debug = true;
                    break;
                case 'I':
                    opts.includeDir = optarg;
                    break;
                case 'L':
                    opts.libraryDir = optarg;
                    break;
                case 'l':
                    opts.library = optarg;
                    break;
                case 256: 
                    opts.colorMode = optarg;
                    break;
                default:
                    break;
            }
        }
        
        // Get filename
        if (optind < argc) {
            opts.filename = argv[optind];
        }
    } else {
        // For other commands, filename is the second argument
        if (argc > 2) {
            opts.filename = argv[2];
        }
    }
    
    return opts;
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool runLexer(const std::string& filename, const std::string& colorMode = "auto", const std::string& errorFormat = "default", bool usePreprocessor = false) {
    try {
        ErrorReporter errorReporter;
        errorReporter.useColor = (colorMode == "auto") ? isatty(fileno(stderr)) : (colorMode == "always");

        errorReporter.setFilename(filename);
        
        std::vector<Token> tokens;
        
        if (usePreprocessor) {
            PreprocessorFrontend frontend(errorReporter);
            tokens = frontend.tokenizeWithPreprocessor(filename);
        } else {
            std::string source = readFile(filename);
            
            // Set source lines for error reporting
            std::istringstream stream(source);
            std::string line;
            int lineNum = 1;
            while (std::getline(stream, line)) {
                errorReporter.setSourceLine(lineNum, line);
                lineNum++;
            }
            
            Scanner scanner(source, errorReporter);
            tokens = scanner.scanTokens();
        }
        
        // Output tokens
        for (const auto& token : tokens) {
            std::cout << token.toString() << std::endl;
        }
        
        // Output errors if any
        if (errorReporter.hasErrors()) {
            std::cout << "\nНайдены ошибки:" << std::endl;
            if (errorFormat == "json") std::cerr << errorReporter.getErrorsAsJson();
            else if (errorFormat == "ide") std::cerr << errorReporter.getErrorsAsIDE();
            else errorReporter.printErrors();
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool runPreprocessor(const std::string& filename, const std::string& colorMode = "auto") {
    try {
        ErrorReporter errorReporter;
        errorReporter.useColor = (colorMode == "auto") ? isatty(fileno(stderr)) : (colorMode == "always");
        errorReporter.setFilename(filename);
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (!preprocessed.empty()) {
            std::cout << preprocessed << std::endl;
        }
        
        if (errorReporter.hasErrors()) {
            errorReporter.printErrors();
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool runParser(const std::string& filename, const Options& opts) {
    try {
        ErrorReporter errorReporter;
        errorReporter.useColor = (opts.colorMode == "auto") ? isatty(fileno(stderr)) : (opts.colorMode == "always");
        errorReporter.setFilename(filename);
        if (opts.verbose) std::cerr << "[Verbose] Parsing " << filename << "...\n";
        // Read and preprocess source
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        // Set source lines for error reporting
        std::istringstream stream(preprocessed);
        std::string line;
        int lineNum = 1;
        while (std::getline(stream, line)) {
            errorReporter.setSourceLine(lineNum, line);
            lineNum++;
        }
        
        // Lexical analysis
        Scanner scanner(preprocessed, errorReporter);
        auto tokens = scanner.scanTokens();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (opts.verbose) {
            std::cerr << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cerr << "  " << token.toString() << std::endl;
            }
            std::cerr << std::endl;
        }
        
        // Parsing
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (!program) {
            std::cerr << "Failed to parse program\n";
            return false;
        }
        
        // Semantic analysis (if needed)
        std::unique_ptr<SemanticAnalyzer> analyzer;
        bool semanticValid = true;
        
        if (opts.semantic || opts.format == ASTFormat::DOT) {
            analyzer = std::make_unique<SemanticAnalyzer>(errorReporter, opts.warnings, opts.werror, opts.wnoUnused);
            semanticValid = analyzer->analyze(*program);
            
            if (opts.verbose && analyzer) {
                std::cerr << "\n=== Symbol Table ===" << std::endl;
                std::cerr << analyzer->getSymbolTable().toString() << std::endl;
                
                std::cerr << "\n=== Memory Layout ===" << std::endl;
                analyzer->printMemoryLayout();
            }
            
            // If this is a check command and there are errors
            if (opts.command == Command::CHECK) {
                if (errorReporter.hasErrors()) {
                    printCompilerErrors(errorReporter, opts);
                    return false;
                }
                return true;
            }
            
            if (!semanticValid) {
                printCompilerErrors(errorReporter, opts);
                return false;
            }
        }
        
        // Output AST (only if no errors and not pure check command)
        if (!errorReporter.hasErrors()) {
            std::ostream* out = &std::cout;
            std::ofstream fileStream;
            
            if (!opts.outputFile.empty()) {
                fileStream.open(opts.outputFile);
                if (!fileStream.is_open()) {
                    std::cerr << "Error: Cannot open output file: " << opts.outputFile << std::endl;
                    return false;
                }
                out = &fileStream;
            }
            
            switch (opts.format) {
                case ASTFormat::TEXT: {
                    ASTPrettyPrinter printer(*out);
                    program->accept(printer);
                    break;
                }
                case ASTFormat::DOT: {
                    ASTDotGenerator generator(*out, true);
                    if (analyzer) {
                        generator.setNodeTypes(analyzer->getExprTypes());
                    }
                    program->accept(generator);
                    break;
                }
                case ASTFormat::JSON: {
                    ASTJsonGenerator generator(*out);
                    program->accept(generator);
                    break;
                }
            }
        }
        
        if (opts.stats) {
            errorReporter.printStats();
        }
        
        return !errorReporter.hasErrors();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool runCompile(const std::string& filename, const Options& opts) {
    if (opts.verbose) std::cerr << "[Verbose] Full compilation of " << filename << "...\n";
    Options newOpts = opts;
    newOpts.semantic = true;
    newOpts.format = ASTFormat::TEXT;
    return runParser(filename, newOpts);
}

bool runIR(const std::string& filename, const Options& opts) {
    try {
        ErrorReporter errorReporter;
        errorReporter.useColor = (opts.colorMode == "auto") ? isatty(fileno(stderr)) : (opts.colorMode == "always");
        errorReporter.setFilename(filename);
        if (opts.verbose) std::cerr << "[Verbose] Generating IR for " << filename << "...\n";
        // Препроцессор
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        // Set source lines
        std::istringstream stream(preprocessed);
        std::string line;
        int lineNum = 1;
        while (std::getline(stream, line)) {
            errorReporter.setSourceLine(lineNum, line);
            lineNum++;
        }
        
        // Лексер
        Scanner scanner(preprocessed, errorReporter);
        auto tokens = scanner.scanTokens();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (opts.verbose) {
            std::cerr << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cerr << "  " << token.toString() << std::endl;
            }
            std::cerr << std::endl;
        }
        
        // Парсер
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (!program) {
            std::cerr << "Failed to parse program\n";
            return false;
        }
        
        // Семантический анализ
        SemanticAnalyzer analyzer(errorReporter, opts.warnings, opts.werror, opts.wnoUnused);
        bool semanticValid = analyzer.analyze(*program);
        
        if (!semanticValid || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (opts.verbose) {
            std::cerr << "\n=== Symbol Table ===" << std::endl;
            std::cerr << analyzer.getSymbolTable().toString() << std::endl;
        }
        
        // Генерация IR
        IRGenerator irGen(analyzer.getSymbolTable(), errorReporter);
        auto irProgram = irGen.generate(*program);
        
        if (irGen.hasErrors() || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        // Вывод IR
        std::ostream* out = &std::cout;
        std::ofstream fileStream;
        
        if (!opts.outputFile.empty()) {
            fileStream.open(opts.outputFile);
            if (!fileStream.is_open()) {
                std::cerr << "Error: Cannot open output file: " << opts.outputFile << std::endl;
                return false;
            }
            out = &fileStream;
        }
        
        *out << irProgram->toString();
        
        if (opts.stats) {
            errorReporter.printStats();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool runSSA(const std::string& filename, const Options& opts) {
    try {
        ErrorReporter errorReporter;
        errorReporter.useColor = (opts.colorMode == "auto") ? isatty(fileno(stderr)) : (opts.colorMode == "always");
        errorReporter.setFilename(filename);
        if (opts.verbose) std::cerr << "[Verbose] Generating SSA for " << filename << "...\n";
        // Препроцессор
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        // Set source lines
        std::istringstream stream(preprocessed);
        std::string line;
        int lineNum = 1;
        while (std::getline(stream, line)) {
            errorReporter.setSourceLine(lineNum, line);
            lineNum++;
        }
        
        // Лексер
        Scanner scanner(preprocessed, errorReporter);
        auto tokens = scanner.scanTokens();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (opts.verbose) {
            std::cerr << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cerr << "  " << token.toString() << std::endl;
            }
            std::cerr << std::endl;
        }
        
        // Парсер
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (!program) {
            std::cerr << "Failed to parse program\n";
            return false;
        }
        
        // Семантический анализ
        SemanticAnalyzer analyzer(errorReporter, opts.warnings, opts.werror, opts.wnoUnused);
        bool semanticValid = analyzer.analyze(*program);
        
        if (!semanticValid || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (opts.verbose) {
            std::cerr << "\n=== Symbol Table ===" << std::endl;
            std::cerr << analyzer.getSymbolTable().toString() << std::endl;
        }
        
        // Генерация IR
        IRGenerator irGen(analyzer.getSymbolTable(), errorReporter);
        auto irProgram = irGen.generate(*program);
        
        if (irGen.hasErrors() || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        // Преобразование в SSA форму
        SSABuilder ssaBuilder;
        ssaBuilder.setOptimizeConstants(true);
        ssaBuilder.setEliminateDeadCode(true);
        
        auto ssaProgram = ssaBuilder.buildSSA(*irProgram);
        
        // Вывод SSA
        std::ostream* out = &std::cout;
        std::ofstream fileStream;
        
        if (!opts.outputFile.empty()) {
            fileStream.open(opts.outputFile);
            if (!fileStream.is_open()) {
                std::cerr << "Error: Cannot open output file: " << opts.outputFile << std::endl;
                return false;
            }
            out = &fileStream;
        }
        
        *out << ssaProgram->toString();
        
        if (opts.verbose) {
            std::cerr << "\n=== SSA Transformation Complete ===" << std::endl;
        }
        
        if (opts.stats) {
            errorReporter.printStats();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// Code Generation (Sprint 5)
// ============================================================================

bool runCodegen(const std::string& filename, const Options& opts) {
    try {
        ErrorReporter errorReporter;
        errorReporter.useColor = (opts.colorMode == "auto") ? isatty(fileno(stderr)) : (opts.colorMode == "always");
        errorReporter.setFilename(filename);
        if (opts.verbose) std::cerr << "[Verbose] Compiling " << filename << "...\n";
        // Препроцессор
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        // Set source lines
        std::istringstream stream(preprocessed);
        std::string line;
        int lineNum = 1;
        while (std::getline(stream, line)) {
            errorReporter.setSourceLine(lineNum, line);
            lineNum++;
        }
        
        // Лексер
        Scanner scanner(preprocessed, errorReporter);
        auto tokens = scanner.scanTokens();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (opts.verbose) {
            std::cerr << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cerr << "  " << token.toString() << std::endl;
            }
            std::cerr << std::endl;
        }
        
        // Парсер
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (!program) {
            std::cerr << "Failed to parse program\n";
            return false;
        }
        
        // Семантический анализ
        SemanticAnalyzer analyzer(errorReporter, opts.warnings, opts.werror, opts.wnoUnused);
        bool semanticValid = analyzer.analyze(*program);
        
        if (!semanticValid || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }
        
        if (opts.verbose) {
            std::cerr << "\n=== Symbol Table ===" << std::endl;
            std::cerr << analyzer.getSymbolTable().toString() << std::endl;
        }
        
        // Генерация IR
        IRGenerator irGen(analyzer.getSymbolTable(), errorReporter);
        auto irProgram = irGen.generate(*program);
        
        if (irGen.hasErrors() || errorReporter.hasErrors()) {
            printCompilerErrors(errorReporter, opts);
            return false;
        }

        // Оптимизации IR (только с флагом --optimize)
        if (opts.optimize) {
            IROptimizer optimizer;
            optimizer.setConstantFolding(true);        
            optimizer.setConstantPropagation(true);    
            optimizer.setDeadCodeElimination(true);    
            optimizer.optimize(*irProgram);
            
            if (opts.stats) {
                auto stats = optimizer.getStats();
                std::cerr << "\n=== Optimization Statistics ===" << std::endl;
                std::cerr << "  Algebraic simplification: " << stats.algebraicSimplifications << " simplifications" << std::endl;
                std::cerr << "  Constant folding:     " << stats.foldedConstants << " expressions folded" << std::endl;
                std::cerr << "  Constant propagation: " << stats.propagatedConstants << " variables propagated" << std::endl;
                std::cerr << "  Dead code eliminated: " << stats.deadInstructions << " instructions removed" << std::endl;
                std::cerr << "  Total instructions:   " << stats.totalBefore 
                          << " -> " << stats.totalAfter 
                          << " (" << (stats.totalBefore - stats.totalAfter) << " removed)" << std::endl;
            }
        }

        // Генерация x86-64 кода
        X86Generator x86Gen(analyzer.getSymbolTable(), errorReporter);
        x86Gen.setSyntaxNASM();
        std::string assembly = x86Gen.generate(*irProgram);
        
        // Вывод ассемблера
        std::ostream* out = &std::cout;
        std::ofstream fileStream;
        
        if (!opts.outputFile.empty()) {
            fileStream.open(opts.outputFile);
            if (!fileStream.is_open()) {
                std::cerr << "Error: Cannot open output file: " << opts.outputFile << std::endl;
                return false;
            }
            out = &fileStream;
        }
        
        *out << assembly;
        
        if (opts.verbose) {
            std::cerr << "\n=== Code Generation Complete ===" << std::endl;
        }
        
        if (opts.stats) {
            errorReporter.printStats();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

// ============================================
// Configuration file support (CLI-4)
// ============================================

std::unordered_map<std::string, std::string> loadConfigFile(const std::string& path) {
    std::unordered_map<std::string, std::string> config;
    std::ifstream file(path);
    if (!file.is_open()) return config;
    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(start, end - start + 1);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        config[key] = value;
    }
    return config;
}

void applyConfig(Options& opts, const std::unordered_map<std::string, std::string>& config) {
    auto setIfAbsent = [&](const std::string& key, auto& target) {
        if (config.count(key)) {
            std::string val = config.at(key);
            if constexpr (std::is_same_v<std::decay_t<decltype(target)>, bool>) {
                target = (val == "true" || val == "1" || val == "yes");
            } else if constexpr (std::is_same_v<std::decay_t<decltype(target)>, int>) {
                target = std::stoi(val);
            } else {
                target = val;
            }
        }
    };
    setIfAbsent("output", opts.outputFile);
    setIfAbsent("verbose", opts.verbose);
    setIfAbsent("stats", opts.stats);
    setIfAbsent("optimize", opts.optimize);
    setIfAbsent("warnings", opts.warnings);
    setIfAbsent("color", opts.colorMode);
    setIfAbsent("target", opts.target);
    setIfAbsent("optlevel", opts.optLevel);
}

void applyEnv(Options& opts) {
    auto getEnv = [](const char* name) -> std::string {
        const char* val = std::getenv(name);
        return val ? std::string(val) : "";
    };
    auto setIfNotEmpty = [&](const char* envName, auto& target) {
        std::string val = getEnv(envName);
        if (!val.empty()) {
            if constexpr (std::is_same_v<std::decay_t<decltype(target)>, bool>) {
                target = (val == "true" || val == "1" || val == "yes");
            } else if constexpr (std::is_same_v<std::decay_t<decltype(target)>, int>) {
                target = std::stoi(val);
            } else {
                target = val;
            }
        }
    };
    setIfNotEmpty("MINICC_OUTPUT", opts.outputFile);
    setIfNotEmpty("MINICC_VERBOSE", opts.verbose);
    setIfNotEmpty("MINICC_STATS", opts.stats);
    setIfNotEmpty("MINICC_OPTIMIZE", opts.optimize);
    setIfNotEmpty("MINICC_WARNINGS", opts.warnings);
    setIfNotEmpty("MINICC_COLOR", opts.colorMode);
    setIfNotEmpty("MINICC_TARGET", opts.target);
    setIfNotEmpty("MINICC_OPTLEVEL", opts.optLevel);
}


int main(int argc, char* argv[]) {
    // Проверка --version до разбора команд
    if (argc >= 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-V")) {
        std::cout << "minicompiler 1.0.0\n";
        std::cout << "Compiler for MiniLang\n";
        std::cout << "Target: x86_64-linux-gnu\n";
        std::cout << "Built: " << __DATE__ << "\n";
        return 0;
    }
    // Предварительно ищем --config в аргументах командной строки
    std::string configPath = ".minirc";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
            break;
        }
    }
    Options opts = parseOptions(argc, argv);
    // Загружаем конфиг (если опция --config не была передана явно, используем путь из opts)
    if (opts.configFile.empty()) opts.configFile = configPath;
    auto config = loadConfigFile(opts.configFile);
    applyConfig(opts, config);
    applyEnv(opts);
    bool success = true;
    
    // Если команда не указана явно, определяем по флагам
    if (opts.command == Command::HELP && !opts.filename.empty()) {
        if (opts.preprocessOnly) opts.command = Command::PREPROCESS;
        else if (opts.asmOnly || opts.objectOnly) opts.command = Command::CODEGEN;
        else opts.command = Command::COMPILE;
    }

    switch (opts.command) {
        case Command::LEX:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runLexer(opts.filename, opts.colorMode, opts.errorFormat, false);
            break;
            
        case Command::PREPROCESS:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runPreprocessor(opts.filename, opts.colorMode);
            break;
            
        case Command::PARSE:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runParser(opts.filename, opts);
            break;
            
        case Command::CHECK:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runParser(opts.filename, opts);
            break;

        case Command::IR:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runIR(opts.filename, opts);
            break;

        case Command::SSA:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runSSA(opts.filename, opts);
            break;

        case Command::CODEGEN:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runCodegen(opts.filename, opts);
            break;
            
        case Command::COMPILE:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runCompile(opts.filename, opts);
            break;
            
        case Command::HELP:
        default:
            printUsage(argv[0]);
            return 0;
    }
    
    return success ? 0 : 1;
}