#!/bin/bash
echo "Удаление BOM из всех исходных файлов..."

# Удаляем BOM из всех .src файлов
find . -name "*.src" -type f -exec sed -i '1s/^\xEF\xBB\xBF//' {} \;

# Удаляем BOM из всех .cpp файлов  
find . -name "*.cpp" -type f -exec sed -i '1s/^\xEF\xBB\xBF//' {} \;

# Удаляем BOM из всех .hpp файлов
find . -name "*.hpp" -type f -exec sed -i '1s/^\xEF\xBB\xBF//' {} \;

echo "Готово!"