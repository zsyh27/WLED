# AI助手项目上下文

## 🤖 给AI助手的项目说明

当AI助手需要协助WLED相关开发时，请参考以下信息快速理解项目结构和开发方法。

## 📋 项目快速概览

### 项目类型
- **主项目**: WLED LED控制系统的二次开发
- **开发内容**: RS485串口控制模块
- **技术栈**: C++, Arduino框架, PlatformIO, ESP32

### 核心文件位置
```
关键实现: usermods/rs485_control/rs485_control_simple.cpp
配置文件: platformio.ini (包含build_src_filter配置)
文档目录: 根目录下的*.md文件
知识库: .kiro/project_knowledge_base.md
```

## 🏗️ WLED架构要点

### Usermod系统
WLED使用Usermod系统进行功能扩展：
- **基类**: `Usermod` (定义在wled.h中)
- **注册机制**: `REGISTER_USERMOD(instance)` 宏
- **管理器**: `UsermodManager` 负责生命周期管理

### 关键接口方法
```cpp
class MyUsermod : public Usermod {
    void setup() override;                    // 初始化
    void loop() override;                     // 主循环
    void addToConfig(JsonObject&) override;   // 配置保存
    bool readFromConfig(JsonObject&) override; // 配置读取
    void addToJsonInfo(JsonObject&) override; // Info页面显示
    uint16_t getId() override;                // 唯一ID
};
```

### 注册要求
```cpp
// 必须使用static声明
static MyUsermod my_usermod;
// 必须使用注册宏
REGISTER_USERMOD(my_usermod);
```

## 🔧 当前项目实现

### 主要类: RS485ControlUsermod
- **文件**: `usermods/rs485_control/rs485_control_simple.cpp`
- **功能**: RS485串口通信控制WLED预设和状态
- **配置**: 波特率、引脚、缓冲区等参数
- **命令**: ECHO, PRESET, STATUS, HELP

### 配置结构
```cpp
struct RS485Config {
    bool enabled;           // 启用状态
    uint32_t baudRate;      // 波特率
    int rxPin, txPin;       // 串口引脚
    bool echoEnabled;       // 回声测试
    bool debugMode;         // 调试模式
};
```

### 编译配置
在`platformio.ini`的`[env:esp32dev]`中添加：
```ini
build_src_filter = +<*> +<../usermods/rs485_control/rs485_control_simple.cpp>
```

## 🚨 常见问题和解决方案

### 问题1: Config页面不显示usermod
**原因**: 缺少正确的注册机制
**解决**: 确保使用`static`声明和`REGISTER_USERMOD`宏

### 问题2: 编译错误
**原因**: 复杂的类依赖或缺少头文件
**解决**: 简化实现，确保包含`wled.h`

### 问题3: 内存不足
**原因**: 字符串常量占用RAM
**解决**: 使用`PROGMEM`和`FPSTR`宏

## 🛠️ 开发工作流

### 1. 创建新usermod
```bash
mkdir usermods/my_usermod
# 创建.h和.cpp文件
# 实现Usermod接口
# 添加注册代码
```

### 2. 配置编译
```ini
# 在platformio.ini中添加
build_src_filter = +<*> +<../usermods/my_usermod/my_usermod.cpp>
```

### 3. 编译测试
```bash
pio run -e esp32dev
```

### 4. 验证功能
- 检查Config→Usermods页面
- 测试配置保存/读取
- 验证核心功能

## 📚 重要参考文档

### 项目文档
- `WLED二次开发完整指南.md` - 详细开发指南
- `RS485控制模块详细说明.md` - 功能说明
- `.kiro/project_knowledge_base.md` - 完整知识库

### 官方资源
- [WLED GitHub](https://github.com/Aircoookie/WLED)
- [WLED知识库](https://kno.wled.ge/)
- [Usermod示例](https://github.com/Aircoookie/WLED/tree/master/usermods)

## 🔍 调试技巧

### 启用调试输出
```cpp
DEBUG_PRINTLN(F("Debug message"));
DEBUG_PRINTF("Value: %d\n", value);
```

### 检查usermod注册
```bash
python check_usermod_registration.py
```

### 验证编译结果
```bash
python verify_rs485_compilation.py
```

## 💡 开发最佳实践

### 内存优化
```cpp
// 使用PROGMEM存储常量
const char _name[] PROGMEM = "MyUsermod";
// 使用FPSTR访问
JsonObject obj = root.createNestedObject(FPSTR(_name));
```

### 配置管理
```cpp
void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top["param"] = value;
}

bool readFromConfig(JsonObject& root) override {
    JsonObject top = root[FPSTR(_name)];
    if (top.isNull()) return false;
    value = top["param"] | defaultValue;
    return true;
}
```

### 性能优化
```cpp
void loop() override {
    static unsigned long lastRun = 0;
    if (millis() - lastRun < 100) return; // 限制频率
    lastRun = millis();
    // 主要逻辑
}
```

## 🎯 AI助手使用指南

### 当用户询问WLED开发问题时：
1. 首先参考本文档了解项目结构
2. 查看相关的.md文档获取详细信息
3. 检查现有代码实现作为参考
4. 提供基于WLED架构的解决方案

### 当需要修改现有功能时：
1. 定位到对应的源文件
2. 理解当前实现逻辑
3. 参考开发指南进行修改
4. 确保遵循WLED的开发规范

### 当需要添加新功能时：
1. 参考现有usermod的实现模式
2. 遵循WLED的接口规范
3. 注意内存和性能优化
4. 提供完整的配置和调试支持

---

**提示**: 这个上下文文件帮助AI助手快速理解WLED项目的结构和开发方法，提供准确和相关的技术支持。