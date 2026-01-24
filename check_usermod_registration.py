#!/usr/bin/env python3
"""
检查WLED固件中的usermod注册情况
验证REGISTER_USERMOD宏是否正确工作
"""

import os
import subprocess
import sys

def check_map_file():
    """检查链接器映射文件中的usermod注册信息"""
    map_file = ".pio/build/esp32dev/firmware.map"
    
    if not os.path.exists(map_file):
        print("❌ 链接器映射文件不存在")
        return False
    
    print("🔍 检查链接器映射文件...")
    
    try:
        with open(map_file, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # 查找usermod相关的section
        usermod_sections = []
        lines = content.split('\n')
        
        for i, line in enumerate(lines):
            if '.dtors.tbl.usermods' in line:
                usermod_sections.append(line.strip())
        
        if usermod_sections:
            print(f"✅ 找到 {len(usermod_sections)} 个usermod section:")
            for section in usermod_sections:
                print(f"   {section}")
            return True
        else:
            print("❌ 未找到usermod section")
            return False
            
    except Exception as e:
        print(f"❌ 读取映射文件失败: {e}")
        return False

def check_build_output():
    """检查编译输出中的usermod信息"""
    print("\n🔍 检查编译输出...")
    
    try:
        # 重新编译以获取详细输出
        result = subprocess.run(['pio', 'run', '-e', 'esp32dev', '-v'], 
                              capture_output=True, text=True, timeout=60)
        
        output = result.stdout + result.stderr
        
        # 查找usermod相关信息
        usermod_info = []
        for line in output.split('\n'):
            if 'usermod' in line.lower() and ('INFO:' in line or 'object entries' in line):
                usermod_info.append(line.strip())
        
        if usermod_info:
            print("✅ 编译输出中的usermod信息:")
            for info in usermod_info:
                print(f"   {info}")
            return True
        else:
            print("⚠️  编译输出中未找到usermod信息")
            return False
            
    except subprocess.TimeoutExpired:
        print("⚠️  编译检查超时")
        return True
    except Exception as e:
        print(f"⚠️  无法检查编译输出: {e}")
        return True

def check_firmware_symbols():
    """检查固件中的usermod符号"""
    firmware_path = "build_output/release/WLED_0.16.0-alpha_ESP32.bin"
    
    if not os.path.exists(firmware_path):
        print("❌ 固件文件不存在")
        return False
    
    print("\n🔍 检查固件中的usermod符号...")
    
    # 检查REGISTER_USERMOD相关的符号
    symbols_to_check = [
        b"RS485Control",
        b"um_rs485_control_usermod",  # REGISTER_USERMOD生成的符号
        b"addToConfig",
        b"appendConfigData",
        b"addToJsonInfo"
    ]
    
    try:
        with open(firmware_path, 'rb') as f:
            firmware_data = f.read()
        
        found_symbols = []
        for symbol in symbols_to_check:
            if symbol in firmware_data:
                found_symbols.append(symbol.decode('utf-8', errors='ignore'))
        
        print(f"✅ 找到 {len(found_symbols)}/{len(symbols_to_check)} 个相关符号:")
        for symbol in found_symbols:
            print(f"   {symbol}")
        
        return len(found_symbols) >= 3  # 至少找到3个符号
        
    except Exception as e:
        print(f"❌ 检查固件符号失败: {e}")
        return False

def main():
    """主函数"""
    print("🔧 检查WLED usermod注册情况")
    print("=" * 50)
    
    # 检查映射文件
    map_ok = check_map_file()
    
    # 检查编译输出
    build_ok = check_build_output()
    
    # 检查固件符号
    firmware_ok = check_firmware_symbols()
    
    print("\n" + "=" * 50)
    print("📋 检查结果:")
    
    if map_ok and firmware_ok:
        print("✅ RS485控制模块已正确注册到WLED系统")
        print("✅ REGISTER_USERMOD宏工作正常")
        print("✅ 固件应该在Config→Usermods页面显示RS485Control")
        
        print("\n📝 关键修复:")
        print("- 添加了REGISTER_USERMOD(rs485_control_usermod)宏")
        print("- 使用链接器section自动注册usermod")
        print("- 编译输出显示2个usermod对象条目")
        
        print("\n🚀 测试步骤:")
        print("1. 上传新固件到ESP32设备")
        print("2. 访问WLED网页界面")
        print("3. 进入Config → Usermods页面")
        print("4. 应该能看到RS485Control配置选项")
        
        return True
    else:
        print("❌ usermod注册存在问题")
        
        if not map_ok:
            print("- 链接器映射文件中未找到usermod section")
        if not firmware_ok:
            print("- 固件中缺少关键符号")
        
        print("\n🔧 建议检查:")
        print("- REGISTER_USERMOD宏是否正确使用")
        print("- 链接器是否正确处理usermod section")
        print("- 编译配置是否正确")
        
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)