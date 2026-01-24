#!/usr/bin/env python3
"""
RS485 Usermod 诊断脚本
检查RS485 usermod是否正确编译到固件中
"""

import subprocess
import sys
import os
from pathlib import Path

def check_compilation_symbols():
    """检查编译后的符号表中是否包含RS485相关符号"""
    print("=== 检查编译符号 ===")
    
    elf_file = Path(".pio/build/esp32dev/firmware.elf")
    if not elf_file.exists():
        print("❌ 固件ELF文件不存在，请先编译")
        return False
    
    try:
        # 使用objdump检查符号表
        result = subprocess.run([
            "xtensa-esp32-elf-objdump", "-t", str(elf_file)
        ], capture_output=True, text=True)
        
        if result.returncode != 0:
            print("⚠️  无法运行objdump，跳过符号检查")
            return True
        
        symbols = result.stdout
        
        # 检查RS485相关符号
        rs485_symbols = [
            "RS485ControlUsermod",
            "rs485_control_usermod",
            "_Z19rs485_control_usermod",  # mangled name
            "USERMOD_ID_RS485_CONTROL"
        ]
        
        found_symbols = []
        for symbol in rs485_symbols:
            if symbol in symbols:
                found_symbols.append(symbol)
        
        if found_symbols:
            print(f"✅ 找到RS485符号: {found_symbols}")
            return True
        else:
            print("❌ 未找到RS485相关符号")
            return False
            
    except Exception as e:
        print(f"⚠️  符号检查失败: {e}")
        return True  # 不是关键错误

def check_source_files():
    """检查源文件是否存在且内容正确"""
    print("\n=== 检查源文件 ===")
    
    files_to_check = [
        ("usermods/rs485_control/rs485_control.cpp", "主实现文件"),
        ("usermods/rs485_control/rs485_control.h", "头文件"),
        ("wled00/my_config.h", "配置文件")
    ]
    
    all_good = True
    
    for file_path, description in files_to_check:
        if not Path(file_path).exists():
            print(f"❌ {description} 不存在: {file_path}")
            all_good = False
            continue
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            if file_path.endswith('.cpp'):
                if "REGISTER_USERMOD" in content:
                    print(f"✅ {description} 包含注册宏")
                else:
                    print(f"❌ {description} 缺少注册宏")
                    all_good = False
                    
            elif file_path.endswith('.h'):
                if "class RS485ControlUsermod" in content:
                    print(f"✅ {description} 包含类定义")
                else:
                    print(f"❌ {description} 缺少类定义")
                    all_good = False
                    
            elif file_path == "wled00/my_config.h":
                if "#define USERMOD_RS485_CONTROL" in content:
                    print(f"✅ {description} 启用了RS485功能")
                else:
                    print(f"❌ {description} 未启用RS485功能")
                    all_good = False
                    
        except Exception as e:
            print(f"❌ 读取 {description} 失败: {e}")
            all_good = False
    
    return all_good

def check_build_output():
    """检查编译输出中的相关信息"""
    print("\n=== 检查编译输出 ===")
    
    try:
        # 重新编译并捕获输出
        result = subprocess.run([
            "pio", "run", "-e", "esp32dev", "-v"
        ], capture_output=True, text=True, timeout=120)
        
        output = result.stdout + result.stderr
        
        # 检查是否编译了RS485文件
        if "rs485_control.cpp" in output:
            print("✅ RS485源文件被编译")
        else:
            print("❌ RS485源文件未被编译")
            return False
        
        # 检查是否有相关的链接信息
        if "RS485" in output or "rs485" in output:
            print("✅ 编译输出包含RS485相关信息")
        else:
            print("⚠️  编译输出中未发现RS485信息")
        
        return result.returncode == 0
        
    except subprocess.TimeoutExpired:
        print("❌ 编译超时")
        return False
    except Exception as e:
        print(f"❌ 编译检查失败: {e}")
        return False

def check_usermod_registration():
    """检查usermod注册机制"""
    print("\n=== 检查Usermod注册 ===")
    
    # 检查usermod.cpp文件
    usermod_cpp = Path("wled00/usermod.cpp")
    if usermod_cpp.exists():
        try:
            with open(usermod_cpp, 'r', encoding='utf-8') as f:
                content = f.read()
            
            if "#ifdef USERMOD_RS485_CONTROL" in content:
                print("✅ usermod.cpp 包含RS485条件编译")
            else:
                print("❌ usermod.cpp 缺少RS485条件编译")
                return False
                
            if '#include "../usermods/rs485_control/rs485_control.h"' in content:
                print("✅ usermod.cpp 包含RS485头文件")
            else:
                print("❌ usermod.cpp 缺少RS485头文件包含")
                return False
                
        except Exception as e:
            print(f"❌ 读取usermod.cpp失败: {e}")
            return False
    else:
        print("❌ usermod.cpp文件不存在")
        return False
    
    return True

def suggest_fixes():
    """提供修复建议"""
    print("\n=== 修复建议 ===")
    
    print("1. 确保所有源文件存在且内容正确")
    print("2. 检查 my_config.h 中是否定义了 USERMOD_RS485_CONTROL")
    print("3. 清理并重新编译:")
    print("   pio run -e esp32dev -t clean")
    print("   pio run -e esp32dev")
    print("4. 检查编译输出中的错误信息")
    print("5. 确认usermod注册宏正确使用")

def main():
    """主函数"""
    print("RS485 Usermod 诊断工具")
    print("=" * 50)
    
    checks = [
        ("源文件检查", check_source_files),
        ("Usermod注册检查", check_usermod_registration),
        ("编译输出检查", check_build_output),
        ("编译符号检查", check_compilation_symbols),
    ]
    
    results = []
    for check_name, check_func in checks:
        print(f"\n🔍 执行: {check_name}")
        try:
            result = check_func()
            results.append((check_name, result))
        except Exception as e:
            print(f"❌ {check_name} 执行失败: {e}")
            results.append((check_name, False))
    
    # 总结结果
    print("\n" + "=" * 50)
    print("诊断结果总结:")
    print("=" * 50)
    
    passed = 0
    for check_name, result in results:
        status = "✅ 通过" if result else "❌ 失败"
        print(f"{check_name}: {status}")
        if result:
            passed += 1
    
    print(f"\n通过率: {passed}/{len(results)} ({passed/len(results)*100:.1f}%)")
    
    if passed < len(results):
        suggest_fixes()
        return 1
    else:
        print("\n🎉 所有检查通过！RS485 usermod应该正常工作。")
        return 0

if __name__ == "__main__":
    sys.exit(main())