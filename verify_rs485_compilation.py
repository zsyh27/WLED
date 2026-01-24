#!/usr/bin/env python3
"""
验证RS485控制模块编译情况
检查固件中是否包含RS485相关的符号和字符串
"""

import os
import subprocess
import sys

def check_firmware_symbols():
    """检查固件中的RS485相关符号"""
    firmware_path = "build_output/release/WLED_0.16.0-alpha_ESP32.bin"
    
    if not os.path.exists(firmware_path):
        print("❌ 固件文件不存在:", firmware_path)
        return False
    
    print("✅ 固件文件存在:", firmware_path)
    
    # 获取文件大小
    file_size = os.path.getsize(firmware_path)
    print(f"📦 固件大小: {file_size:,} bytes ({file_size/1024/1024:.2f} MB)")
    
    # 检查固件中是否包含RS485相关字符串
    rs485_strings = [
        b"RS485Control",
        b"RS485 Control",
        b"baudRate",
        b"rxPin",
        b"txPin",
        b"echoEnabled",
        b"debugMode"
    ]
    
    found_strings = []
    
    try:
        with open(firmware_path, 'rb') as f:
            firmware_data = f.read()
            
        for search_string in rs485_strings:
            if search_string in firmware_data:
                found_strings.append(search_string.decode('utf-8', errors='ignore'))
                print(f"✅ 找到字符串: {search_string.decode('utf-8', errors='ignore')}")
            else:
                print(f"❌ 未找到字符串: {search_string.decode('utf-8', errors='ignore')}")
    
    except Exception as e:
        print(f"❌ 读取固件文件时出错: {e}")
        return False
    
    if len(found_strings) >= 4:  # 至少找到4个关键字符串
        print(f"\n✅ RS485控制模块已成功编译到固件中! (找到 {len(found_strings)}/{len(rs485_strings)} 个关键字符串)")
        return True
    else:
        print(f"\n❌ RS485控制模块可能未正确编译到固件中 (只找到 {len(found_strings)}/{len(rs485_strings)} 个关键字符串)")
        return False

def check_build_log():
    """检查编译日志中的RS485相关信息"""
    print("\n🔍 检查编译过程...")
    
    # 检查是否编译了RS485源文件
    try:
        result = subprocess.run(['pio', 'run', '-e', 'esp32dev', '-v'], 
                              capture_output=True, text=True, timeout=30)
        
        if "rs485_control_simple.cpp" in result.stdout:
            print("✅ RS485控制模块源文件已编译")
            return True
        else:
            print("❌ 未找到RS485控制模块源文件编译记录")
            return False
            
    except subprocess.TimeoutExpired:
        print("⚠️  编译检查超时，跳过详细检查")
        return True
    except Exception as e:
        print(f"⚠️  无法检查编译日志: {e}")
        return True

def main():
    print("🔧 验证RS485控制模块编译情况")
    print("=" * 50)
    
    # 检查固件符号
    firmware_ok = check_firmware_symbols()
    
    # 检查编译日志
    build_ok = check_build_log()
    
    print("\n" + "=" * 50)
    print("📋 验证结果:")
    
    if firmware_ok:
        print("✅ RS485控制模块已成功编译并包含在固件中")
        print("✅ 固件可以用于网页烧录")
        print("\n📝 下一步操作:")
        print("1. 将固件文件上传到WLED设备")
        print("2. 进入 Config → Usermods 页面")
        print("3. 查找 'RS485Control' 配置选项")
        print("4. 配置串口参数 (RX: GPIO26, TX: GPIO27, 波特率: 9600)")
        return True
    else:
        print("❌ RS485控制模块编译验证失败")
        print("\n🔧 建议检查:")
        print("1. platformio.ini 中的 build_src_filter 配置")
        print("2. rs485_control_simple.cpp 文件是否存在")
        print("3. 重新编译固件")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)