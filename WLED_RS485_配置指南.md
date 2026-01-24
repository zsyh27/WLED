# WLED RS485串口控制 - 配置指南

## 快速配置

### 1. 基础配置步骤

#### 步骤1: 启用功能
在 `wled00/my_config.h` 中添加：
```cpp
#define USERMOD_RS485_CONTROL
```

#### 步骤2: 编译烧录
```bash
pio run -e esp32dev -t upload
```

#### 步骤3: Web界面配置
1. 访问WLED Web界面
2. Config → Usermods → RS485 Control
3. 启用功能并保存

#### 步骤4: 硬件连接
```
ESP32 GPIO26 → RS485模块 RO
ESP32 GPIO27 → RS485模块 DI
ESP32 3.3V   → RS485模块 VCC
ESP32 GND    → RS485模块 GND
```

## 详细配置选项

### Web界面配置参数

| 参数名称 | 默认值 | 可选值 | 说明 |
|---------|--------|--------|------|
| 启用功能 | false | true/false | 是否启用RS485功能 |
| 波特率 | 9600 | 9600/19200/38400/57600 | 串口通信速率 |
| 接收引脚 | 26 | 0-39 | UART接收引脚 |
| 发送引脚 | 27 | 0-33 | UART发送引脚 |
| 缓冲区大小 | 512 | 256-2048 | 接收缓冲区字节数 |
| 通信超时 | 1000 | 500-5000 | 超时时间（毫秒） |
| 回显功能 | true | true/false | 是否启用回显测试 |
| 调试模式 | false | true/false | 是否输出调试信息 |

### 配置文件格式

配置保存在WLED的JSON配置中：
```json
{
  "RS485Control": {
    "enabled": true,
    "baudRate": 9600,
    "rxPin": 26,
    "txPin": 27,
    "bufferSize": 512,
    "timeout": 1000,
    "echoEnabled": true,
    "debugMode": false
  }
}
```

## 硬件配置

### 推荐的RS485模块

#### 1. MAX485模块
```
特点：
- 成本低廉
- 易于使用
- 3.3V兼容

连接：
VCC → 3.3V
GND → GND
DI  → GPIO27
RO  → GPIO26
DE  → 3.3V (或连接到控制引脚)
RE  → GND (或连接到控制引脚)
A+  → 485总线A+
B-  → 485总线B-
```

#### 2. SP485模块
```
特点：
- 更好的ESD保护
- 更稳定的通信
- 支持更长距离

连接方式与MAX485相同
```

### 引脚选择指南

#### 接收引脚 (RX)
- **推荐**: GPIO26, GPIO25, GPIO34, GPIO35
- **避免**: GPIO6-11 (连接到Flash)
- **注意**: 某些引脚仅支持输入

#### 发送引脚 (TX)
- **推荐**: GPIO27, GPIO25, GPIO32, GPIO33
- **避免**: GPIO6-11 (连接到Flash)
- **注意**: 需要支持输出的引脚

#### 引脚冲突检查
确保选择的引脚不与其他功能冲突：
- LED数据引脚
- 按钮引脚
- I2C引脚
- SPI引脚

## 网络配置

### 多设备网络拓扑

#### 星型网络
```
中控主机
    |
USB转485
    |
RS485总线
    |
├── WLED设备1
├── WLED设备2
└── WLED设备3
```

#### 总线型网络
```
中控主机 → USB转485 → WLED设备1 → WLED设备2 → WLED设备3
```

### 总线终端电阻

在总线的两端添加120Ω终端电阻：
```
A+ ----[120Ω]---- B-
```

### 距离和速率关系

| 波特率 | 最大距离 | 推荐距离 |
|--------|----------|----------|
| 9600   | 1200m    | 800m     |
| 19200  | 600m     | 400m     |
| 38400  | 300m     | 200m     |
| 57600  | 200m     | 100m     |

## 性能优化配置

### 低延迟配置
```json
{
  "baudRate": 57600,
  "bufferSize": 256,
  "timeout": 500
}
```

### 高稳定性配置
```json
{
  "baudRate": 9600,
  "bufferSize": 1024,
  "timeout": 2000
}
```

### 多设备配置
```json
{
  "baudRate": 19200,
  "bufferSize": 512,
  "timeout": 1500
}
```

## 环境特定配置

### 工业环境
- 使用较低波特率（9600）
- 增加超时时间（2000ms）
- 启用调试模式进行监控
- 使用屏蔽线缆

### 舞台演出
- 使用中等波特率（19200）
- 标准缓冲区大小（512）
- 禁用调试模式减少干扰
- 预先测试所有预设

### 家庭自动化
- 使用标准波特率（9600）
- 启用回显功能便于调试
- 适中的超时设置（1000ms）

## 故障排除配置

### 通信问题诊断

#### 1. 启用调试模式
```json
{
  "debugMode": true
}
```

#### 2. 降低波特率
```json
{
  "baudRate": 9600
}
```

#### 3. 增加缓冲区
```json
{
  "bufferSize": 1024
}
```

#### 4. 延长超时时间
```json
{
  "timeout": 3000
}
```

### 性能问题优化

#### 减少内存使用
```json
{
  "bufferSize": 256
}
```

#### 提高响应速度
```json
{
  "timeout": 500,
  "baudRate": 38400
}
```

## 安全配置

### 访问控制
- 确保485总线物理安全
- 考虑添加指令验证
- 监控异常通信模式

### 错误恢复
- 启用自动重连机制
- 设置合理的超时时间
- 实现指令重试逻辑

## 配置验证

### 验证步骤

#### 1. 硬件连接验证
```bash
# 使用串口工具测试基本通信
echo "ECHO test" > /dev/ttyUSB0
```

#### 2. 软件配置验证
```bash
# 检查编译是否包含RS485功能
pio run -e esp32dev -v
```

#### 3. 功能测试验证
```bash
# 发送测试指令
ECHO Hello
PRESET SET 1
STATUS
```

### 配置检查清单

- [ ] my_config.h中已启用USERMOD_RS485_CONTROL
- [ ] 硬件连接正确
- [ ] Web界面配置已保存
- [ ] 波特率设置一致
- [ ] 引脚配置无冲突
- [ ] 终端电阻已安装
- [ ] 回显测试通过
- [ ] 预设控制正常

## 高级配置

### 自定义指令扩展
可以通过修改源码添加自定义指令：

1. 在CommandParser中添加新指令类型
2. 在主循环中添加处理逻辑
3. 更新帮助信息

### 多协议支持
可以扩展支持其他协议：
- Modbus RTU
- DMX512
- 自定义二进制协议

### 性能监控
启用性能监控以优化配置：
```json
{
  "debugMode": true,
  "performanceMonitoring": true
}
```

## 配置模板

### 基础模板
```json
{
  "RS485Control": {
    "enabled": true,
    "baudRate": 9600,
    "rxPin": 26,
    "txPin": 27,
    "bufferSize": 512,
    "timeout": 1000,
    "echoEnabled": true,
    "debugMode": false
  }
}
```

### 高性能模板
```json
{
  "RS485Control": {
    "enabled": true,
    "baudRate": 38400,
    "rxPin": 26,
    "txPin": 27,
    "bufferSize": 256,
    "timeout": 500,
    "echoEnabled": false,
    "debugMode": false
  }
}
```

### 调试模板
```json
{
  "RS485Control": {
    "enabled": true,
    "baudRate": 9600,
    "rxPin": 26,
    "txPin": 27,
    "bufferSize": 1024,
    "timeout": 2000,
    "echoEnabled": true,
    "debugMode": true
  }
}
```

---

**提示**: 配置更改后需要重启ESP32设备才能生效。建议在生产环境部署前充分测试配置。