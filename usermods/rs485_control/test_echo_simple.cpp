// 简单的EchoService功能验证
// 这个文件可以用来手动验证EchoService的核心功能

#include <Arduino.h>

// 模拟WLED的调试宏
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)

// 包含必要的结构体定义
struct RS485Stats {
    uint32_t messagesReceived = 0;
    uint32_t messagesSent = 0;
    uint32_t parseErrors = 0;
    uint32_t bufferOverflows = 0;
    uint32_t timeouts = 0;
    uint32_t lastActivity = 0;
    bool connected = false;
};

// EchoService类的简化版本用于测试
class EchoServiceTest {
private:
    bool enabled;
    RS485Stats* stats;
    bool debugMode;

public:
    EchoServiceTest() : enabled(true), stats(nullptr), debugMode(false) {}
    
    void initialize(RS485Stats* st, bool debug = false) {
        stats = st;
        debugMode = debug;
        Serial.println("EchoService初始化完成");
    }
    
    String processEcho(const String& message) {
        if (!enabled) {
            Serial.println("回显服务已禁用，消息被忽略");
            return "";
        }
        
        if (message.length() == 0) {
            Serial.println("接收到空消息");
            return "";
        }
        
        String processedMessage = processSpecialCharacters(message);
        
        if (debugMode) {
            Serial.printf("处理回显 - 输入: '%s', 输出: '%s'\n", 
                        message.c_str(), processedMessage.c_str());
        }
        
        return processedMessage;
    }
    
    String processSpecialCharacters(const String& input) {
        String output = "";
        output.reserve(input.length() + 10);
        
        for (size_t i = 0; i < input.length(); i++) {
            char c = input[i];
            
            switch (c) {
                case '\n':
                case '\r':
                case '\t':
                    output += c;
                    break;
                    
                case '\\':
                    if (i + 1 < input.length()) {
                        char nextChar = input[i + 1];
                        switch (nextChar) {
                            case 'n':
                                output += '\n';
                                i++;
                                break;
                            case 'r':
                                output += '\r';
                                i++;
                                break;
                            case 't':
                                output += '\t';
                                i++;
                                break;
                            case '\\':
                                output += '\\';
                                i++;
                                break;
                            default:
                                output += c;
                                break;
                        }
                    } else {
                        output += c;
                    }
                    break;
                    
                default:
                    if (c >= 32 && c <= 126) {
                        output += c;
                    } else if (c == '\n' || c == '\r' || c == '\t') {
                        output += c;
                    }
                    break;
            }
        }
        
        return output;
    }
    
    bool processEchoCommand(const String& message, String& response) {
        if (!enabled) {
            response = "ERROR: Echo service is disabled";
            return false;
        }
        
        String echoMessage = message;
        if (message.startsWith("ECHO ")) {
            echoMessage = message.substring(5);
        }
        
        response = processEcho(echoMessage);
        return true;
    }
    
    void setEnabled(bool enable) { 
        enabled = enable; 
        Serial.printf("回显服务已%s\n", enable ? "启用" : "禁用");
    }
    
    bool isEnabled() const { return enabled; }
    void setDebugMode(bool debug) { debugMode = debug; }
    
    String getStatus() const {
        String status = "Echo Service: ";
        status += enabled ? "Enabled" : "Disabled";
        if (debugMode) {
            status += " (Debug Mode)";
        }
        return status;
    }
};

// 测试函数
void testBasicEcho() {
    Serial.println("\n=== 测试基本回显功能 ===");
    EchoServiceTest echo;
    RS485Stats stats;
    echo.initialize(&stats, true);
    
    String input = "Hello World";
    String output = echo.processEcho(input);
    
    Serial.printf("输入: '%s'\n", input.c_str());
    Serial.printf("输出: '%s'\n", output.c_str());
    Serial.printf("测试结果: %s\n", (input == output) ? "通过" : "失败");
}

void testNewlinePreservation() {
    Serial.println("\n=== 测试换行符保持功能 ===");
    EchoServiceTest echo;
    RS485Stats stats;
    echo.initialize(&stats, true);
    
    String input = "Line 1\nLine 2";
    String output = echo.processEcho(input);
    
    Serial.printf("输入: '%s'\n", input.c_str());
    Serial.printf("输出: '%s'\n", output.c_str());
    Serial.printf("测试结果: %s\n", (input == output) ? "通过" : "失败");
}

void testEscapeSequences() {
    Serial.println("\n=== 测试转义序列处理 ===");
    EchoServiceTest echo;
    RS485Stats stats;
    echo.initialize(&stats, true);
    
    String input = "Line 1\\nLine 2";
    String expected = "Line 1\nLine 2";
    String output = echo.processEcho(input);
    
    Serial.printf("输入: '%s'\n", input.c_str());
    Serial.printf("期望: '%s'\n", expected.c_str());
    Serial.printf("输出: '%s'\n", output.c_str());
    Serial.printf("测试结果: %s\n", (expected == output) ? "通过" : "失败");
}

void testEnableDisable() {
    Serial.println("\n=== 测试启用/禁用功能 ===");
    EchoServiceTest echo;
    RS485Stats stats;
    echo.initialize(&stats, true);
    
    String input = "Test message";
    
    // 测试启用状态
    echo.setEnabled(true);
    String output1 = echo.processEcho(input);
    Serial.printf("启用状态 - 输入: '%s', 输出: '%s'\n", input.c_str(), output1.c_str());
    
    // 测试禁用状态
    echo.setEnabled(false);
    String output2 = echo.processEcho(input);
    Serial.printf("禁用状态 - 输入: '%s', 输出: '%s'\n", input.c_str(), output2.c_str());
    
    bool test1 = (input == output1);
    bool test2 = (output2 == "");
    Serial.printf("测试结果: %s\n", (test1 && test2) ? "通过" : "失败");
}

void testEchoCommand() {
    Serial.println("\n=== 测试ECHO指令处理 ===");
    EchoServiceTest echo;
    RS485Stats stats;
    echo.initialize(&stats, true);
    
    String command = "ECHO Hello World";
    String response;
    
    bool result = echo.processEchoCommand(command, response);
    
    Serial.printf("指令: '%s'\n", command.c_str());
    Serial.printf("响应: '%s'\n", response.c_str());
    Serial.printf("处理结果: %s\n", result ? "成功" : "失败");
    Serial.printf("测试结果: %s\n", (result && response == "Hello World") ? "通过" : "失败");
}

void runAllTests() {
    Serial.println("开始EchoService功能测试...\n");
    
    testBasicEcho();
    testNewlinePreservation();
    testEscapeSequences();
    testEnableDisable();
    testEchoCommand();
    
    Serial.println("\n所有测试完成！");
}

// 如果这个文件被直接编译运行，执行测试
#ifdef ECHO_SERVICE_TEST_MAIN
void setup() {
    Serial.begin(115200);
    delay(2000);
    runAllTests();
}

void loop() {
    // 空循环
}
#endif