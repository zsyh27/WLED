#include <Arduino.h>
#include <ArduinoJson.h>

// 简化的测试环境
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)

// 模拟WLED的getJsonValue函数
template<typename T>
bool getJsonValue(JsonVariant val, T& dest, T defaultVal) {
    if (val.isNull()) {
        dest = defaultVal;
        return false;
    }
    dest = val.as<T>();
    return true;
}

// 包含我们的配置结构
struct RS485Config {
    bool enabled = true;
    uint32_t baudRate = 9600;
    int rxPin = 26;
    int txPin = 27;
    size_t bufferSize = 512;
    uint32_t timeout = 1000;
    bool echoEnabled = true;
    bool debugMode = false;
};

// 配置验证结果
enum class ConfigValidationResult {
    VALID,
    INVALID_BAUD_RATE,
    INVALID_GPIO_PIN,
    INVALID_BUFFER_SIZE,
    INVALID_TIMEOUT,
    MEMORY_ERROR
};

struct ConfigValidationInfo {
    ConfigValidationResult result;
    String message;
    bool hasWarnings;
    String warnings;
};

// 简化的ConfigManager用于测试
class TestConfigManager {
private:
    RS485Config* config;
    RS485Config defaultConfig;
    bool debugMode;
    String lastValidationMessage;
    String lastWarningMessage;
    
    static const uint32_t SUPPORTED_BAUD_RATES[];
    static const size_t SUPPORTED_BAUD_RATES_COUNT;
    
    bool isValidBaudRate(uint32_t baudRate) const {
        for (size_t i = 0; i < SUPPORTED_BAUD_RATES_COUNT; i++) {
            if (SUPPORTED_BAUD_RATES[i] == baudRate) {
                return true;
            }
        }
        return false;
    }
    
    bool isValidGpioPin(int pin) const {
        if (pin < 0 || pin > 39) return false;
        if (pin >= 6 && pin <= 11) return false;
        return true;
    }
    
    bool isValidBufferSize(size_t size) const {
        return size >= 64 && size <= 4096;
    }
    
    bool isValidTimeout(uint32_t timeout) const {
        return timeout >= 100 && timeout <= 30000;
    }
    
    void initializeDefaults() {
        defaultConfig.enabled = true;
        defaultConfig.baudRate = 9600;
        defaultConfig.rxPin = 26;
        defaultConfig.txPin = 27;
        defaultConfig.bufferSize = 512;
        defaultConfig.timeout = 1000;
        defaultConfig.echoEnabled = true;
        defaultConfig.debugMode = false;
    }

public:
    TestConfigManager() : config(nullptr), debugMode(false) {
        initializeDefaults();
    }
    
    void initialize(RS485Config* cfg, bool debug = false) {
        config = cfg;
        debugMode = debug;
    }
    
    ConfigValidationInfo validateConfig(const RS485Config& cfg) const {
        ConfigValidationInfo info;
        info.result = ConfigValidationResult::VALID;
        info.hasWarnings = false;
        info.message = "";
        info.warnings = "";
        
        // 验证波特率
        if (!isValidBaudRate(cfg.baudRate)) {
            info.result = ConfigValidationResult::INVALID_BAUD_RATE;
            info.message = "Invalid baud rate: " + String(cfg.baudRate) + 
                          ". Supported rates: " + getSupportedBaudRatesString();
            return info;
        }
        
        // 验证GPIO引脚
        if (!isValidGpioPin(cfg.rxPin)) {
            info.result = ConfigValidationResult::INVALID_GPIO_PIN;
            info.message = "Invalid RX pin: " + String(cfg.rxPin);
            return info;
        }
        
        if (!isValidGpioPin(cfg.txPin)) {
            info.result = ConfigValidationResult::INVALID_GPIO_PIN;
            info.message = "Invalid TX pin: " + String(cfg.txPin);
            return info;
        }
        
        if (cfg.rxPin == cfg.txPin) {
            info.result = ConfigValidationResult::INVALID_GPIO_PIN;
            info.message = "RX and TX pins cannot be the same";
            return info;
        }
        
        // 验证缓冲区大小
        if (!isValidBufferSize(cfg.bufferSize)) {
            info.result = ConfigValidationResult::INVALID_BUFFER_SIZE;
            info.message = "Invalid buffer size: " + String(cfg.bufferSize);
            return info;
        }
        
        // 验证超时时间
        if (!isValidTimeout(cfg.timeout)) {
            info.result = ConfigValidationResult::INVALID_TIMEOUT;
            info.message = "Invalid timeout: " + String(cfg.timeout);
            return info;
        }
        
        // 添加警告
        if (cfg.bufferSize > 1024) {
            info.hasWarnings = true;
            info.warnings += "Large buffer size may consume significant memory. ";
        }
        
        if (cfg.timeout < 500) {
            info.hasWarnings = true;
            info.warnings += "Short timeout may cause communication issues. ";
        }
        
        info.message = "Configuration is valid";
        return info;
    }
    
    bool restoreFromDefaults(String& reason) {
        if (!config) return false;
        
        *config = defaultConfig;
        lastValidationMessage = "Configuration restored to defaults: " + reason;
        
        Serial.printf("TestConfigManager: Restored defaults - %s\n", reason.c_str());
        return true;
    }
    
    static String getSupportedBaudRatesString() {
        String rates = "";
        for (size_t i = 0; i < SUPPORTED_BAUD_RATES_COUNT; i++) {
            if (i > 0) rates += ", ";
            rates += String(SUPPORTED_BAUD_RATES[i]);
        }
        return rates;
    }
    
    static uint32_t getDefaultBaudRate() { 
        return 9600; 
    }
    
    bool isConfigValid() const {
        if (!config) return false;
        ConfigValidationInfo validation = validateConfig(*config);
        return validation.result == ConfigValidationResult::VALID;
    }
    
    String getValidationMessage() const { 
        return lastValidationMessage; 
    }
    
    String getWarningMessage() const {
        return lastWarningMessage;
    }
};

// 定义支持的波特率
const uint32_t TestConfigManager::SUPPORTED_BAUD_RATES[] = {9600, 19200, 38400, 57600};
const size_t TestConfigManager::SUPPORTED_BAUD_RATES_COUNT = 4;

// 测试函数
void testConfigValidation() {
    Serial.println("=== 测试配置验证功能 ===");
    
    RS485Config testConfig;
    TestConfigManager configManager;
    configManager.initialize(&testConfig, true);
    
    // 测试1: 有效配置
    Serial.println("\n测试1: 有效配置");
    ConfigValidationInfo result = configManager.validateConfig(testConfig);
    Serial.printf("结果: %s\n", result.result == ConfigValidationResult::VALID ? "有效" : "无效");
    Serial.printf("消息: %s\n", result.message.c_str());
    
    // 测试2: 无效波特率
    Serial.println("\n测试2: 无效波特率");
    testConfig.baudRate = 115200;  // 不支持的波特率
    result = configManager.validateConfig(testConfig);
    Serial.printf("结果: %s\n", result.result == ConfigValidationResult::INVALID_BAUD_RATE ? "正确检测到无效波特率" : "检测失败");
    Serial.printf("消息: %s\n", result.message.c_str());
    
    // 测试3: 无效GPIO引脚
    Serial.println("\n测试3: 无效GPIO引脚");
    testConfig.baudRate = 9600;  // 恢复有效波特率
    testConfig.rxPin = 50;       // 无效引脚
    result = configManager.validateConfig(testConfig);
    Serial.printf("结果: %s\n", result.result == ConfigValidationResult::INVALID_GPIO_PIN ? "正确检测到无效引脚" : "检测失败");
    Serial.printf("消息: %s\n", result.message.c_str());
    
    // 测试4: 相同的RX和TX引脚
    Serial.println("\n测试4: 相同的RX和TX引脚");
    testConfig.rxPin = 26;
    testConfig.txPin = 26;  // 与RX相同
    result = configManager.validateConfig(testConfig);
    Serial.printf("结果: %s\n", result.result == ConfigValidationResult::INVALID_GPIO_PIN ? "正确检测到引脚冲突" : "检测失败");
    Serial.printf("消息: %s\n", result.message.c_str());
    
    // 测试5: 无效缓冲区大小
    Serial.println("\n测试5: 无效缓冲区大小");
    testConfig.txPin = 27;      // 恢复有效引脚
    testConfig.bufferSize = 32; // 太小
    result = configManager.validateConfig(testConfig);
    Serial.printf("结果: %s\n", result.result == ConfigValidationResult::INVALID_BUFFER_SIZE ? "正确检测到无效缓冲区大小" : "检测失败");
    Serial.printf("消息: %s\n", result.message.c_str());
    
    // 测试6: 带警告的有效配置
    Serial.println("\n测试6: 带警告的有效配置");
    testConfig.bufferSize = 2048;  // 大缓冲区
    testConfig.timeout = 200;      // 短超时
    result = configManager.validateConfig(testConfig);
    Serial.printf("结果: %s\n", result.result == ConfigValidationResult::VALID ? "有效" : "无效");
    Serial.printf("有警告: %s\n", result.hasWarnings ? "是" : "否");
    Serial.printf("警告: %s\n", result.warnings.c_str());
    
    Serial.println("\n=== 配置验证测试完成 ===");
}

void testDefaultRestore() {
    Serial.println("\n=== 测试默认配置恢复 ===");
    
    RS485Config testConfig;
    TestConfigManager configManager;
    configManager.initialize(&testConfig, true);
    
    // 设置无效配置
    testConfig.baudRate = 115200;  // 无效
    testConfig.rxPin = 50;         // 无效
    
    Serial.printf("恢复前 - 波特率: %d, RX引脚: %d\n", testConfig.baudRate, testConfig.rxPin);
    
    // 恢复默认配置
    String reason = "测试恢复";
    bool success = configManager.restoreFromDefaults(reason);
    
    Serial.printf("恢复结果: %s\n", success ? "成功" : "失败");
    Serial.printf("恢复后 - 波特率: %d, RX引脚: %d\n", testConfig.baudRate, testConfig.rxPin);
    Serial.printf("验证消息: %s\n", configManager.getValidationMessage().c_str());
    
    // 验证恢复后的配置是否有效
    bool isValid = configManager.isConfigValid();
    Serial.printf("恢复后配置有效性: %s\n", isValid ? "有效" : "无效");
    
    Serial.println("=== 默认配置恢复测试完成 ===");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("开始RS485配置管理测试...");
    
    testConfigValidation();
    testDefaultRestore();
    
    Serial.println("\n所有测试完成!");
}

void loop() {
    // 空循环
}