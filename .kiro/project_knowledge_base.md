# WLED RS485控制模块 - 项目知识库

## 🆕 最新更新 (2025-01-24)

### RS485死循环紧急修复 🚨
**问题**: 发送RS485命令后设备出现死循环回复错误消息，导致系统不可用。

**根本原因**: 
- RS485半双工特性：设备会接收到自己发送的消息
- 缺少回环检测：设备处理自己的回复消息，形成无限循环
- 典型场景：`@12 echo test` → 错误回复 → 处理自己的回复 → 再次错误 → 死循环

**解决方案**:
1. 添加回环检测机制，识别自己发送的消息
2. 检查接收消息是否以自己的地址前缀开头
3. 如果是自发消息，直接忽略，避免重复处理

**技术实现**:
```cpp
// 生成自己的地址前缀 (@012 格式)
String ownAddressPrefix = "@" + formatAddress(config.deviceAddress) + " ";

// 回环检测：忽略自己发送的消息
if (message.startsWith(ownAddressPrefix)) {
    return; // 直接忽略，避免死循环
}
```

**修复文件**: `usermods/rs485_control/rs485_control_simple.cpp` - `loop()`函数

### 自定义序列效果状态重置修复
**问题**: 自定义序列效果"Custom Seq 1"在每次调用预设时不会从头开始执行，而是继续之前的状态。

**根本原因**: 
- `SEGENV.call == 0`检查后没有立即返回，导致状态初始化后立即被后续逻辑覆盖
- 状态变量在效果重新激活时没有正确重置到初始值

**解决方案**:
1. 在`SEGENV.call == 0`分支中添加立即返回语句
2. 确保状态变量正确重置：
   - `SEGENV.step = strip.now` (记录开始时间)
   - `SEGENV.aux0 = 0` (重置序列步骤为0)  
   - `SEGENV.aux1 = 0` (重置循环计数器为0)
3. 立即显示序列中的第一个LED，确保视觉反馈

**修复文件**: `wled00/FX.cpp` - `mode_custom_sequence_1()`函数

### RS485地址范围扩展 (1-99 → 1-255)
**改进**: 将RS485设备地址范围从1-99扩展到1-255，支持更多设备。

**开销分析**:
- **内存开销**: 无增加（`uint8_t`已支持0-255）
- **处理开销**: 无增加（简单整数比较，O(1)复杂度）
- **通信开销**: 微乎其微（字符串长度略增）

**修改内容**:
1. 更新配置说明：`Device address (1-255, unique for each device)`
2. 改进地址格式化逻辑，支持3位数地址：
   - 1-9: `@001`, `@002`, ...
   - 10-99: `@010`, `@099`, ...
   - 100-255: `@100`, `@255`, ...

**修复文件**: `usermods/rs485_control/rs485_control_simple.cpp`

**测试验证**:
- 通过网页界面切换预设：序列正确从LED 1开始
- 通过RS485命令`@001 PRESET 3`：每次调用都从头开始
- 地址范围测试：支持`@001`到`@255`的完整范围

---

## 📋 项目概述

本项目为WLED LED控制系统开发了RS485串口控制模块，允许通过RS485通信协议远程控制WLED的预设、状态查询和系统管理。

### 项目基本信息
- **项目名称**：WLED RS485控制模块
- **开发平台**：ESP32
- **框架**：Arduino + PlatformIO
- **WLED版本**：0.16.0-alpha
- **开发语言**：C++
- **通信协议**：RS485串口通信

## 🏗️ 项目架构

### 核心组件
```
RS485ControlUsermod (主控制类)
├── RS485Config (配置管理)
├── RS485Stats (统计信息)
├── 串口通信处理
├── 命令解析器
└── WLED预设控制
```

### 文件结构
```
usermods/rs485_control/
├── rs485_control_simple.cpp    # 简化版实现（当前使用）
├── rs485_control.cpp          # 完整版实现（复杂依赖）
├── rs485_control.h            # 头文件定义
├── library.json               # PlatformIO库配置
└── readme.md                  # 模块说明文档
```

## 🔧 技术实现

### 主要类定义

#### RS485ControlUsermod类
```cpp
class RS485ControlUsermod : public Usermod {
private:
    RS485Config config;          // 配置参数
    RS485Stats stats;           // 运行统计
    HardwareSerial* serial;     // 串口对象
    int currentPreset;          // 当前预设
    
public:
    // WLED Usermod接口
    void setup() override;
    void loop() override;
    void connected() override;
    
    // 配置管理
    void addToConfig(JsonObject& root) override;
    bool readFromConfig(JsonObject& root) override;
    void appendConfigData() override;
    
    // 状态和信息
    void addToJsonInfo(JsonObject& root) override;
    void addToJsonState(JsonObject& root) override;
    void readFromJsonState(JsonObject& root) override;
    
    // 事件处理
    void onStateChange(uint8_t mode) override;
    uint16_t getId() override;
};
```

#### 配置结构
```cpp
struct RS485Config {
    bool enabled = true;         // 启用状态
    uint32_t baudRate = 9600;    // 波特率
    int rxPin = 26;              // 接收引脚
    int txPin = 27;              // 发送引脚
    size_t bufferSize = 512;     // 缓冲区大小
    uint32_t timeout = 1000;     // 超时时间
    bool echoEnabled = true;     // 回声服务
    bool debugMode = false;      // 调试模式
};
```

### 关键技术点

#### 1. Usermod注册机制
```cpp
// 必须使用static声明和REGISTER_USERMOD宏
static RS485ControlUsermod rs485_control_usermod;
REGISTER_USERMOD(rs485_control_usermod);
```

#### 2. 链接器Section机制
- WLED使用特殊的链接器section `.dtors.tbl.usermods.1` 自动发现usermod
- `REGISTER_USERMOD`宏将usermod指针放入此section
- UsermodManager在运行时扫描此section来注册所有usermod

#### 3. 内存优化
```cpp
// 使用PROGMEM存储字符串常量
const char RS485ControlUsermod::_name[] PROGMEM = "RS485Control";

// 使用FPSTR宏访问PROGMEM字符串
JsonObject top = root.createNestedObject(FPSTR(_name));
```

## 📡 功能特性

### 支持的命令
1. **ECHO <text>** - 回声测试（需启用echoEnabled）
2. **PRESET <1-250>** - 激活指定预设
3. **PRESET?** - 查询当前预设
4. **STATUS** - 查询系统状态
5. **HELP** - 显示帮助信息

### 配置选项
- **Enabled**: 启用/禁用RS485控制
- **Baud Rate**: 串口波特率（9600-115200）
- **RX Pin**: 接收数据引脚（GPIO编号）
- **TX Pin**: 发送数据引脚（GPIO编号）
- **Buffer Size**: 缓冲区大小（256-2048字节）
- **Timeout**: 超时时间（500-5000毫秒）
- **Echo Enabled**: 启用回声服务进行通信测试
- **Debug Mode**: 启用调试输出到串口监视器

### 硬件连接
```
ESP32          RS485模块
GPIO26    →    RO (接收输出)
GPIO27    →    DI (数据输入)
3.3V      →    VCC
GND       →    GND
```

## 🛠️ 开发过程记录

### 主要开发阶段

#### 1. 需求分析和设计
- 分析WLED usermod系统架构
- 设计RS485通信协议
- 定义配置参数和命令格式

#### 2. 初始实现
- 创建复杂版本（rs485_control.cpp）
- 实现完整的类层次结构
- 遇到编译依赖问题

#### 3. 简化重构
- 创建简化版本（rs485_control_simple.cpp）
- 移除复杂的类依赖关系
- 直接在主类中实现所有功能

#### 4. 注册机制修复
- 发现usermod未在Config页面显示的问题
- 研究WLED的usermod注册机制
- 正确使用REGISTER_USERMOD宏和static声明

#### 5. 功能完善
- 扩展预设支持范围（1-16 → 1-250）
- 改进配置说明和错误处理
- 添加详细的调试和监控功能

### 关键问题和解决方案

#### 问题1: 编译错误
**原因**: 复杂的类依赖关系导致编译失败
**解决**: 创建简化版本，将所有功能集成到单一类中

#### 问题2: Config页面不显示
**原因**: 缺少正确的usermod注册
**解决**: 使用`static`声明和`REGISTER_USERMOD`宏

#### 问题3: 预设范围限制
**原因**: 硬编码的1-16限制过于严格
**解决**: 扩展到1-250以匹配WLED标准

## 📁 重要文件清单

### 核心代码文件
- `usermods/rs485_control/rs485_control_simple.cpp` - 主实现文件
- `usermods/rs485_control/rs485_control.h` - 头文件定义
- `usermods/rs485_control/library.json` - PlatformIO库配置

### 配置文件
- `platformio.ini` - 编译配置（包含build_src_filter）
- `wled00/my_config.h` - WLED自定义配置

### 文档文件
- `WLED二次开发完整指南.md` - 完整开发指南
- `RS485控制模块详细说明.md` - 功能使用说明
- `WLED_RS485_最终修复说明.md` - 问题修复记录
- `WLED_RS485_V4_网页烧录指南.md` - 用户使用指南

### 测试和验证文件
- `verify_rs485_compilation.py` - 编译验证脚本
- `check_usermod_registration.py` - 注册检查脚本
- `final_rs485_test.py` - 最终测试脚本

### 生成的固件
- `build_output/release/WLED_0.16.0-alpha_ESP32.bin` - 最终固件文件

## 🔍 调试和故障排除

### 常见问题

#### 1. Config页面不显示RS485Control
**检查项目**:
- 确认使用了`REGISTER_USERMOD`宏
- 确认usermod实例声明为`static`
- 检查编译输出中的usermod数量

#### 2. RS485通信无响应
**检查项目**:
- 硬件连接是否正确
- 波特率设置是否匹配
- 启用调试模式查看通信日志

#### 3. 预设激活失败
**检查项目**:
- 预设ID是否在1-250范围内
- 预设是否在WLED中已创建
- 检查WLED系统状态

### 调试方法

#### 启用调试输出
1. 在RS485Control配置中启用Debug Mode
2. 连接串口监视器（波特率115200）
3. 观察RS485通信日志

#### 验证编译结果
```bash
# 检查usermod注册
python check_usermod_registration.py

# 验证固件内容
python verify_rs485_compilation.py

# 运行完整测试
python final_rs485_test.py
```

## 🚀 部署和使用

### 编译命令
```bash
pio run -e esp32dev
```

### 固件烧录
1. 通过WLED网页界面上传固件文件
2. 等待设备重启完成
3. 进入Config → Usermods配置RS485参数

### 基本使用
1. 配置硬件连接（RX: GPIO26, TX: GPIO27）
2. 设置波特率（默认9600）
3. 启用Echo服务进行连接测试
4. 发送命令控制WLED

## 📚 扩展开发指南

### 添加新命令
1. 在`processCommand()`函数中添加命令解析
2. 实现对应的处理逻辑
3. 更新HELP命令的响应信息

### 修改配置参数
1. 在`RS485Config`结构中添加新参数
2. 在`addToConfig()`和`readFromConfig()`中处理
3. 在`appendConfigData()`中添加界面说明

### 集成其他通信协议
1. 参考RS485实现创建新的通信类
2. 实现对应的协议解析器
3. 在主usermod类中集成新功能

## 🔄 版本历史

### v1.0 (当前版本)
- 基本RS485通信功能
- 预设控制（1-250）
- 回声测试服务
- Web界面集成
- 调试和监控功能

### 未来计划
- 支持更多WLED功能控制
- 添加批量命令处理
- 实现命令队列和优先级
- 支持自定义命令扩展

---

**注意**: 此知识库记录了完整的开发过程和技术细节，为后续的WLED二次开发提供参考。建议在开始新的开发项目时，首先阅读本文档以了解WLED的架构和开发方法。