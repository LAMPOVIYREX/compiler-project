#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Preprocessor.hpp"
#include "../lexer/Token.hpp"                    // ДОБАВЛЕНО
#include "../utils/ErrorReporter.hpp"

namespace minicompiler {

class PreprocessorFrontend {
public:
    PreprocessorFrontend(ErrorReporter& errorReporter);
    
    // Обработать файл через препроцессор
    std::string preprocessFile(const std::string& filename);
    
    // Получить исходный код после препроцессора
    const std::string& getPreprocessedSource() const { return preprocessedSource; }
    
    // Конвертировать позицию из обработанного файла в оригинальный
    int getOriginalLine(int processedLine) const;
    
    // Запустить препроцессор и затем лексер
    std::vector<Token> tokenizeWithPreprocessor(const std::string& filename);

private:
    ErrorReporter& errorReporter;
    std::unique_ptr<Preprocessor> preprocessor;
    std::string preprocessedSource;
    
    std::string readFile(const std::string& filename);
    std::string cleanSource(const std::string& source);
};

} // namespace minicompiler