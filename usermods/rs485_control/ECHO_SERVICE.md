# EchoService 实现文档

## 概述

EchoService类是WLED RS485控制功能的核心组件之一，负责实现消息回显功能。该服务用于验证RS485硬件连接是否正常工作，并为后续的指令解析和执行功能提供基础。

## 功能特性

### 1. 消息回显核心逻辑
- **完整消息回显**: 接收到的文本消息会原封不动地回复给发送方
- **特殊字符处理**: 正确处理换行符、回车符、制表符等特殊字符
- **转义序列支持**: 支持 `\n`, `\r`, `\t`, `\\` 等转义序列的处理

### 2. 启用/禁用控制
- **运行时控制**: 可以在运行时启用或禁用回显服务
- **配置持久化**: 启用/禁用状态会保存到WLED配置中
- **状态查询**: 提供状态查询接口用于诊断

### 3. 特殊字符和换行符处理
- **换行符保持**: 严格按照需求2.2，保持消息中的换行符
- **回车符支持**: 支持Windows风格的`\r\n`换行符
- **制表符保持**: 保持消息中的制表符用于格式化
- **非打印字符过滤**: 过滤掉非打印的控制字符

### 4. 主usermod循环集成
- **无缝集成**: 完全集成到RS485ControlUsermod的主循环中
- **性能优化**: 不影响WLED的其他功能和性能
- **错误处理**: 提供完善的错误处理和诊断信息

## 接口说明

### 核心方法

```cpp
// 初始化服务
void initialize(RS485Stats* stats, bool debugMode = false);

// 处理回显消息
String processEcho(const String& message);

// 处理ECHO指令
bool processEchoCommand(const String& message, String& response);

// 启用/禁用控制
void setEnabled(bool enable);
bool isEnabled() const;

// 调试模式控制
void setDebugMode(bool debug);

// 状态查询
String getStatus() const;

// 服务重置
void reset();
```

### 特殊字符处理

```cpp
// 处理特殊字符和转义序列
String processSpecialCharacters(const String& input);
```

## 需求映射

### 需求2.1: 消息回显
- ✅ **实现**: `processEcho()` 方法实现完整的消息回显
- ✅ **验证**: 接收到的消息原封不动地回复给发送方

### 需求2.2: 换行符保持
- ✅ **实现**: `processSpecialCharacters()` 方法保持换行符
- ✅ **验证**: 消息中的`\n`和`\r`字符被完整保留

### 需求2.5: 功能兼容性
- ✅ **实现**: 服务完全集成到usermod中，不影响其他WLED功能
- ✅ **验证**: 通过配置可以启用/禁用，不影响系统稳定性

## 使用示例

### 基本回显
```
输入: "Hello World"
输出: "Hello World"
```

### 换行符处理
```
输入: "Line 1\nLine 2"
输出: "Line 1\nLine 2"
```

### 转义序列处理
```
输入: "Line 1\\nLine 2"
输出: "Line 1\nLine 2"
```

### ECHO指令处理
```
输入: "ECHO Hello World"
输出: "Hello World"
```

## 配置选项

### WLED配置界面
- **echoEnabled**: 启用/禁用回显服务
- **debugMode**: 启用/禁用调试模式

### JSON API
```json
{
  "RS485Control": {
    "echoEnabled": true,
    "echoServiceStatus": "Echo Service: Enabled"
  }
}
```

## 调试和诊断

### 调试模式
启用调试模式后，服务会输出详细的处理信息：
```
EchoService: Processing echo - Input: 'Hello', Output: 'Hello'
EchoService: Echo command processed - Response: 'Hello'
```

### 状态查询
```cpp
String status = echoService.getStatus();
// 返回: "Echo Service: Enabled" 或 "Echo Service: Disabled"
```

## 测试验证

### 单元测试
- 基本回显功能测试
- 空消息处理测试
- 启用/禁用功能测试
- 换行符保持测试
- 转义序列处理测试
- ECHO指令处理测试

### 集成测试
- 与RS485Interface的集成测试
- 与CommandParser的集成测试
- WLED系统兼容性测试

## 性能考虑

### 内存使用
- 使用`String.reserve()`预分配内存，避免频繁的内存分配
- 及时释放临时字符串对象

### 处理效率
- 单次遍历处理特殊字符，时间复杂度O(n)
- 避免不必要的字符串复制操作

### 系统影响
- 不阻塞主循环执行
- 不影响WLED的其他功能
- 可配置的启用/禁用控制

## 错误处理

### 服务禁用
```cpp
if (!enabled) {
    response = "ERROR: Echo service is disabled";
    return false;
}
```

### 空消息处理
```cpp
if (message.length() == 0) {
    return "";
}
```

### 调试信息
调试模式下会输出详细的错误和处理信息，便于问题诊断。

## 未来扩展

### 可能的增强功能
1. **消息过滤**: 支持基于内容的消息过滤
2. **格式转换**: 支持不同的消息格式转换
3. **统计信息**: 记录回显消息的统计信息
4. **性能监控**: 监控处理延迟和吞吐量

### 配置扩展
1. **最大消息长度**: 可配置的最大回显消息长度
2. **字符过滤规则**: 可配置的特殊字符处理规则
3. **响应格式**: 可配置的响应消息格式