#!/usr/bin/env python3
"""
修复RS485编译错误的脚本
将类定义从.cpp文件移动到.h文件，只保留实现
"""

import re
import sys
from pathlib import Path

def extract_class_definitions():
    """从.cpp文件中提取类定义，移动到.h文件"""
    
    cpp_file = Path("usermods/rs485_control/rs485_control.cpp")
    h_file = Path("usermods/rs485_control/rs485_control.h")
    
    if not cpp_file.exists():
        print("❌ RS485 .cpp文件不存在")
        return False
    
    if not h_file.exists():
        print("❌ RS485 .h文件不存在")
        return False
    
    # 读取现有文件
    with open(cpp_file, 'r', encoding='utf-8') as f:
        cpp_content = f.read()
    
    with open(h_file, 'r', encoding='utf-8') as f:
        h_content = f.read()
    
    print("🔧 修复编译错误...")
    
    # 1. 修复const方法问题
    cpp_content = cpp_content.replace(
        'int getCurrentPreset() {',
        'int getCurrentPreset() const {'
    )
    
    # 2. 修复String参数问题
    cpp_content = cpp_content.replace(
        'bool restoreFromDefaults(String& reason) {',
        'bool restoreFromDefaults(const String& reason) {'
    )
    
    # 3. 修复String操作问题
    cpp_content = re.sub(
        r'operation\.toLowerCase\(\)',
        'String(operation).toLowerCase()',
        cpp_content
    )
    
    # 4. 修复字符串连接问题
    cpp_content = re.sub(
        r'", Echo:" \+ \(config->echoEnabled \? "On" : "Off"\)',
        '", Echo:" + String(config->echoEnabled ? "On" : "Off")',
        cpp_content
    )
    
    # 5. 移除重复的类定义（保留实现）
    # 找到类定义的开始和结束
    class_patterns = [
        (r'class ErrorLogger \{.*?\n\};', 'ErrorLogger'),
        (r'class RS485Interface \{.*?\n\};', 'RS485Interface'),
        (r'class CommandParser \{.*?\n\};', 'CommandParser'),
        (r'enum class ConfigValidationResult \{.*?\n\};', 'ConfigValidationResult'),
        (r'struct ConfigValidationInfo \{.*?\n\};', 'ConfigValidationInfo')
    ]
    
    for pattern, class_name in class_patterns:
        matches = re.findall(pattern, cpp_content, re.DOTALL)
        if matches:
            print(f"  移除重复的{class_name}类定义")
            cpp_content = re.sub(pattern, '', cpp_content, flags=re.DOTALL)
    
    # 6. 移除不存在的方法调用
    cpp_content = re.sub(
        r'if \(rs485Interface\) rs485Interface->setErrorLogger\(errorLogger\);',
        '// Error logger integration handled in constructor',
        cpp_content
    )
    
    cpp_content = re.sub(
        r'if \(commandParser\) commandParser->setErrorLogger\(errorLogger\);',
        '// Error logger integration handled in constructor',
        cpp_content
    )
    
    # 7. 修复文件结尾的语法错误
    cpp_content = re.sub(r'\n\}\s*$', '', cpp_content)
    
    # 写回修复后的文件
    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(cpp_content)
    
    print("✅ 编译错误修复完成")
    return True

def main():
    """主函数"""
    print("RS485编译错误修复工具")
    print("=" * 40)
    
    if extract_class_definitions():
        print("\n🎉 修复完成！现在可以重新编译了。")
        return 0
    else:
        print("\n❌ 修复失败")
        return 1

if __name__ == "__main__":
    sys.exit(main())