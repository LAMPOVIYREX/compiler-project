#include <iostream>
#include <fstream>
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
};

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
    
    std::cout << "Options for parse/check/ir/ssa/codegen command:\n";
    std::cout << "  --format <text|dot|json>  Output format (default: text)\n";
    std::cout << "  --output <file>           Output file (default: stdout)\n";
    std::cout << "  --verbose                 Show detailed information\n";
    std::cout << "  --stats                   Show compilation statistics\n\n";
    
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
    } else if (cmd == "help") {
        opts.command = Command::HELP;
        return opts;
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
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c;
        
        // Skip first two arguments (program name and command)
        optind = 2;
        
        while ((c = getopt_long(argc, argv, "f:o:vs", long_options, &option_index)) != -1) {
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
                case 'O':
                    opts.optimize = true;
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

bool runLexer(const std::string& filename, bool usePreprocessor = false) {
    try {
        ErrorReporter errorReporter;
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
            errorReporter.printErrors();
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool runPreprocessor(const std::string& filename) {
    try {
        ErrorReporter errorReporter;
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
        errorReporter.setFilename(filename);
        
        // Read and preprocess source
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
            errorReporter.printErrors();
            return false;
        }
        
        if (opts.verbose) {
            std::cout << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cout << "  " << token.toString() << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Parsing
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
            analyzer = std::make_unique<SemanticAnalyzer>(errorReporter);
            semanticValid = analyzer->analyze(*program);
            
            if (opts.verbose && analyzer) {
                std::cout << "\n=== Symbol Table ===" << std::endl;
                std::cout << analyzer->getSymbolTable().toString() << std::endl;
                
                std::cout << "\n=== Memory Layout ===" << std::endl;
                analyzer->printMemoryLayout();
            }
            
            // If this is a check command and there are errors
            if (opts.command == Command::CHECK) {
                if (errorReporter.hasErrors()) {
                    errorReporter.printErrors();
                    return false;
                }
                return true;
            }
            
            if (!semanticValid) {
                errorReporter.printErrors();
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
    Options newOpts = opts;
    newOpts.semantic = true;
    newOpts.format = ASTFormat::TEXT;
    return runParser(filename, newOpts);
}

bool runIR(const std::string& filename, const Options& opts) {
    try {
        ErrorReporter errorReporter;
        errorReporter.setFilename(filename);
        
        // Препроцессор
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
            errorReporter.printErrors();
            return false;
        }
        
        if (opts.verbose) {
            std::cout << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cout << "  " << token.toString() << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Парсер
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            errorReporter.printErrors();
            return false;
        }
        
        if (!program) {
            std::cerr << "Failed to parse program\n";
            return false;
        }
        
        // Семантический анализ
        SemanticAnalyzer analyzer(errorReporter);
        bool semanticValid = analyzer.analyze(*program);
        
        if (!semanticValid || errorReporter.hasErrors()) {
            errorReporter.printErrors();
            return false;
        }
        
        if (opts.verbose) {
            std::cout << "\n=== Symbol Table ===" << std::endl;
            std::cout << analyzer.getSymbolTable().toString() << std::endl;
        }
        
        // Генерация IR
        IRGenerator irGen(analyzer.getSymbolTable(), errorReporter);
        auto irProgram = irGen.generate(*program);
        
        if (irGen.hasErrors() || errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
        errorReporter.setFilename(filename);
        
        // Препроцессор
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
            errorReporter.printErrors();
            return false;
        }
        
        if (opts.verbose) {
            std::cout << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cout << "  " << token.toString() << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Парсер
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            errorReporter.printErrors();
            return false;
        }
        
        if (!program) {
            std::cerr << "Failed to parse program\n";
            return false;
        }
        
        // Семантический анализ
        SemanticAnalyzer analyzer(errorReporter);
        bool semanticValid = analyzer.analyze(*program);
        
        if (!semanticValid || errorReporter.hasErrors()) {
            errorReporter.printErrors();
            return false;
        }
        
        if (opts.verbose) {
            std::cout << "\n=== Symbol Table ===" << std::endl;
            std::cout << analyzer.getSymbolTable().toString() << std::endl;
        }
        
        // Генерация IR
        IRGenerator irGen(analyzer.getSymbolTable(), errorReporter);
        auto irProgram = irGen.generate(*program);
        
        if (irGen.hasErrors() || errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
            std::cout << "\n=== SSA Transformation Complete ===" << std::endl;
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
        errorReporter.setFilename(filename);
        
        // Препроцессор
        PreprocessorFrontend frontend(errorReporter);
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (preprocessed.empty() || errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
            errorReporter.printErrors();
            return false;
        }
        
        if (opts.verbose) {
            std::cout << "=== Tokens (" << tokens.size() << ") ===\n";
            for (const auto& token : tokens) {
                std::cout << "  " << token.toString() << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Парсер
        Parser parser(tokens, errorReporter);
        auto program = parser.parse();
        
        if (errorReporter.hasErrors()) {
            errorReporter.printErrors();
            return false;
        }
        
        if (!program) {
            std::cerr << "Failed to parse program\n";
            return false;
        }
        
        // Семантический анализ
        SemanticAnalyzer analyzer(errorReporter);
        bool semanticValid = analyzer.analyze(*program);
        
        if (!semanticValid || errorReporter.hasErrors()) {
            errorReporter.printErrors();
            return false;
        }
        
        if (opts.verbose) {
            std::cout << "\n=== Symbol Table ===" << std::endl;
            std::cout << analyzer.getSymbolTable().toString() << std::endl;
        }
        
        // Генерация IR
        IRGenerator irGen(analyzer.getSymbolTable(), errorReporter);
        auto irProgram = irGen.generate(*program);
        
        if (irGen.hasErrors() || errorReporter.hasErrors()) {
            errorReporter.printErrors();
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
            std::cout << "\n=== Code Generation Complete ===" << std::endl;
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

int main(int argc, char* argv[]) {
    Options opts = parseOptions(argc, argv);
    bool success = true;
    
    switch (opts.command) {
        case Command::LEX:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runLexer(opts.filename, false);
            break;
            
        case Command::PREPROCESS:
            if (opts.filename.empty()) {
                std::cerr << "Error: Missing filename\n";
                printUsage(argv[0]);
                return 1;
            }
            success = runPreprocessor(opts.filename);
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