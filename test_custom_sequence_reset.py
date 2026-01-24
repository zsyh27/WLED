#!/usr/bin/env python3
"""
自定义序列效果重置测试脚本
测试通过RS485命令调用预设时，自定义序列效果是否正确从头开始
"""

import serial
import time
import sys

def test_sequence_reset(port='COM5', baudrate=9600, device_address=1):
    """
    测试自定义序列效果的重置功能
    
    Args:
        port: 串口端口
        baudrate: 波特率
        device_address: 设备地址 (1-255)
    """
    
    try:
        # 打开串口连接
        ser = serial.Serial(port, baudrate, timeout=2)
        print(f"已连接到 {port}，波特率 {baudrate}")
        
        # 等待串口稳定
        time.sleep(1)
        
        # 格式化地址为3位数格式
        addr_str = f"{device_address:03d}"
        
        # 测试序列
        test_commands = [
            f"@{addr_str} STATUS",  # 检查设备状态
            f"@{addr_str} PRESET 1",  # 切换到预设1
            f"@{addr_str} PRESET 3",  # 激活自定义序列效果（假设保存在预设3）
        ]
        
        print("\n开始测试自定义序列效果重置功能...")
        print("=" * 50)
        
        for i, command in enumerate(test_commands, 1):
            print(f"\n步骤 {i}: 发送命令 '{command}'")
            
            # 发送命令
            ser.write((command + '\n').encode())
            ser.flush()
            
            # 等待响应
            time.sleep(0.5)
            
            # 读取响应
            response = ""
            while ser.in_waiting > 0:
                response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                time.sleep(0.1)
            
            if response.strip():
                print(f"响应: {response.strip()}")
            else:
                print("无响应")
            
            # 在激活预设3后等待更长时间观察效果
            if "PRESET 3" in command:
                print("等待5秒观察序列效果...")
                time.sleep(5)
                
                # 再次激活预设3测试重置
                print(f"\n重复测试: 再次发送 '{command}'")
                ser.write((command + '\n').encode())
                ser.flush()
                time.sleep(0.5)
                
                response = ""
                while ser.in_waiting > 0:
                    response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                    time.sleep(0.1)
                
                if response.strip():
                    print(f"响应: {response.strip()}")
                
                print("观察LED是否从序列开始位置（LED 1）重新开始...")
                print("预期序列：1→3→5→7→9→8→6→4→2")
        
        print("\n" + "=" * 50)
        print("测试完成！")
        print("\n验证要点：")
        print("1. 每次发送 'PRESET 3' 命令时")
        print("2. LED序列应该从第1个LED开始")
        print("3. 而不是从之前停止的位置继续")
        print("4. 序列顺序：1→3→5→7→9→8→6→4→2")
        
    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("请检查：")
        print("1. 串口端口是否正确")
        print("2. 设备是否已连接")
        print("3. 波特率是否匹配")
        return False
        
    except KeyboardInterrupt:
        print("\n测试被用户中断")
        return False
        
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        return False
        
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("串口连接已关闭")
    
    return True

def main():
    """主函数"""
    print("自定义序列效果重置测试")
    print("=" * 30)
    
    # 可以根据实际情况修改这些参数
    port = 'COM5'  # Windows串口
    baudrate = 9600
    device_address = 1
    
    # 如果有命令行参数，使用命令行参数
    if len(sys.argv) > 1:
        port = sys.argv[1]
    if len(sys.argv) > 2:
        baudrate = int(sys.argv[2])
    if len(sys.argv) > 3:
        device_address = int(sys.argv[3])
    
    print(f"测试参数:")
    print(f"  串口: {port}")
    print(f"  波特率: {baudrate}")
    print(f"  设备地址: {device_address} (格式: @{device_address:03d})")
    print()
    
    # 运行测试
    success = test_sequence_reset(port, baudrate, device_address)
    
    if success:
        print("测试执行完成")
    else:
        print("测试执行失败")
        sys.exit(1)

if __name__ == "__main__":
    main()