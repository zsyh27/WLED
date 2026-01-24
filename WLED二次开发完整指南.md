# WLED二次开发完整指南

## 📚 项目概述

本文档记录了WLED RS485控制模块的完整开发过程，包括项目结构、类设计、开发方法和最佳实践，为后续的WLED二次开发提供参考。

## 🏗️ WLED项目结构

### 核心目录结构
```
WLED/
├── wled00/                    # WLED核心代码
│   ├── wled.h                # 主头文件，包含全局变量和函数声明
│   ├── wled.cpp              # 主程序文件
│   ├── um_manager.cpp        # Usermod管理器
│   ├── fcn_declare.h         # 函数声明和宏定义
│   ├── usermod.cpp           # V1 usermod接口（已废弃）
│   └── data/                 # Web界面文件
├── usermods/                 # 用户模块目录
│   ├── EXAMPLE/              # 示例usermod
│   ├── rs485_control/        # 我们开发的RS485控制模块
│   └── [其他usermod]/
├── platformio.ini            # PlatformIO配置文件
├── pio-scripts/              # 编译脚本
└── tools/                    # 工具和分区表
```

### 关键文件说明

#### 1. wled00/wled.h
- **作用**：WLED的主头文件
- **包含内容**：
  - 全局变量声明（如`strip`、`bri`、`col[]`等）
  - 核心函数声明
  - 预设和状态管理函数
- **重要性**：所有usermod都必须包含此文件

#### 2. wled00/um_manager.cpp
- **作用**：Usermod管理器，负责usermod的生命周期管理
- **核心功能**：
  - 自动发现和注册usermod
  - 调用usermod的各种回调函数
  - 管理usermod的配置和状态

#### 3. wled00/fcn_declare.h
- **作用**：函数声明和重要宏定义
- **关键宏**：
  - `REGISTER_USERMOD(x)`：usermod注册宏
  - `DEBUG_PRINTLN`：调试输出宏
  - `USERMOD_ID_*`：usermod ID定义

## 🧩 WLED Usermod系统架构

### Usermod生命周期
```
1. 编译时注册 → 2. 系统启动 → 3. setup() → 4. loop() → 5. 配置管理 → 6. 状态同步
```

### 核心类和接口

#### 1. Usermod基类
```cpp
class Usermod {
public:
    // 生命周期方法
    virtual void setup() {}                    // 初始化
    virtual void loop() {}                     // 主循环
    virtual void connected() {}                // WiFi连接后

    // 配置管理
    virtual void addToConfig(JsonObject& root) {}      // 添加配置到JSON
    virtual bool readFromConfig(JsonObject& root) {}   // 从JSON读取配置
    virtual void appendConfigData() {}                 // 添加配置界面元素

    // 状态管理
    virtual void addToJsonInfo(JsonObject& root) {}    // 添加信息到Info页面
    virtual void addToJsonState(JsonObject& root) {}   // 添加状态到JSON API
    virtual void readFromJsonState(JsonObject& root) {} // 从JSON API读取状态

    // 事件处理
    virtual void onStateChange(uint8_t mode) {}        // WLED状态变化
    virtual bool handleButton(uint8_t b) { return false; } // 按钮处理
    virtual void handleOverlayDraw() {}                // 覆盖绘制

    // 通信接口
    virtual bool onMqttMessage(char* topic, char* payload) { return false; }
    virtual bool onUdpPacket(uint8_t* payload, size_t len) { return false; }
    
    // 标识
    virtual uint16_t getId() { return USERMOD_ID_UNSPECIFIED; }
};
```

#### 2. UsermodManager类
```cpp
namespace UsermodManager {
    void setup();                              // 初始化所有usermod
    void loop();                               // 调用所有usermod的loop()
    void connected();                          // 通知WiFi连接
    void addToConfig(JsonObject& obj);         // 收集所有usermod配置
    bool readFromConfig(JsonObject& obj);      // 分发配置到各usermod
    void addToJsonInfo(JsonObject& obj);       // 收集Info信息
    void addToJsonState(JsonObject& obj);      // 收集状态信息
    void readFromJsonState(JsonObject& obj);   // 分发状态更新
    void onStateChange(uint8_t mode);          // 通知状态变化
    Usermod* lookup(uint16_t mod_id);          // 查找特定usermod
    size_t getModCount();                      // 获取usermod数量
}
```

## 🔧 RS485控制模块类设计

### 主要类结构

#### 1. RS485ControlUsermod（主控制类）
```cpp
class RS485ControlUsermod : public Usermod {
private:
    // 配置和状态
    RS485Config config;           // 配置参数
    RS485Stats stats;            // 统计信息
    bool initDone;               // 初始化标志
    HardwareSerial* serial;      // 串口对象
    int currentPreset;           // 当前预设
    
    // 配置常量（存储在PROGMEM中节省RAM）
    static const char _name[] PROGMEM;
    static const char _enabled[] PROGMEM;
    // ... 其他配置键名

public:
    // Usermod接口实现
    void setup() override;                     // 初始化串口和配置
    void loop() override;                      // 处理RS485通信
    void connected() override;                 // WiFi连接处理
    
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
    uint16_t getId() override { return USERMOD_ID_RS485_CONTROL; }

private:
    // 内部方法
    void applyWLEDPreset(int presetId);       // 应用WLED预设
    String processCommand(const String& input); // 处理RS485命令
};
```

#### 2. 配置和状态结构体
```cpp
// 配置结构
struct RS485Config {
    bool enabled = true;          // 启用状态
    uint32_t baudRate = 9600;     // 波特率
    int rxPin = 26;               // 接收引脚
    int txPin = 27;               // 发送引脚
    size_t bufferSize = 512;      // 缓冲区大小
    uint32_t timeout = 1000;      // 超时时间
    bool echoEnabled = true;      // 回声服务
    bool debugMode = false;       // 调试模式
};

// 统计结构
struct RS485Stats {
    uint32_t messagesReceived = 0;  // 接收消息数
    uint32_t messagesSent = 0;      // 发送消息数
    uint32_t parseErrors = 0;       // 解析错误数
    uint32_t bufferOverflows = 0;   // 缓冲区溢出数
    uint32_t timeouts = 0;          // 超时次数
    uint32_t lastActivity = 0;      // 最后活动时间
    bool connected = false;         // 连接状态
};
```

### 类的职责分工

#### RS485ControlUsermod
- **主要职责**：作为usermod的入口点，实现WLED usermod接口
- **核心功能**：
  - 管理RS485串口通信
  - 处理配置的读取和保存
  - 提供Web界面集成
  - 处理命令解析和执行

#### 配置管理
- **RS485Config**：存储所有可配置参数
- **配置持久化**：通过WLED的JSON配置系统自动保存
- **配置验证**：确保参数在有效范围内

#### 统计和监控
- **RS485Stats**：记录运行时统计信息
- **性能监控**：跟踪消息处理性能
- **错误统计**：帮助诊断通信问题

## 🛠️ WLED二次开发方法

### 1. 开发环境搭建

#### 必需工具
```bash
# 安装PlatformIO
pip install platformio

# 克隆WLED源码
git clone https://github.com/Aircoookie/WLED.git
cd WLED

# 安装依赖
pio pkg install
```

#### 开发工具推荐
- **IDE**：Visual Studio Code + PlatformIO插件
- **调试**：串口监视器（115200波特率）
- **测试**：WLED Web界面 + 串口工具

### 2. Usermod开发流程

#### 步骤1：创建usermod目录
```bash
mkdir usermods/my_usermod
cd usermods/my_usermod
```

#### 步骤2：创建基本文件
```cpp
// my_usermod.h
#pragma once
#include "wled.h"

#define USERMOD_ID_MY_USERMOD 60  // 选择唯一ID

class MyUsermod : public Usermod {
private:
    bool enabled = true;
    static const char _name[] PROGMEM;
    
public:
    void setup() override;
    void loop() override;
    void addToConfig(JsonObject& root) override;
    bool readFromConfig(JsonObject& root) override;
    uint16_t getId() override { return USERMOD_ID_MY_USERMOD; }
};
```

```cpp
// my_usermod.cpp
#include "my_usermod.h"

const char MyUsermod::_name[] PROGMEM = "MyUsermod";

void MyUsermod::setup() {
    // 初始化代码
}

void MyUsermod::loop() {
    // 主循环代码
}

void MyUsermod::addToConfig(JsonObject& root) {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top["enabled"] = enabled;
}

bool MyUsermod::readFromConfig(JsonObject& root) {
    JsonObject top = root[FPSTR(_name)];
    if (top.isNull()) return false;
    
    enabled = top["enabled"] | enabled;
    return true;
}

// 注册usermod
static MyUsermod my_usermod;
REGISTER_USERMOD(my_usermod);
```

#### 步骤3：配置编译
```ini
# platformio.ini 中添加
[env:esp32dev_my_usermod]
extends = env:esp32dev
build_src_filter = +<*> +<../usermods/my_usermod/my_usermod.cpp>
```

#### 步骤4：编译和测试
```bash
pio run -e esp32dev_my_usermod
```

### 3. 关键开发技巧

#### 内存管理
```cpp
// 使用PROGMEM存储字符串常量
const char MyUsermod::_name[] PROGMEM = "MyUsermod";

// 使用FPSTR宏访问PROGMEM字符串
JsonObject top = root.createNestedObject(FPSTR(_name));

// 避免大量动态内存分配
static char buffer[256];  // 使用静态缓冲区
```

#### 配置管理最佳实践
```cpp
void MyUsermod::addToConfig(JsonObject& root) {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top[FPSTR(_enabled)] = enabled;
    top[FPSTR(_pin)] = pin;
    top[FPSTR(_interval)] = interval;
}

bool MyUsermod::readFromConfig(JsonObject& root) {
    JsonObject top = root[FPSTR(_name)];
    if (top.isNull()) return false;
    
    bool configChanged = false;
    
    if (top[FPSTR(_enabled)] != enabled) {
        enabled = top[FPSTR(_enabled)] | enabled;
        configChanged = true;
    }
    
    // 配置变化时重新初始化
    if (configChanged) {
        reinitialize();
    }
    
    return configChanged;
}
```

#### Web界面集成
```cpp
void MyUsermod::appendConfigData() {
    oappend(SET_F("addInfo('MyUsermod:enabled',1,'Enable my usermod');"));
    oappend(SET_F("addInfo('MyUsermod:pin',1,'GPIO pin number');"));
}

void MyUsermod::addToJsonInfo(JsonObject& root) {
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");
    
    JsonArray infoArr = user.createNestedArray(FPSTR(_name));
    infoArr.add(enabled ? F("Enabled") : F("Disabled"));
    infoArr.add(String(F("Pin: ")) + String(pin));
}
```

### 4. 调试和测试

#### 调试输出
```cpp
void MyUsermod::setup() {
    DEBUG_PRINTLN(F("MyUsermod: Starting setup..."));
    
    if (enabled) {
        DEBUG_PRINTF("MyUsermod: Initialized on pin %d\n", pin);
    }
    
    DEBUG_PRINTLN(F("MyUsermod: Setup completed"));
}
```

#### 性能监控
```cpp
void MyUsermod::loop() {
    static unsigned long lastRun = 0;
    unsigned long now = millis();
    
    if (now - lastRun < 100) return;  // 限制执行频率
    lastRun = now;
    
    // 主要逻辑
}
```

#### 错误处理
```cpp
bool MyUsermod::initialize() {
    if (!validateConfig()) {
        DEBUG_PRINTLN(F("MyUsermod: Invalid configuration"));
        return false;
    }
    
    if (!initializeHardware()) {
        DEBUG_PRINTLN(F("MyUsermod: Hardware initialization failed"));
        return false;
    }
    
    return true;
}
```

## 📋 开发检查清单

### 编码规范
- [ ] 使用PROGMEM存储字符串常量
- [ ] 实现所有必要的Usermod接口方法
- [ ] 添加适当的调试输出
- [ ] 处理配置变化和错误情况
- [ ] 限制loop()函数的执行频率

### 配置管理
- [ ] 实现addToConfig()和readFromConfig()
- [ ] 添加配置界面说明（appendConfigData）
- [ ] 验证配置参数的有效性
- [ ] 处理配置变化时的重新初始化

### Web界面集成
- [ ] 实现addToJsonInfo()显示状态信息
- [ ] 添加启用/禁用控制按钮
- [ ] 提供有用的统计信息
- [ ] 确保界面响应式设计

### 测试验证
- [ ] 编译无错误和警告
- [ ] 在Config→Usermods中正确显示
- [ ] 配置保存和加载正常
- [ ] 功能按预期工作
- [ ] 内存使用合理

### 文档和维护
- [ ] 编写README.md说明文档
- [ ] 记录配置参数和使用方法
- [ ] 提供示例和故障排除指南
- [ ] 标注版本和兼容性信息

## 🔄 版本管理和发布

### 版本控制
```cpp
#define USERMOD_VERSION "1.0.0"

void MyUsermod::addToJsonInfo(JsonObject& root) {
    // ... 其他信息
    infoArr.add(String(F("Version: ")) + String(USERMOD_VERSION));
}
```

### 兼容性标记
```cpp
// 在README.md中记录
## 兼容性
- WLED版本：0.16.0+
- ESP32：支持
- ESP8266：支持（需要足够内存）
- Arduino框架：2.0.9+
```

### 发布准备
1. **代码审查**：确保代码质量和安全性
2. **测试验证**：在不同硬件平台上测试
3. **文档完善**：更新使用说明和API文档
4. **示例提供**：提供完整的使用示例
5. **社区反馈**：收集用户反馈和改进建议

## 📚 参考资源

### 官方文档
- [WLED GitHub](https://github.com/Aircoookie/WLED)
- [WLED知识库](https://kno.wled.ge/)
- [Usermod开发指南](https://kno.wled.ge/advanced/custom-features/)

### 社区资源
- [WLED论坛](https://wled.discourse.group/)
- [示例Usermod](https://github.com/Aircoookie/WLED/tree/master/usermods)
- [MoonModules项目](https://mm.kno.wled.ge/)

### 开发工具
- [PlatformIO文档](https://docs.platformio.org/)
- [ESP32参考手册](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [ArduinoJSON库](https://arduinojson.org/)

这个完整的指南涵盖了WLED二次开发的所有关键方面，为后续开发提供了详细的参考和最佳实践。