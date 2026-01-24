#!/usr/bin/env python3
"""
性能影响分析脚本 - 任务10.1补充
分析RS485功能对WLED系统性能的影响
需求: 5.4, 5.5
"""

import subprocess
import sys
import re
import json
from pathlib import Path

class PerformanceAnalyzer:
    def __init__(self):
        self.baseline_metrics = {}
        self.rs485_metrics = {}
        self.analysis_results = {}
        
    def log_info(self, message):
        print(f"[INFO] {message}")
        
    def log_success(self, message):
        print(f"[SUCCESS] ✓ {message}")
        
    def log_warning(self, message):
        print(f"[WARNING] ⚠ {message}")
        
    def log_error(self, message):
        print(f"[ERROR] ✗ {message}")

    def extract_build_metrics(self, build_output):
        """从编译输出中提取性能指标"""
        metrics = {
            "ram_used": 0,
            "ram_total": 0,
            "ram_percent": 0.0,
            "flash_used": 0,
            "flash_total": 0,
            "flash_percent": 0.0,
            "build_time": 0.0
        }
        
        # 提取RAM使用信息
        ram_match = re.search(r'RAM:\s*\[.*?\]\s*(\d+\.?\d*)%\s*\(used (\d+) bytes from (\d+) bytes\)', build_output)
        if ram_match:
            metrics["ram_percent"] = float(ram_match.group(1))
            metrics["ram_used"] = int(ram_match.group(2))
            metrics["ram_total"] = int(ram_match.group(3))
        
        # 提取Flash使用信息
        flash_match = re.search(r'Flash:\s*\[.*?\]\s*(\d+\.?\d*)%\s*\(used (\d+) bytes from (\d+) bytes\)', build_output)
        if flash_match:
            metrics["flash_percent"] = float(flash_match.group(1))
            metrics["flash_used"] = int(flash_match.group(2))
            metrics["flash_total"] = int(flash_match.group(3))
        
        # 提取编译时间
        time_match = re.search(r'Took (\d+\.?\d*) seconds', build_output)
        if time_match:
            metrics["build_time"] = float(time_match.group(1))
        
        return metrics

    def build_without_rs485(self):
        """编译不包含RS485功能的版本作为基准"""
        self.log_info("Building baseline version without RS485...")
        
        # 临时禁用RS485功能
        config_path = "wled00/my_config.h"
        try:
            with open(config_path, "r", encoding="utf-8") as f:
                original_config = f.read()
            
            # 注释掉RS485定义
            modified_config = original_config.replace(
                "#define USERMOD_RS485_CONTROL",
                "// #define USERMOD_RS485_CONTROL"
            )
            
            with open(config_path, "w", encoding="utf-8") as f:
                f.write(modified_config)
            
            # 清理并重新编译
            subprocess.run(["pio", "run", "-e", "esp32dev", "-t", "clean"], 
                         capture_output=True, text=True)
            
            result = subprocess.run(["pio", "run", "-e", "esp32dev"], 
                                  capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0:
                self.baseline_metrics = self.extract_build_metrics(result.stdout)
                self.log_success("Baseline build completed")
            else:
                self.log_error(f"Baseline build failed: {result.stderr}")
                return False
            
            # 恢复原始配置
            with open(config_path, "w", encoding="utf-8") as f:
                f.write(original_config)
                
        except Exception as e:
            self.log_error(f"Failed to build baseline: {str(e)}")
            return False
            
        return True

    def build_with_rs485(self):
        """编译包含RS485功能的版本"""
        self.log_info("Building version with RS485...")
        
        try:
            # 清理并重新编译
            subprocess.run(["pio", "run", "-e", "esp32dev", "-t", "clean"], 
                         capture_output=True, text=True)
            
            result = subprocess.run(["pio", "run", "-e", "esp32dev"], 
                                  capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0:
                self.rs485_metrics = self.extract_build_metrics(result.stdout)
                self.log_success("RS485 build completed")
            else:
                self.log_error(f"RS485 build failed: {result.stderr}")
                return False
                
        except Exception as e:
            self.log_error(f"Failed to build with RS485: {str(e)}")
            return False
            
        return True

    def analyze_performance_impact(self):
        """分析性能影响"""
        self.log_info("Analyzing performance impact...")
        
        if not self.baseline_metrics or not self.rs485_metrics:
            self.log_error("Missing metrics for comparison")
            return False
        
        # 计算差异
        ram_diff = self.rs485_metrics["ram_used"] - self.baseline_metrics["ram_used"]
        flash_diff = self.rs485_metrics["flash_used"] - self.baseline_metrics["flash_used"]
        build_time_diff = self.rs485_metrics["build_time"] - self.baseline_metrics["build_time"]
        
        ram_percent_diff = self.rs485_metrics["ram_percent"] - self.baseline_metrics["ram_percent"]
        flash_percent_diff = self.rs485_metrics["flash_percent"] - self.baseline_metrics["flash_percent"]
        
        self.analysis_results = {
            "ram_impact": {
                "absolute": ram_diff,
                "percentage": ram_percent_diff,
                "acceptable": ram_diff < 10240  # 小于10KB
            },
            "flash_impact": {
                "absolute": flash_diff,
                "percentage": flash_percent_diff,
                "acceptable": flash_diff < 51200  # 小于50KB
            },
            "build_time_impact": {
                "absolute": build_time_diff,
                "acceptable": build_time_diff < 5.0  # 小于5秒
            }
        }
        
        return True

    def generate_report(self):
        """生成性能影响报告"""
        print("\n" + "=" * 60)
        print("PERFORMANCE IMPACT ANALYSIS REPORT")
        print("=" * 60)
        
        print("\nBASELINE METRICS (without RS485):")
        print(f"  RAM:   {self.baseline_metrics['ram_used']:,} bytes ({self.baseline_metrics['ram_percent']:.1f}%)")
        print(f"  Flash: {self.baseline_metrics['flash_used']:,} bytes ({self.baseline_metrics['flash_percent']:.1f}%)")
        print(f"  Build time: {self.baseline_metrics['build_time']:.1f} seconds")
        
        print("\nRS485 METRICS (with RS485):")
        print(f"  RAM:   {self.rs485_metrics['ram_used']:,} bytes ({self.rs485_metrics['ram_percent']:.1f}%)")
        print(f"  Flash: {self.rs485_metrics['flash_used']:,} bytes ({self.rs485_metrics['flash_percent']:.1f}%)")
        print(f"  Build time: {self.rs485_metrics['build_time']:.1f} seconds")
        
        print("\nIMPACT ANALYSIS:")
        
        # RAM影响
        ram_impact = self.analysis_results["ram_impact"]
        ram_status = "✓ ACCEPTABLE" if ram_impact["acceptable"] else "✗ HIGH"
        print(f"  RAM Impact: +{ram_impact['absolute']:,} bytes (+{ram_impact['percentage']:.1f}%) - {ram_status}")
        
        # Flash影响
        flash_impact = self.analysis_results["flash_impact"]
        flash_status = "✓ ACCEPTABLE" if flash_impact["acceptable"] else "✗ HIGH"
        print(f"  Flash Impact: +{flash_impact['absolute']:,} bytes (+{flash_impact['percentage']:.1f}%) - {flash_status}")
        
        # 编译时间影响
        build_impact = self.analysis_results["build_time_impact"]
        build_status = "✓ ACCEPTABLE" if build_impact["acceptable"] else "✗ HIGH"
        print(f"  Build Time Impact: +{build_impact['absolute']:.1f} seconds - {build_status}")
        
        # 总体评估
        all_acceptable = (ram_impact["acceptable"] and 
                         flash_impact["acceptable"] and 
                         build_impact["acceptable"])
        
        print(f"\nOVERALL ASSESSMENT:")
        if all_acceptable:
            print("✓ RS485 functionality has ACCEPTABLE performance impact")
            print("  The feature can be safely integrated without significant system degradation")
        else:
            print("⚠ RS485 functionality has SIGNIFICANT performance impact")
            print("  Consider optimization or review resource usage")
        
        # 建议
        print(f"\nRECOMMENDATIONS:")
        if ram_impact["absolute"] > 5120:  # 5KB
            print("  - Consider optimizing RAM usage in RS485 implementation")
        if flash_impact["absolute"] > 25600:  # 25KB
            print("  - Consider code size optimization for RS485 functionality")
        if build_impact["absolute"] > 2.0:
            print("  - Build time increase is within normal range for added functionality")
        
        if all_acceptable:
            print("  - No immediate optimizations required")
            print("  - RS485 functionality ready for production use")
        
        return all_acceptable

    def run_analysis(self):
        """运行完整的性能分析"""
        self.log_info("Starting performance impact analysis...")
        
        # 构建基准版本
        if not self.build_without_rs485():
            return False
        
        # 构建RS485版本
        if not self.build_with_rs485():
            return False
        
        # 分析影响
        if not self.analyze_performance_impact():
            return False
        
        # 生成报告
        return self.generate_report()

def main():
    """主函数"""
    print("WLED RS485 Performance Impact Analyzer")
    print("Task 10.1 - Performance Analysis Component")
    print("=" * 60)
    
    analyzer = PerformanceAnalyzer()
    success = analyzer.run_analysis()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())