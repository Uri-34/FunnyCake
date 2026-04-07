#!/bin/bash

DIR_NAME=$(basename "$(pwd)")
OUTPUT="${DIR_NAME}.pri"

echo "Генерация $OUTPUT из файлов в $(pwd)..."
> "$OUTPUT"

cat >> "$OUTPUT" <<EOF
# Автосгенерировано: $(date)
# Скриптом: $0

EOF

write_var_per_line() {
    local varname="$1"
    shift
    local items=("$@")

    [ ${#items[@]} -eq 0 ] && return

    for item in "${items[@]}"; do
        item_esc=$(printf '%q' "$item" | sed "s/'/\\'/g")
        echo "${varname} += "'$$PWD'"/${item_esc}" >> "$OUTPUT"
    done
    echo "" >> "$OUTPUT"
}

declare -a src_files hdr_files ui_files qrc_files other_files

while IFS= read -r -d '' file; do
    rel_path="${file#./}"

    # === Исключаем нежелательные пути (case-insensitive) ===
    rel_lower=$(printf '%s' "$rel_path" | tr '[:upper:]' '[:lower:]')
    if printf '%s\n' "$rel_lower" | grep -qE '(^|/)(test|temp|\.git|build-)/'; then
        continue
    fi

    # === Классификация ===
    case "$rel_path" in
        *.cpp|*.cc|*.cxx)
            src_files+=("$rel_path")
            ;;
        *.h|*.hpp|*.hh)
            hdr_files+=("$rel_path")
            ;;
        *.ui)
            ui_files+=("$rel_path")
            ;;
        *.qrc)
            qrc_files+=("$rel_path")
            ;;
        CMakeLists.txt|README*|*.md|LICENSE*)
            other_files+=("$rel_path")
            ;;
    esac
done < <(find . -type f -print0 2>/dev/null)

# Сортировка
sort_array() { local arr=("$@"); IFS=$'\n'; sorted=($(sort <<<"${arr[*]}")); unset IFS; printf '%s\n' "${sorted[@]}"; }
src_files=($(sort_array "${src_files[@]}"))
hdr_files=($(sort_array "${hdr_files[@]}"))
ui_files=($(sort_array "${ui_files[@]}"))
qrc_files=($(sort_array "${qrc_files[@]}"))
other_files=($(sort_array "${other_files[@]}"))

# INCLUDEPATH: все директории с .h + корень
declare -A inc_dirs
for f in "${hdr_files[@]}"; do
    inc_dirs["$(dirname "$f")"]=1
done
inc_dirs["."]=1  # всегда включаем корень

mapfile -t include_paths < <(printf '%s\n' "${!inc_dirs[@]}" | sort)

echo "# Include directories" >> "$OUTPUT"
write_var_per_line "INCLUDEPATH" "${include_paths[@]}"

write_var_per_line "SOURCES" "${src_files[@]}"
write_var_per_line "HEADERS" "${hdr_files[@]}"
write_var_per_line "FORMS" "${ui_files[@]}"
write_var_per_line "RESOURCES" "${qrc_files[@]}"
write_var_per_line "OTHER_FILES" "${other_files[@]}"

echo "✅ Готово → $OUTPUT"
echo "   SOURCES: ${#src_files[@]}, HEADERS: ${#hdr_files[@]}"
