#!/bin/bash

DIR_NAME=$(basename "$(pwd)")
OUTPUT="${DIR_NAME}.pri"

echo "Генерация $OUTPUT из файлов в $(pwd)..."
> "$OUTPUT"

# Включить вывод генерируемых строк в консоль
VERBOSE=true

# Функция вывода: одна строка — один файл/путь, с $$PWD без искажений
# При VERBOSE=true дублирует вывод в консоль
write_var_per_line() {
    local varname="$1"
    shift
    local items=("$@")

    if [ ${#items[@]} -eq 0 ]; then 
        echo "# (пропущено: $varname — нет элементов)" >&2
        return
    fi

    echo "# $varname (${#items[@]} элементов)" >> "$OUTPUT"
    if $VERBOSE; then
        echo -e "\n# $varname (${#items[@]} элементов):" >&2
    fi

    for item in "${items[@]}"; do
        local line="${varname} += "'$$PWD'"/${item}"
        echo "$line" >> "$OUTPUT"
        if $VERBOSE; then
            echo "  $line" >&2
        fi
    done
    echo "" >> "$OUTPUT"
}

# --- Заголовок .pri ---
cat >> "$OUTPUT" <<EOF
# Автосгенерировано: $(date)
# Скриптом: $0

EOF

# --- Сбор директорий с заголовками (для INCLUDEPATH) ---
HDR_DIRS=()
while IFS= read -r -d '' file; do
    dir=$(dirname "$file" | sed 's|^\./||')
    if [ "$dir" != "." ]; then
        HDR_DIRS+=("$dir")
    fi
done < <(
    find . -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.hh" \) \
        ! -path "*/TEST" ! -path "*/TEST/*" \
        ! -path "*/TEMP" ! -path "*/TEMP/*" \
        ! -path "*/.git/*" ! -path "*/build-*/*" \
        -print0 | sort -z
)

# Уникальные директории
UNIQ_DIRS=()
declare -A seen
for d in "${HDR_DIRS[@]}"; do
    [[ -z ${seen[$d]} ]] && { UNIQ_DIRS+=("$d"); seen[$d]=1; }
done

# --- Сбор файлов ---
mapfile -t src_arr < <(find . -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \) ! -path "*/TEST" ! -path "*/TEST/*" ! -path "*/TEMP" ! -path "*/TEMP/*" ! -path "*/.git/*" ! -path "*/build-*/*" | sed 's|^\./||' | sort)
mapfile -t hdr_arr < <(find . -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.hh" \) ! -path "*/TEST" ! -path "*/TEST/*" ! -path "*/TEMP" ! -path "*/TEMP/*" ! -path "*/.git/*" ! -path "*/build-*/*" | sed 's|^\./||' | sort)
mapfile -t ui_arr < <(find . -type f -name "*.ui" ! -path "*/TEST" ! -path "*/TEST/*" ! -path "*/TEMP" ! -path "*/TEMP/*" ! -path "*/.git/*" ! -path "*/build-*/*" | sed 's|^\./||' | sort)
mapfile -t qrc_arr < <(find . -type f -name "*.qrc" ! -path "*/TEST" ! -path "*/TEST/*" ! -path "*/TEMP" ! -path "*/TEMP/*" ! -path "*/.git/*" ! -path "*/build-*/*" | sed 's|^\./||' | sort)
mapfile -t other_arr < <(find . -type f \( -name "CMakeLists.txt" -o -name "README*" -o -name "*.md" \) ! -path "*/TEST" ! -path "*/TEST/*" ! -path "*/TEMP" ! -path "*/TEMP/*" ! -path "*/.git/*" ! -path "*/build-*/*" | sed 's|^\./||' | sort)

# --- Запись в .pri ---
if [ ${#UNIQ_DIRS[@]} -gt 0 ]; then
    echo "# Include directories (from header locations)" >> "$OUTPUT"
    if $VERBOSE; then
        echo -e "\n# INCLUDEPATH (${#UNIQ_DIRS[@]} директорий):" >&2
        for d in "${UNIQ_DIRS[@]}"; do
            echo "  INCLUDEPATH += "'$$PWD'"/$d" >&2
        done
    fi
    write_var_per_line "INCLUDEPATH" "${UNIQ_DIRS[@]}"
else
    echo "# No subdirs with headers — using project root" >> "$OUTPUT"
    echo "INCLUDEPATH += "'$$PWD'" # project root" >> "$OUTPUT"
    echo "" >> "$OUTPUT"
    if $VERBOSE; then
        echo -e "\n# INCLUDEPATH: project root" >&2
        echo "  INCLUDEPATH += "'$$PWD'" # project root" >&2
    fi
fi

write_var_per_line "SOURCES" "${src_arr[@]}"
write_var_per_line "HEADERS" "${hdr_arr[@]}"
write_var_per_line "FORMS" "${ui_arr[@]}"
write_var_per_line "RESOURCES" "${qrc_arr[@]}"
write_var_per_line "OTHER_FILES" "${other_arr[@]}"

echo "✅ Готово → $OUTPUT"
if $VERBOSE; then
    echo -e "\n📊 Статистика:"
    echo "   SOURCES:      ${#src_arr[@]}"
    echo "   HEADERS:      ${#hdr_arr[@]}"
    echo "   FORMS:        ${#ui_arr[@]}"
    echo "   RESOURCES:    ${#qrc_arr[@]}"
    echo "   OTHER_FILES:  ${#other_arr[@]}"
    echo "   INCLUDEPATH:  ${#UNIQ_DIRS[@]} директорий"
fi
