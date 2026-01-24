# 需求文档

## 介绍

为WLED项目添加基于485串口的文本指令控制功能，使中控主机能够通过485总线控制多个LED-BUG SR控制器的灯光效果。该功能将分阶段实现，首先验证硬件连接的回显功能，然后扩展到完整的指令解析和执行。

## 术语表

- **WLED_System**: 运行在ESP32-WROOM-32E上的WLED灯光控制系统
- **RS485_Interface**: 基于UART2的485串口通信接口
- **Command_Parser**: 文本指令解析器
- **Echo_Service**: 回显服务，用于验证通信连接
- **Preset_Controller**: WLED预设效果控制器
- **Central_Host**: 通过485总线发送控制指令的中控主机

## 需求

### 需求 1: 485串口通信基础设施

**用户故事:** 作为系统集成商，我需要通过485串口与WLED控制器通信，以便在复杂的灯光控制网络中集成多个设备。

#### 验收标准

1. WHEN 系统启动时，THE RS485_Interface SHALL 初始化UART2端口使用GPIO26作为RX和GPIO27作为TX
2. WHEN 配置串口参数时，THE RS485_Interface SHALL 设置波特率为9600、8数据位、1停止位、无校验位
3. WHEN 485模块连接时，THE RS485_Interface SHALL 支持收发自动切换功能
4. WHEN 接收数据时，THE RS485_Interface SHALL 将接收到的字节存储到接收缓冲区
5. WHEN 发送数据时，THE RS485_Interface SHALL 将数据写入UART2发送缓冲区

### 需求 2: 回显验证功能

**用户故事:** 作为开发人员，我需要验证485硬件连接是否正常工作，以便确保后续功能开发的基础。

#### 验收标准

1. WHEN 接收到任何文本消息时，THE Echo_Service SHALL 将相同的消息原封不动地回复给发送方
2. WHEN 消息包含换行符时，THE Echo_Service SHALL 保持换行符在回复中
3. WHEN 接收缓冲区满时，THE Echo_Service SHALL 处理缓冲区溢出并继续正常工作
4. WHEN 连续接收多条消息时，THE Echo_Service SHALL 按接收顺序依次回复每条消息
5. WHEN 回显功能启用时，THE WLED_System SHALL 继续正常运行其他功能

### 需求 3: 文本指令解析

**用户故事:** 作为中控系统操作员，我需要发送简单的文本指令来控制灯光效果，以便快速调整场景。

#### 验收标准

1. WHEN 接收到文本指令时，THE Command_Parser SHALL 解析指令格式并提取命令类型和参数
2. WHEN 指令格式无效时，THE Command_Parser SHALL 返回错误消息说明正确格式
3. WHEN 指令参数超出范围时，THE Command_Parser SHALL 返回参数范围错误消息
4. WHEN 解析成功时，THE Command_Parser SHALL 将解析结果传递给相应的控制器
5. WHEN 指令包含特殊字符时，THE Command_Parser SHALL 正确处理转义字符

### 需求 4: WLED预设效果控制

**用户故事:** 作为灯光控制操作员，我需要通过文本指令激活不同的WLED预设效果，以便远程控制灯光场景。

#### 验收标准

1. WHEN 接收到预设激活指令时，THE Preset_Controller SHALL 激活指定编号的WLED预设效果
2. WHEN 预设编号不存在时，THE Preset_Controller SHALL 返回预设不存在的错误消息
3. WHEN 预设激活成功时，THE Preset_Controller SHALL 返回确认消息包含预设编号和名称
4. WHEN 预设激活失败时，THE Preset_Controller SHALL 返回详细的错误信息
5. WHEN 查询当前预设时，THE Preset_Controller SHALL 返回当前激活的预设信息

### 需求 5: 系统集成和兼容性

**用户故事:** 作为WLED用户，我需要485控制功能不影响现有WLED功能，以便保持系统的稳定性和完整性。

#### 验收标准

1. WHEN 485功能运行时，THE WLED_System SHALL 保持所有现有Web界面功能正常工作
2. WHEN 485功能运行时，THE WLED_System SHALL 保持WiFi连接和网络功能正常
3. WHEN 485功能运行时，THE WLED_System SHALL 保持现有API接口正常响应
4. WHEN 系统资源不足时，THE WLED_System SHALL 优先保证核心灯光控制功能
5. WHEN 485功能禁用时，THE WLED_System SHALL 释放相关资源并正常运行

### 需求 6: 配置和管理

**用户故事:** 作为系统管理员，我需要配置485通信参数，以便适应不同的网络环境和需求。

#### 验收标准

1. WHEN 访问配置界面时，THE WLED_System SHALL 提供485功能的启用/禁用选项
2. WHEN 修改波特率时，THE WLED_System SHALL 支持常用波特率选择（9600, 19200, 38400, 57600），默认使用9600
3. WHEN 保存配置时，THE WLED_System SHALL 将485配置参数持久化存储
4. WHEN 重启系统时，THE WLED_System SHALL 根据保存的配置初始化485功能
5. WHEN 配置无效时，THE WLED_System SHALL 使用默认配置并显示警告信息

### 需求 7: 错误处理和诊断

**用户故事:** 作为技术支持人员，我需要获取485通信的状态和错误信息，以便快速诊断和解决问题。

#### 验收标准

1. WHEN 通信错误发生时，THE WLED_System SHALL 记录错误类型和时间戳到日志
2. WHEN 查询系统状态时，THE WLED_System SHALL 报告485接口的连接状态和统计信息
3. WHEN 缓冲区溢出时，THE WLED_System SHALL 清空缓冲区并记录溢出事件
4. WHEN 硬件故障时，THE WLED_System SHALL 检测并报告UART通信异常
5. WHEN 诊断模式启用时，THE WLED_System SHALL 输出详细的通信调试信息