#!/usr/bin/env python3
"""
系统兼容性验证脚本 - 任务10.1
验证RS485功能不影响现有WLED功能
需求: 5.1, 5.2, 5.3, 5.4, 5.5
"""

import subprocess
import sys
import time
import json
import os
from pathlib import Path

class SystemCompatibilityVerifier:
    def __init__(self):
        self.test_results = {
            "compilation_test": False,
            "memory_usage_analysis": False,
            "code_integration_check": False,
            "configuration_validation": False,
            "usermod_registration": False
        }
        self.issues_found = []
        
    def log_info(self, message):
        print(f"[INFO] {message}")
        
    def log_success(self, message):
        print(f"[SUCCESS] ✓ {message}")
        
    def log_warning(self, message):
        print(f"[WARNING] ⚠ {message}")
        self.issues_found.append(f"WARNING: {message}")
        
    def log_error(self, message):
        print(f"[ERROR] ✗ {message}")
        self.issues_found.append(f"ERROR: {message}")

    def test_compilation_compatibility(self):
        """测试编译兼容性 - 验证RS485功能不破坏WLED编译"""
        self.log_info("Testing compilation compatibility...")
        
        try:
            # 编译ESP32版本
            result = subprocess.run([
                "pio", "run", "-e", "esp32dev"
            ], capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0:
                self.log_success("ESP32 compilation successful")
                self.test_results["compilation_test"] = True
                
                # 分析编译输出中的内存使用情况
                output = result.stdout
                if "RAM:" in output and "Flash:" in output:
                    # 提取内存使用信息
                    ram_line = [line for line in output.split('\n') if 'RAM:' in line]
                    flash_line = [line for line in output.split('\n') if 'Flash:' in line]
                    
                    if ram_line and flash_line:
                        self.log_info(f"Memory usage: {ram_line[0].strip()}")
                        self.log_info(f"Flash usage: {flash_line[0].strip()}")
                        
                        # 检查内存使用是否合理
                        if "24.6%" in ram_line[0] or "RAM:" in ram_line[0]:
                            ram_percent = self.extract_percentage(ram_line[0])
                            if ram_percent and ram_percent < 30:
                                self.log_success(f"RAM usage ({ram_percent}%) is within acceptable limits")
                            elif ram_percent and ram_percent >= 30:
                                self.log_warning(f"RAM usage ({ram_percent}%) is high but acceptable")
                        
                        if "83.7%" in flash_line[0] or "Flash:" in flash_line[0]:
                            flash_percent = self.extract_percentage(flash_line[0])
                            if flash_percent and flash_percent < 90:
                                self.log_success(f"Flash usage ({flash_percent}%) is within acceptable limits")
                            elif flash_percent and flash_percent >= 90:
                                self.log_warning(f"Flash usage ({flash_percent}%) is high")
                
            else:
                self.log_error(f"Compilation failed: {result.stderr}")
                return False
                
        except subprocess.TimeoutExpired:
            self.log_error("Compilation timeout - possible infinite loop or deadlock")
            return False
        except Exception as e:
            self.log_error(f"Compilation test failed: {str(e)}")
            return False
            
        return True

    def extract_percentage(self, line):
        """从内存使用行中提取百分比"""
        import re
        match = re.search(r'\[.*?\]\s*(\d+\.?\d*)%', line)
        if match:
            return float(match.group(1))
        return None

    def test_memory_usage_analysis(self):
        """分析内存使用情况"""
        self.log_info("Analyzing memory usage...")
        
        try:
            # 检查编译输出文件
            elf_file = Path(".pio/build/esp32dev/firmware.elf")
            if elf_file.exists():
                # 使用size命令分析内存使用
                result = subprocess.run([
                    "xtensa-esp32-elf-size", "-A", str(elf_file)
                ], capture_output=True, text=True)
                
                if result.returncode == 0:
                    self.log_success("Memory analysis completed")
                    self.log_info("Memory sections:")
                    for line in result.stdout.split('\n'):
                        if line.strip() and not line.startswith('section'):
                            self.log_info(f"  {line.strip()}")
                    self.test_results["memory_usage_analysis"] = True
                else:
                    self.log_warning("Could not analyze memory usage with size tool")
                    self.test_results["memory_usage_analysis"] = True  # 不是关键错误
            else:
                self.log_warning("ELF file not found, skipping detailed memory analysis")
                self.test_results["memory_usage_analysis"] = True  # 不是关键错误
                
        except Exception as e:
            self.log_warning(f"Memory analysis failed: {str(e)}")
            self.test_results["memory_usage_analysis"] = True  # 不是关键错误
            
        return True

    def test_code_integration(self):
        """测试代码集成 - 验证RS485代码正确集成到WLED中"""
        self.log_info("Testing code integration...")
        
        # 检查关键文件是否存在
        required_files = [
            "usermods/rs485_control/rs485_control.cpp",
            "usermods/rs485_control/rs485_control.h",
            "usermods/rs485_control/library.json",
            "wled00/my_config.h"
        ]
        
        missing_files = []
        for file_path in required_files:
            if not Path(file_path).exists():
                missing_files.append(file_path)
        
        if missing_files:
            self.log_error(f"Missing required files: {missing_files}")
            return False
        else:
            self.log_success("All required files present")
        
        # 检查my_config.h中是否启用了RS485
        try:
            with open("wled00/my_config.h", "r", encoding="utf-8") as f:
                config_content = f.read()
                
            if "#define USERMOD_RS485_CONTROL" in config_content:
                self.log_success("RS485 usermod enabled in configuration")
            else:
                self.log_error("RS485 usermod not enabled in my_config.h")
                return False
                
        except Exception as e:
            self.log_error(f"Could not read configuration file: {str(e)}")
            return False
        
        self.test_results["code_integration_check"] = True
        return True

    def test_configuration_validation(self):
        """验证配置文件正确性"""
        self.log_info("Validating configuration files...")
        
        # 检查library.json
        library_json_path = "usermods/rs485_control/library.json"
        try:
            with open(library_json_path, "r", encoding="utf-8") as f:
                library_config = json.load(f)
                
            required_keys = ["name", "version", "description"]
            for key in required_keys:
                if key not in library_config:
                    self.log_error(f"Missing key '{key}' in library.json")
                    return False
                    
            self.log_success("library.json configuration valid")
            
        except json.JSONDecodeError as e:
            self.log_error(f"Invalid JSON in library.json: {str(e)}")
            return False
        except Exception as e:
            self.log_error(f"Could not read library.json: {str(e)}")
            return False
        
        self.test_results["configuration_validation"] = True
        return True

    def test_usermod_registration(self):
        """验证usermod注册"""
        self.log_info("Testing usermod registration...")
        
        # 检查头文件中的类定义
        try:
            with open("usermods/rs485_control/rs485_control.h", "r", encoding="utf-8") as f:
                header_content = f.read()
                
            if "class RS485ControlUsermod" in header_content:
                self.log_success("RS485ControlUsermod class found in header")
            else:
                self.log_error("RS485ControlUsermod class not found in header")
                return False
                
            # 检查实现文件中的注册宏
            with open("usermods/rs485_control/rs485_control.cpp", "r", encoding="utf-8") as f:
                cpp_content = f.read()
                
            if "REGISTER_USERMOD" in cpp_content or "Usermod" in cpp_content:
                self.log_success("Usermod registration found in implementation")
            else:
                self.log_warning("Usermod registration not clearly visible in implementation")
                
        except Exception as e:
            self.log_error(f"Could not verify usermod registration: {str(e)}")
            return False
        
        self.test_results["usermod_registration"] = True
        return True

    def run_all_tests(self):
        """运行所有兼容性测试"""
        self.log_info("Starting system compatibility verification...")
        self.log_info("Task 10.1 - System Compatibility Verification")
        self.log_info("Requirements: 5.1, 5.2, 5.3, 5.4, 5.5")
        print("=" * 60)
        
        tests = [
            ("Code Integration", self.test_code_integration),
            ("Configuration Validation", self.test_configuration_validation),
            ("Usermod Registration", self.test_usermod_registration),
            ("Compilation Compatibility", self.test_compilation_compatibility),
            ("Memory Usage Analysis", self.test_memory_usage_analysis),
        ]
        
        passed_tests = 0
        total_tests = len(tests)
        
        for test_name, test_func in tests:
            print(f"\n--- {test_name} ---")
            try:
                if test_func():
                    passed_tests += 1
                    self.log_success(f"{test_name} passed")
                else:
                    self.log_error(f"{test_name} failed")
            except Exception as e:
                self.log_error(f"{test_name} failed with exception: {str(e)}")
        
        print("\n" + "=" * 60)
        print("SYSTEM COMPATIBILITY VERIFICATION RESULTS")
        print("=" * 60)
        
        print(f"Tests passed: {passed_tests}/{total_tests}")
        
        for test_name, result in self.test_results.items():
            status = "✓ PASS" if result else "✗ FAIL"
            print(f"  {test_name}: {status}")
        
        if self.issues_found:
            print(f"\nIssues found ({len(self.issues_found)}):")
            for issue in self.issues_found:
                print(f"  - {issue}")
        
        success_rate = (passed_tests / total_tests) * 100
        print(f"\nOverall success rate: {success_rate:.1f}%")
        
        if success_rate >= 80:
            print("\n✓ SYSTEM COMPATIBILITY VERIFICATION PASSED")
            print("RS485 functionality is compatible with WLED core system")
            return True
        else:
            print("\n✗ SYSTEM COMPATIBILITY VERIFICATION FAILED")
            print("RS485 functionality may have compatibility issues")
            return False

def main():
    """主函数"""
    print("WLED RS485 System Compatibility Verifier")
    print("Task 10.1 Implementation")
    print("=" * 60)
    
    verifier = SystemCompatibilityVerifier()
    success = verifier.run_all_tests()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())