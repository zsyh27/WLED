# WLED RS485控制功能 V4版本 烧录说明

## 📁 编译完成的文件位置

编译成功！固件文件位于：
- **主固件**: `.pio/build/esp32dev/firmware.bin`
- **引导程序**: `.pio/build/esp32dev/bootloader.bin`
- **分区表**: `.pio/build/esp32dev/partitions.bin`

## 🔧 烧录方法

### 方法1: 使用ESP32 Flash Download Tool (推荐)

1. **下载工具**: 从乐鑫官网下载 [ESP32 Flash Download Tool](https://www.espressif.com/en/support/download/other-tools)

2. **配置烧录参数**:
   ```
   bootloader.bin    -> 0x1000
   partitions.bin    -> 0x8000  
   firmware.bin      -> 0x10000
   ```

3. **串口设置**:
   - 波特率: 921600 (如果失败可尝试115200)
   - 串口: COM4 (根据你的实际端口)

4. **烧录步骤**:
   - 按住ESP32的BOOT按钮
   - 点击RESET按钮
   - 释放RESET按钮，继续按住BOOT按钮
   - 点击"START"开始烧录
   - 烧录开始后可以释放BOOT按钮

### 方法2: 使用esptool.py命令行

```bash
# 先擦除flash (重要!)
esptool.py --chip esp32 --port COM4 erase_flash

# 烧录固件
esptool.py --chip esp32 --port COM4 --baud 921600 write_flash -z 0x1000 .pio/build/esp32dev/bootloader.bin 0x8000 .pio/build/esp32dev/partitions.bin 0x10000 .pio/build/esp32dev/firmware.bin
```

### 方法3: 使用Arduino IDE

1. 将固件文件复制到Arduino项目目录
2. 使用Arduino IDE的"工具" -> "ESP32 Sketch Data Upload"

## ⚠️ 重要注意事项

1. **首次烧录V4版本必须完全擦除flash**:
   ```bash
   esptool.py --chip esp32 --port COM4 erase_flash
   ```

2. **V4版本不能通过OTA更新**，必须通过串口烧录

3. **如果烧录失败**，请尝试：
   - 降低波特率到115200
   - 确保ESP32进入下载模式（按住BOOT按钮）
   - 检查USB线缆质量
   - 尝试不同的USB端口

## 🧪 功能测试

烧录成功后，可以通过以下方式测试RS485功能：

### 1. Web界面配置
1. 连接ESP32的WiFi热点或配置WiFi
2. 打开WLED Web界面
3. 进入"Config" -> "Usermods"
4. 启用"RS485Control"
5. 配置GPIO26(RX)和GPIO27(TX)

### 2. 硬件连接
```
ESP32 GPIO26 → 485转TTL模块 RO (接收)
ESP32 GPIO27 → 485转TTL模块 DI (发送)
485转TTL模块 A+/B- → sscom的485接口
```

### 3. sscom测试指令
```
ECHO Hello World          # 回显测试
PRESET SET 1              # 激活预设1
PRESET SET 5              # 激活预设5  
PRESET GET                # 查询当前预设
STATUS                    # 查询系统状态
```

### 4. sscom配置
- 波特率: 9600
- 数据位: 8
- 停止位: 1
- 校验位: 无

## ✅ 已实现的功能

- ✅ RS485串口通信 (GPIO26/27)
- ✅ 回显服务 (ECHO指令)
- ✅ 预设控制 (PRESET SET/GET指令)
- ✅ 系统状态查询 (STATUS指令)
- ✅ 错误处理和诊断
- ✅ Web界面配置
- ✅ 调试模式支持

## 📊 编译信息

```
平台: ESP-IDF V4 (arduino-esp32 v2.0.9)
RAM使用: 24.6% (80,476 / 327,680 bytes)
Flash使用: 82.2% (1,293,541 / 1,572,864 bytes)
编译时间: 18.23秒
```

## 🔍 故障排除

如果遇到问题，请检查：

1. **编译问题**: 代码已验证无编译错误
2. **烧录问题**: 确保ESP32进入下载模式
3. **通信问题**: 检查485硬件连接和配置
4. **功能问题**: 通过Web界面查看usermod状态

---

**编译完成时间**: $(Get-Date)
**固件版本**: WLED V4 with RS485 Control
**开发者**: Kiro AI Assistant