#!/usr/bin/env python3
"""
RS485地址范围测试脚本
测试1-255地址范围的通信功能
"""

import serial
import time
import sys

def test_address_range(port='COM5', baudrate=9600, test_addresses=None):
    """
    测试RS485地址范围功能
    
    Args:
        port: 串口端口
        baudrate: 波特率
        test_addresses: 要测试的地址列表，默认测试关键地址
    """
    
    if test_addresses is None:
        # 测试关键地址：边界值和典型值
        test_addresses = [1, 9, 10, 99, 100, 255]
    
    try:
        # 打开串口连接
        ser = serial.Serial(port, baudrate, timeout=2)
        print(f"已连接到 {port}，波特率 {baudrate}")
        print(f"测试地址范围: {test_addresses}")
        
        # 等待串口稳定
        time.sleep(1)
        
        print("\n开始RS485地址范围测试...")
        print("=" * 60)
        
        results = {}
        
        for addr in test_addresses:
            print(f"\n测试地址 {addr} (格式: @{addr:03d})")
            print("-" * 30)
            
            # 格式化地址
            addr_str = f"{addr:03d}"
            
            # 测试命令
            test_commands = [
                f"@{addr_str} STATUS",
                f"@{addr_str} ADDRESS?",
                f"@{addr_str} ECHO Test_{addr}"
            ]
            
            addr_results = []
            
            for cmd in test_commands:
                print(f"发送: {cmd}")
                
                # 发送命令
                ser.write((cmd + '\n').encode())
                ser.flush()
                
                # 等待响应
                time.sleep(0.5)
                
                # 读取响应
                response = ""
                start_time = time.time()
                while ser.in_waiting > 0 or (time.time() - start_time) < 1.0:
                    if ser.in_waiting > 0:
                        response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                    time.sleep(0.1)
                
                if response.strip():
                    print(f"响应: {response.strip()}")
                    addr_results.append(True)
                else:
                    print("无响应")
                    addr_results.append(False)
                
                time.sleep(0.2)  # 命令间隔
            
            # 记录结果
            success_rate = sum(addr_results) / len(addr_results) * 100
            results[addr] = {
                'success_rate': success_rate,
                'responses': addr_results
            }
            
            print(f"地址 {addr} 成功率: {success_rate:.1f}%")
        
        # 输出测试总结
        print("\n" + "=" * 60)
        print("测试总结")
        print("=" * 60)
        
        total_success = 0
        total_tests = 0
        
        for addr, result in results.items():
            status = "✅ 通过" if result['success_rate'] >= 66.7 else "❌ 失败"
            print(f"地址 {addr:3d} (@{addr:03d}): {result['success_rate']:5.1f}% {status}")
            total_success += sum(result['responses'])
            total_tests += len(result['responses'])
        
        overall_success = total_success / total_tests * 100 if total_tests > 0 else 0
        print(f"\n总体成功率: {overall_success:.1f}% ({total_success}/{total_tests})")
        
        # 地址格式验证
        print("\n地址格式验证:")
        print("1-9:     @001, @002, ..., @009")
        print("10-99:   @010, @011, ..., @099")
        print("100-255: @100, @101, ..., @255")
        
        return results
        
    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("请检查：")
        print("1. 串口端口是否正确")
        print("2. 设备是否已连接")
        print("3. 波特率是否匹配")
        return None
        
    except KeyboardInterrupt:
        print("\n测试被用户中断")
        return None
        
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        return None
        
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("\n串口连接已关闭")

def test_broadcast_vs_unicast(port='COM5', baudrate=9600, device_address=1):
    """
    测试广播与单播的区别
    
    Args:
        port: 串口端口
        baudrate: 波特率  
        device_address: 设备实际地址
    """
    
    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        print(f"测试广播与单播功能 (设备地址: {device_address})")
        print("=" * 50)
        
        time.sleep(1)
        
        # 测试广播命令 (@000)
        print("\n1. 测试广播命令 (@000)")
        print("预期: 设备执行命令但不回复")
        
        ser.write(b"@000 ECHO Broadcast_Test\n")
        ser.flush()
        time.sleep(1)
        
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        if response.strip():
            print(f"❌ 意外收到回复: {response.strip()}")
        else:
            print("✅ 正确：无回复（广播模式）")
        
        # 测试单播命令
        print(f"\n2. 测试单播命令 (@{device_address:03d})")
        print("预期: 设备执行命令并回复")
        
        addr_str = f"{device_address:03d}"
        ser.write(f"@{addr_str} ECHO Unicast_Test\n".encode())
        ser.flush()
        time.sleep(1)
        
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        if response.strip():
            print(f"✅ 正确收到回复: {response.strip()}")
        else:
            print("❌ 未收到预期回复")
        
        # 测试错误地址
        wrong_addr = 999 if device_address != 999 else 998
        print(f"\n3. 测试错误地址 (@{wrong_addr:03d})")
        print("预期: 设备忽略命令，无回复")
        
        ser.write(f"@{wrong_addr:03d} ECHO Wrong_Address\n".encode())
        ser.flush()
        time.sleep(1)
        
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)
        
        if response.strip():
            print(f"❌ 意外收到回复: {response.strip()}")
        else:
            print("✅ 正确：无回复（地址不匹配）")
            
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

def main():
    """主函数"""
    print("RS485地址范围测试工具")
    print("=" * 30)
    
    # 默认参数
    port = 'COM5'
    baudrate = 9600
    device_address = 1
    
    # 命令行参数处理
    if len(sys.argv) > 1:
        port = sys.argv[1]
    if len(sys.argv) > 2:
        baudrate = int(sys.argv[2])
    if len(sys.argv) > 3:
        device_address = int(sys.argv[3])
    
    print(f"连接参数:")
    print(f"  串口: {port}")
    print(f"  波特率: {baudrate}")
    print(f"  设备地址: {device_address}")
    print()
    
    # 选择测试模式
    print("选择测试模式:")
    print("1. 地址范围测试 (测试关键地址)")
    print("2. 广播与单播测试")
    print("3. 全部测试")
    
    try:
        choice = input("请选择 (1-3): ").strip()
        
        if choice == '1':
            print("\n开始地址范围测试...")
            test_address_range(port, baudrate)
            
        elif choice == '2':
            print("\n开始广播与单播测试...")
            test_broadcast_vs_unicast(port, baudrate, device_address)
            
        elif choice == '3':
            print("\n开始全部测试...")
            test_address_range(port, baudrate)
            print("\n" + "="*60)
            test_broadcast_vs_unicast(port, baudrate, device_address)
            
        else:
            print("无效选择，退出")
            sys.exit(1)
            
    except KeyboardInterrupt:
        print("\n测试被用户中断")
        sys.exit(0)

if __name__ == "__main__":
    main()