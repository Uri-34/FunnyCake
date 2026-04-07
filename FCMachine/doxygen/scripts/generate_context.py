#!/usr/bin/env python3
"""
Генерация ПОЛНОГО контекста проекта с детальной информацией о классах.

Извлекает:
  - Полные сигнатуры методов с параметрами и возвращаемыми типами
  - Члены-данные с типами и именами
  - Явную иерархию наследования
  - Сигналы и слоты для асинхронного взаимодействия
  - Конструкторы с параметрами инициализации
  - Виртуальные методы для понимания полиморфизма
  - Перечисления и константы

Исключает папки: TEMP, TEST
Включает: *.h файлы

Использование:
    python3 generate_context.py --output ./doxygen/context/PROJECT_CONTEXT.md
"""

import argparse
import re
from pathlib import Path
import sys
from datetime import datetime
from typing import Dict, List, Optional

def should_exclude(path: Path) -> bool:
    """Проверяет, нужно ли исключить папку из обхода"""
    return path.name in {'TEMP', 'TEST', '.git', '__pycache__', 'build'}

class CppClassParser:
    """Парсер для извлечения полной информации о C++ классе"""
    
    def __init__(self, content: str):
        self.content = content
        self.classes = []
    
    def parse(self) -> List[Dict]:
        """Парсит все классы в файле"""
        class_pattern = r'(?:(?:class|struct)\s+(\w+)\s*(?::\s*([^{\n]+))?\s*\{)'
        
        for match in re.finditer(class_pattern, self.content, re.MULTILINE):
            class_name = match.group(1)
            inheritance = match.group(2) if match.group(2) else None
            
            class_start = match.end()
            class_end = self._find_matching_brace(class_start)
            
            if class_end == -1:
                continue
            
            class_body = self.content[class_start:class_end]
            
            class_info = {
                'name': class_name,
                'type': 'class' if 'class' in match.group(0)[:5] else 'struct',
                'inheritance': self._parse_inheritance(inheritance) if inheritance else [],
                'brief': self._extract_brief(self.content[:match.start()]),
                'constructors': self._extract_constructors(class_body, class_name),
                'destructor': self._extract_destructor(class_body, class_name),
                # ИСПРАВЛЕНО: передаём class_name в _extract_methods
                'public_methods': self._extract_methods(class_body, 'public', class_name),
                'protected_methods': self._extract_methods(class_body, 'protected', class_name),
                'private_methods': self._extract_methods(class_body, 'private', class_name),
                'public_members': self._extract_members(class_body, 'public'),
                'protected_members': self._extract_members(class_body, 'protected'),
                'private_members': self._extract_members(class_body, 'private'),
                'signals': self._extract_signals_slots(class_body, 'signals'),
                'slots': self._extract_signals_slots(class_body, 'slots'),
                'enums': self._extract_enums(class_body),
                'constants': self._extract_constants(class_body)
            }
            
            self.classes.append(class_info)
        
        return self.classes
    
    def _find_matching_brace(self, start_pos: int) -> int:
        """Находит закрывающую скобку для класса"""
        brace_count = 1
        pos = start_pos
        
        while pos < len(self.content) and brace_count > 0:
            if self.content[pos] == '{':
                brace_count += 1
            elif self.content[pos] == '}':
                brace_count -= 1
            pos += 1
        
        return pos if brace_count == 0 else -1
    
    def _parse_inheritance(self, inheritance_str: str) -> List[Dict]:
        """Парсит строку наследования"""
        result = []
        # Разбиваем по запятым, но учитываем вложенные шаблоны
        parts = re.split(r',\s*(?![^<]*>)', inheritance_str)
        
        for part in parts:
            part = part.strip()
            if not part:
                continue
            
            # Извлекаем тип наследования и имя базового класса
            access_specifier = 'private'
            base_class = part
            
            if part.startswith('public '):
                access_specifier = 'public'
                base_class = part[7:].strip()
            elif part.startswith('protected '):
                access_specifier = 'protected'
                base_class = part[10:].strip()
            
            result.append({
                'access': access_specifier,
                'class': base_class
            })
        
        return result
    
    def _extract_brief(self, text: str) -> str:
        """Извлекает краткое описание из комментария @brief"""
        # Ищем последний @brief перед объявлением класса
        brief_match = re.search(r'///?\s*@brief\s+(.+?)(?:\n\s*///?|$)', text, re.DOTALL)
        if brief_match:
            return brief_match.group(1).strip()
        return ""
    
    def _extract_constructors(self, class_body: str, class_name: str) -> List[Dict]:
        """Извлекает конструкторы класса"""
        constructors = []
        
        # Паттерн для конструкторов (имя класса с параметрами)
        pattern = rf'{class_name}\s*\(([^)]*)\)\s*(?:\s*:\s*([^{{]+))?'
        
        for match in re.finditer(pattern, class_body):
            params = self._parse_parameters(match.group(1))
            initializer_list = match.group(2) if match.group(2) else None
            
            constructors.append({
                'name': class_name,
                'params': params,
                'initializer_list': initializer_list,
                'is_explicit': 'explicit' in match.group(0),
                'is_default': '= default' in match.group(0),
                'is_delete': '= delete' in match.group(0)
            })
        
        # Ищем конструктор копирования
        copy_pattern = rf'{class_name}\s*\(\s*const\s+{class_name}\s*&'
        if re.search(copy_pattern, class_body):
            constructors.append({
                'name': f'{class_name}(const {class_name}&)',
                'params': [{'type': f'const {class_name}&', 'name': 'other'}],
                'is_copy': True
            })
        
        # Ищем конструктор перемещения
        move_pattern = rf'{class_name}\s*\(\s*{class_name}\s*&&'
        if re.search(move_pattern, class_body):
            constructors.append({
                'name': f'{class_name}({class_name}&&)',
                'params': [{'type': f'{class_name}&&', 'name': 'other'}],
                'is_move': True
            })
        
        return constructors
    
    def _extract_destructor(self, class_body: str, class_name: str) -> Optional[Dict]:
        """Извлекает деструктор класса"""
        pattern = rf'~{class_name}\s*\(\s*\)'
        match = re.search(pattern, class_body)
        
        if match:
            return {
                'name': f'~{class_name}()',
                'is_virtual': 'virtual' in match.group(0),
                'is_default': '= default' in match.group(0),
                'is_delete': '= delete' in match.group(0)
            }
        return None
    
    def _extract_methods(self, class_body: str, access: str, class_name: str) -> List[Dict]:
        """Извлекает методы с указанной областью видимости (ИСПРАВЛЕНО: добавлен class_name)"""
        methods = []
        
        # Находим секцию с указанной областью видимости
        section_pattern = rf'{access}:\s*(.*?)(?=\n\s*(?:public|protected|private|signals|slots):|\n\s*\}})'
        section_match = re.search(section_pattern, class_body, re.DOTALL)
        
        if not section_match:
            return methods
        
        section_content = section_match.group(1)
        
        # Ищем методы (виртуальные и обычные)
        method_pattern = r'(?:virtual\s+)?([\w:<>\s*&]+?)\s+(\w+)\s*\(([^)]*)\)\s*(const)?\s*(override|final|noexcept)?'
        
        for match in re.finditer(method_pattern, section_content):
            return_type = match.group(1).strip()
            method_name = match.group(2)
            params_str = match.group(3)
            is_const = match.group(4) is not None
            specifier = match.group(5) if match.group(5) else ""
            
            # ИСПРАВЛЕНО: используем переданный class_name для фильтрации
            if method_name == class_name or method_name.startswith('~') or method_name.startswith('operator'):
                continue
            
            params = self._parse_parameters(params_str)
            
            methods.append({
                'name': method_name,
                'return_type': return_type,
                'params': params,
                'is_virtual': 'virtual' in match.group(0),
                'is_const': is_const,
                'specifier': specifier,
                'access': access
            })
        
        return methods
    
    def _extract_members(self, class_body: str, access: str) -> List[Dict]:
        """Извлекает члены-данные с указанной областью видимости"""
        members = []
        
        # Находим секцию
        section_pattern = rf'{access}:\s*(.*?)(?=\n\s*(?:public|protected|private|signals|slots):|\n\s*\}})'
        section_match = re.search(section_pattern, class_body, re.DOTALL)
        
        if not section_match:
            return members
        
        section_content = section_match.group(1)
        
        # Ищем объявления переменных (исключая методы)
        member_pattern = r'(?:(mutable|static|const)\s+)?([\w:<>\s*&]+?)\s+(\w+)(?:\s*=\s*([^;]+))?;'
        
        for match in re.finditer(member_pattern, section_content):
            is_mutable = match.group(1) == 'mutable'
            is_static = match.group(1) == 'static'
            is_const = match.group(1) == 'const' or 'const' in match.group(2)
            var_type = match.group(2).strip()
            var_name = match.group(3)
            default_value = match.group(4) if match.group(4) else None
            
            # Пропускаем объявления типов и перечислений
            if var_type in ['using', 'typedef', 'enum']:
                continue
            
            members.append({
                'name': var_name,
                'type': var_type,
                'is_mutable': is_mutable,
                'is_static': is_static,
                'is_const': is_const,
                'default_value': default_value,
                'access': access
            })
        
        return members
    
    def _extract_signals_slots(self, class_body: str, section: str) -> List[Dict]:
        """Извлекает сигналы или слоты"""
        items = []
        
        # Находим секцию
        section_pattern = rf'{section}:\s*(.*?)(?=\n\s*(?:public|protected|private|signals|slots):|\n\s*\}})'
        section_match = re.search(section_pattern, class_body, re.DOTALL)
        
        if not section_match:
            return items
        
        section_content = section_match.group(1)
        
        # Ищем объявления сигналов/слотов
        item_pattern = r'void\s+(\w+)\s*\(([^)]*)\);'
        
        for match in re.finditer(item_pattern, section_content):
            name = match.group(1)
            params_str = match.group(2)
            params = self._parse_parameters(params_str)
            
            items.append({
                'name': name,
                'params': params,
                'type': 'signal' if section == 'signals' else 'slot'
            })
        
        return items
    
    def _extract_enums(self, class_body: str) -> List[Dict]:
        """Извлекает перечисления"""
        enums = []
        
        # Ищем объявления enum
        enum_pattern = r'enum\s+(?:class\s+)?(\w+)\s*\{([^}]+)\};'
        
        for match in re.finditer(enum_pattern, class_body):
            name = match.group(1)
            values_str = match.group(2)
            
            # Извлекаем значения
            values = []
            for val in values_str.split(','):
                val = val.strip()
                if val:
                    # Убираем инициализатор (= значение)
                    val_name = val.split('=')[0].strip()
                    values.append(val_name)
            
            enums.append({
                'name': name,
                'values': values
            })
        
        return enums
    
    def _extract_constants(self, class_body: str) -> List[Dict]:
        """Извлекает статические константы"""
        constants = []
        
        # Ищем static constexpr
        pattern = r'static\s+constexpr\s+([\w:<>\s]+?)\s+(\w+)\s*=\s*([^;]+);'
        
        for match in re.finditer(pattern, class_body):
            const_type = match.group(1).strip()
            const_name = match.group(2)
            const_value = match.group(3).split('//')[0].strip()
            
            # Фильтруем только значимые константы
            if (const_name.isupper() and len(const_name) >= 3) or \
               const_name.startswith(('DEFAULT_', 'MIN_', 'MAX_')):
                constants.append({
                    'name': const_name,
                    'type': const_type,
                    'value': const_value
                })
        
        return constants
    
    def _parse_parameters(self, params_str: str) -> List[Dict]:
        """Парсит строку параметров метода"""
        if not params_str or params_str.strip() == '':
            return []
        
        params = []
        # Разбиваем по запятым, но учитываем вложенные скобки и шаблоны
        current_param = ""
        depth = 0
        
        for char in params_str:
            if char == '<' or char == '(':
                depth += 1
            elif char == '>' or char == ')':
                depth -= 1
            elif char == ',' and depth == 0:
                if current_param.strip():
                    params.append(self._parse_single_parameter(current_param.strip()))
                current_param = ""
                continue
            current_param += char
        
        if current_param.strip():
            params.append(self._parse_single_parameter(current_param.strip()))
        
        return params
    
    def _parse_single_parameter(self, param_str: str) -> Dict:
        """Парсит один параметр"""
        # Разбиваем на тип и имя
        parts = param_str.rsplit(' ', 1)
        
        if len(parts) == 2:
            param_type = parts[0].strip()
            param_name = parts[1].strip()
        else:
            param_type = param_str.strip()
            param_name = ""
        
        return {
            'type': param_type,
            'name': param_name
        }

def build_tree_with_full_content(root: Path, prefix: str = '', is_last: bool = True) -> list:
    """Рекурсивно строит дерево проекта с полным содержимым файлов"""
    lines = []
    
    # Добавляем текущую директорию
    if root == root.parent:
        lines.append(f"{root.name}/")
    else:
        connector = '└── ' if is_last else '├── '
        lines.append(f"{prefix}{connector}{root.name}/")
        prefix += '    ' if is_last else '│   '
    
    # Получаем все элементы в директории
    try:
        entries = sorted(root.iterdir(), key=lambda x: (x.is_file(), x.name.lower()))
    except PermissionError:
        return lines
    
    # Фильтруем: исключаем папки TEMP/TEST, оставляем только .h файлы и разрешённые директории
    dirs = []
    files = []
    
    for entry in entries:
        if entry.is_dir():
            if not should_exclude(entry):
                dirs.append(entry)
        elif entry.is_file() and entry.suffix == '.h':
            files.append(entry)
    
    # Обрабатываем директории
    for i, dir_path in enumerate(dirs):
        is_last_dir = (i == len(dirs) - 1) and (len(files) == 0)
        lines.extend(build_tree_with_full_content(dir_path, prefix, is_last_dir))
    
    # Обрабатываем файлы
    for i, file_path in enumerate(files):
        is_last_file = (i == len(files) - 1)
        connector = '└── ' if is_last_file else '├── '
        lines.append(f"{prefix}{connector}{file_path.name}")
        
        # ИСПРАВЛЕНО: определяем indent ДО блока try
        indent = prefix + '    '
        
        try:
            content = file_path.read_text(encoding='utf-8')
            parser = CppClassParser(content)
            classes = parser.parse()
            
            # Добавляем информацию о каждом классе
            for cls in classes:
                lines.append(f"{indent}└── 📦 {cls['type']} {cls['name']}")
                
                # Наследование
                if cls['inheritance']:
                    for base in cls['inheritance']:
                        lines.append(f"{indent}    └── 📌 Наследует: {base['access']} {base['class']}")
                
                # Краткое описание
                if cls['brief']:
                    lines.append(f"{indent}    └── @brief {cls['brief']}")
                
                # Конструкторы
                if cls['constructors']:
                    lines.append(f"{indent}    └── 🏗️ Конструкторы:")
                    for ctor in cls['constructors']:
                        params_str = ', '.join([f"{p['type']} {p['name']}".strip() for p in ctor['params']])
                        ctor_line = f"{indent}        └── {ctor['name']}({params_str})"
                        if ctor.get('is_explicit'):
                            ctor_line += " [explicit]"
                        if ctor.get('is_default'):
                            ctor_line += " [= default]"
                        if ctor.get('is_delete'):
                            ctor_line += " [= delete]"
                        if ctor.get('initializer_list'):
                            ctor_line += f" : {ctor['initializer_list'][:50]}..."
                        lines.append(ctor_line)
                
                # Деструктор
                if cls['destructor']:
                    dtor_line = f"{indent}    └── 🗑️ {cls['destructor']['name']}"
                    if cls['destructor']['is_virtual']:
                        dtor_line += " [virtual]"
                    if cls['destructor']['is_default']:
                        dtor_line += " [= default]"
                    if cls['destructor']['is_delete']:
                        dtor_line += " [= delete]"
                    lines.append(dtor_line)
                
                # Публичные методы
                if cls['public_methods']:
                    lines.append(f"{indent}    └── 📡 Публичные методы:")
                    for method in cls['public_methods']:
                        params_str = ', '.join([f"{p['type']} {p['name']}".strip() for p in method['params']])
                        modifiers = []
                        if method['is_virtual']:
                            modifiers.append('virtual')
                        if method['is_const']:
                            modifiers.append('const')
                        if method['specifier']:
                            modifiers.append(method['specifier'])
                        
                        modifiers_str = ' '.join(modifiers)
                        if modifiers_str:
                            modifiers_str = f" [{modifiers_str}]"
                        
                        lines.append(f"{indent}        └── {method['return_type']} {method['name']}({params_str}){modifiers_str}")
                
                # Защищённые методы
                if cls['protected_methods']:
                    lines.append(f"{indent}    └── 🔒 Защищённые методы:")
                    for method in cls['protected_methods']:
                        params_str = ', '.join([f"{p['type']} {p['name']}".strip() for p in method['params']])
                        modifiers = []
                        if method['is_virtual']:
                            modifiers.append('virtual')
                        if method['is_const']:
                            modifiers.append('const')
                        
                        modifiers_str = ' '.join(modifiers)
                        if modifiers_str:
                            modifiers_str = f" [{modifiers_str}]"
                        
                        lines.append(f"{indent}        └── {method['return_type']} {method['name']}({params_str}){modifiers_str}")
                
                # Публичные члены-данные
                if cls['public_members']:
                    lines.append(f"{indent}    └── 💾 Публичные члены-данные:")
                    for member in cls['public_members']:
                        member_line = f"{indent}        └── {member['type']} {member['name']}"
                        if member['is_static']:
                            member_line += " [static]"
                        if member['is_const']:
                            member_line += " [const]"
                        if member['is_mutable']:
                            member_line += " [mutable]"
                        if member['default_value']:
                            member_line += f" = {member['default_value']}"
                        lines.append(member_line)
                
                # Сигналы
                if cls['signals']:
                    lines.append(f"{indent}    └── 🔊 Сигналы:")
                    for signal in cls['signals']:
                        params_str = ', '.join([f"{p['type']} {p['name']}".strip() for p in signal['params']])
                        lines.append(f"{indent}        └── void {signal['name']}({params_str})")
                
                # Слоты
                if cls['slots']:
                    lines.append(f"{indent}    └── 🎯 Слоты:")
                    for slot in cls['slots']:
                        params_str = ', '.join([f"{p['type']} {p['name']}".strip() for p in slot['params']])
                        lines.append(f"{indent}        └── void {slot['name']}({params_str})")
                
                # Перечисления
                if cls['enums']:
                    lines.append(f"{indent}    └── 🔢 Перечисления:")
                    for enum in cls['enums']:
                        lines.append(f"{indent}        └── enum {enum['name']}")
                        for value in enum['values'][:5]:  # Первые 5 значений
                            lines.append(f"{indent}            └── {value}")
                        if len(enum['values']) > 5:
                            lines.append(f"{indent}            └── ... (+{len(enum['values'])-5} значений)")
                
                # Константы
                if cls['constants']:
                    lines.append(f"{indent}    └── 🔑 Константы:")
                    for const in cls['constants']:
                        lines.append(f"{indent}        └── {const['type']} {const['name']} = {const['value']}")
        
        except Exception as e:
            # ИСПРАВЛЕНО: indent уже определён выше
            lines.append(f"{indent}    └── ⚠️ Ошибка парсинга: {type(e).__name__}: {str(e)[:80]}")
    
    return lines

def main():
    parser = argparse.ArgumentParser(description='Генерация ПОЛНОГО контекста проекта с детальной информацией о классах')
    parser.add_argument('--root', default='.', help='Корневая директория проекта (по умолчанию: текущая)')
    parser.add_argument('--output', required=True, help='Файл для сохранения результата')
    parser.add_argument('-v', '--verbose', action='store_true', help='Подробный вывод')
    
    args = parser.parse_args()
    
    root_path = Path(args.root).resolve()
    
    if not root_path.exists():
        print(f"Ошибка: директория '{args.root}' не существует", file=sys.stderr)
        sys.exit(1)
    
    # Проверка: если путь абсолютный и начинается с корня — предупреждение
    output_path = Path(args.output)
    if output_path.is_absolute() and output_path.parts[0] == '/':
        print(f"⚠️  Внимание: путь '{output_path}' является абсолютным и начинается с корня системы.", file=sys.stderr)
        print(f"   Возможно, вы хотели использовать относительный путь?", file=sys.stderr)
        print(f"   Пример: './doxygen/context/PROJECT_CONTEXT.md'", file=sys.stderr)
        print(f"   Продолжить? (y/n): ", end='', file=sys.stderr)
        if input().strip().lower() != 'y':
            sys.exit(1)
    
    if args.verbose:
        print(f"Сканирование проекта: {root_path}")
        print(f"Исключаются папки: TEMP, TEST")
        print(f"Включаются файлы: *.h")
        print(f"Извлекается: полная информация о классах, методах, членах, наследовании, сигналах, слотах")
    
    # Строим дерево с полным содержимым
    tree_lines = build_tree_with_full_content(root_path)
    
    # Добавляем заголовок
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    output_lines = [
        "# ПОЛНЫЙ КОНТЕКСТ ПРОЕКТА FCPlotter",
        f"*Сгенерировано: {timestamp}*",
        "",
        "## 📁 Структура проекта с ПОЛНЫМ содержимым .h файлов",
        "",
        "```",
        *tree_lines,
        "```",
        "",
        f"*Корневая директория: {root_path.name}*",
        "*Исключены папки: TEMP, TEST*",
        "*Включены файлы: *.h*",
        "*Извлечено: классы, наследование, конструкторы, методы, члены-данные, сигналы, слоты, перечисления, константы*"
    ]
    
    # Создание директории с обработкой ошибок
    try:
        output_path.parent.mkdir(parents=True, exist_ok=True)
    except PermissionError:
        print(f"❌ Ошибка: нет прав на создание директории '{output_path.parent}'", file=sys.stderr)
        print(f"   Используйте относительный путь, например: './doxygen/context/PROJECT_CONTEXT.md'", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"❌ Ошибка при создании директории: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Запись файла с обработкой ошибок
    try:
        output_path.write_text('\n'.join(output_lines), encoding='utf-8')
    except PermissionError:
        print(f"❌ Ошибка: нет прав на запись в '{output_path}'", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"❌ Ошибка при записи файла: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Выводим статистику
    file_count = sum(1 for line in tree_lines if '.h' in line)
    class_count = sum(1 for line in tree_lines if '📦 class' in line or '📦 struct' in line)
    method_count = sum(1 for line in tree_lines if '📡' in line or '🔒' in line)
    member_count = sum(1 for line in tree_lines if '💾' in line)
    signal_count = sum(1 for line in tree_lines if '🔊' in line)
    slot_count = sum(1 for line in tree_lines if '🎯' in line)
    
    print(f"✓ ПОЛНЫЙ контекст проекта сохранён: {output_path}")
    print(f"  Файлов .h: {file_count}")
    print(f"  Классов/структур: {class_count}")
    print(f"  Методов: {method_count}")
    print(f"  Членов-данных: {member_count}")
    print(f"  Сигналов: {signal_count}")
    print(f"  Слотов: {slot_count}")
    
    if args.verbose:
        print("\nПример содержимого (первые 100 строк):")
        print('\n'.join(output_lines[:100]))

if __name__ == '__main__':
    main()
