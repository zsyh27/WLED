/*
 * RS485控制功能检查点验证测试
 * 这是一个独立的测试程序，用于验证基础通信功能
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>

// 模拟Arduino String类
class String {
private:
    std::string data;
    
public:
    String() {}
    String(const char* str) : data(str) {}
    String(const std::string& str) : data(str) {}
    
    size_t length() const { return data.length(); }
    const char* c_str() const { return data.c_str(); }
    
    String substring(size_t start) const {
        return String(data.substr(start));
    }
    
    String substring(size_t start, size_t end) const {
        return String(data.substr(start, end - start));
    }
    
    bool startsWith(const String& prefix) const {
        return data.find(prefix.data) == 0;
    }
    
    String& operator+=(const String& other) {
        data += other.data;
        return *this;
    }
    
    String& operator+=(char c) {
        data += c;
        return *this;
    }
    
    char operator[](size_t index) const {
        return data[index];
    }
    
    bool operator==(const String& other) const {
        return data == other.data;
    }
    
    void reserve(size_t capacity) {
        data.reserve(capacity);
    }
};

// 模拟millis()函数
unsigned long millis() {
    return 1000; // 固定返回值用于测试
}

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

// 简化的循环缓冲区实现
template<typename T>
class CircularBuffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    size_t overflowCount;

public:
    CircularBuffer(size_t size) : capacity(size), head(0), tail(0), count(0), overflowCount(0) {
        buffer.resize(capacity);
    }
    
    bool push(const T& item) {
        if (count >= capacity) {
            overflowCount++;
            return false;
        }
        buffer[head] = item;
        head = (head + 1) % capacity;
        count++;
        return true;
    }
    
    bool pop(T& item) {
        if (count == 0) return false;
        item = buffer[tail];
        tail = (tail + 1) % capacity;
        count--;
        return true;
    }
    
    size_t size() const { return count; }
    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count >= capacity; }
    size_t getCapacity() const { return capacity; }
    size_t available() const { return capacity - count; }
    uint8_t getUtilization() const { return (count * 100) / capacity; }
    size_t getOverflowCount() const { return overflowCount; }
    bool hasOverflowed() const { return overflowCount > 0; }
    void clear() { head = tail = count = 0; }
    void resetOverflowCount() { overflowCount = 0; }
};

// EchoService类
class EchoService {
private:
    bool enabled;
    RS485Stats* stats;
    bool debugMode;

public:
    EchoService() : enabled(true), stats(nullptr), debugMode(false) {}
    
    void initialize(RS485Stats* st, bool debug = false) {
        stats = st;
        debugMode = debug;
    }
    
    String processEcho(const String& message) {
        if (!enabled) return "";
        if (message.length() == 0) return "";
        
        return processSpecialCharacters(message);
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
                            case 'n': output += '\n'; i++; break;
                            case 'r': output += '\r'; i++; break;
                            case 't': output += '\t'; i++; break;
                            case '\\': output += '\\'; i++; break;
                            default: output += c; break;
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
    
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    void setDebugMode(bool debug) { debugMode = debug; }
    
    String getStatus() const {
        String status = "Echo Service: ";
        status += enabled ? "Enabled" : "Disabled";
        if (debugMode) status += " (Debug Mode)";
        return status;
    }
};

// RS485Interface类
class RS485Interface {
private:
    CircularBuffer<char>* rxBuffer;
    CircularBuffer<char>* txBuffer;
    RS485Config* config;
    RS485Stats* stats;
    bool initialized;

public:
    RS485Interface() : rxBuffer(nullptr), txBuffer(nullptr), config(nullptr), stats(nullptr), initialized(false) {}
    
    ~RS485Interface() {
        delete rxBuffer;
        delete txBuffer;
    }
    
    bool initialize(RS485Config* cfg, RS485Stats* st) {
        config = cfg;
        stats = st;
        
        if (!config->enabled) return false;
        
        rxBuffer = new CircularBuffer<char>(config->bufferSize);
        txBuffer = new CircularBuffer<char>(config->bufferSize);
        
        if (!rxBuffer || !txBuffer) return false;
        
        stats->connected = true;
        stats->messagesReceived = 0;
        stats->messagesSent = 0;
        stats->parseErrors = 0;
        stats->bufferOverflows = 0;
        stats->timeouts = 0;
        stats->lastActivity = millis();
        
        initialized = true;
        return true;
    }
    
    bool sendMessage(const String& message) {
        if (!initialized || !config->enabled) return false;
        if (message.length() == 0) return false;
        
        // 模拟发送消息到缓冲区
        for (size_t i = 0; i < message.length(); i++) {
            if (!txBuffer->push(message[i])) {
                stats->bufferOverflows++;
                return false;
            }
        }
        
        stats->messagesSent++;
        return true;
    }
    
    bool hasMessage() {
        return initialized && !rxBuffer->isEmpty();
    }
    
    String receiveMessage() {
        if (!initialized) return "";
        
        String message = "";
        char c;
        
        while (rxBuffer->pop(c)) {
            if (c == '\n' || c == '\r') {
                break;
            } else if (c >= 32 && c <= 126) {
                message += c;
            }
        }
        
        if (message.length() > 0) {
            stats->messagesReceived++;
        }
        
        return message;
    }
    
    // 模拟接收消息的方法（用于测试）
    void simulateReceive(const String& message) {
        if (!initialized) return;
        
        for (size_t i = 0; i < message.length(); i++) {
            rxBuffer->push(message[i]);
        }
        rxBuffer->push('\n'); // 添加换行符
    }
    
    bool isInitialized() const { return initialized; }
    bool isConnected() const { return initialized && stats && stats->connected; }
    
    RS485Stats getStats() {
        if (stats) return *stats;
        return RS485Stats();
    }
    
    void clearBuffers() {
        if (rxBuffer) rxBuffer->clear();
        if (txBuffer) txBuffer->clear();
    }
    
    size_t getReceiveBufferSize() const {
        return rxBuffer ? rxBuffer->size() : 0;
    }
    
    size_t getTransmitBufferSize() const {
        return txBuffer ? txBuffer->size() : 0;
    }
    
    bool areBuffersHealthy() const {
        if (!rxBuffer || !txBuffer) return false;
        return rxBuffer->getUtilization() < 90 && txBuffer->getUtilization() < 90;
    }
};

// 测试框架
class TestFramework {
private:
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;

public:
    void assert_true(bool condition, const std::string& message) {
        totalTests++;
        if (condition) {
            passedTests++;
            std::cout << "✓ " << message << std::endl;
        } else {
            failedTests++;
            std::cout << "✗ " << message << " - FAILED" << std::endl;
        }
    }
    
    void assert_equal_string(const std::string& expected, const std::string& actual, const std::string& message) {
        totalTests++;
        if (expected == actual) {
            passedTests++;
            std::cout << "✓ " << message << std::endl;
        } else {
            failedTests++;
            std::cout << "✗ " << message << " - FAILED" << std::endl;
            std::cout << "  Expected: '" << expected << "'" << std::endl;
            std::cout << "  Actual: '" << actual << "'" << std::endl;
        }
    }
    
    void assert_equal_int(int expected, int actual, const std::string& message) {
        totalTests++;
        if (expected == actual) {
            passedTests++;
            std::cout << "✓ " << message << std::endl;
        } else {
            failedTests++;
            std::cout << "✗ " << message << " - FAILED" << std::endl;
            std::cout << "  Expected: " << expected << std::endl;
            std::cout << "  Actual: " << actual << std::endl;
        }
    }
    
    void printSummary() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "测试总结:" << std::endl;
        std::cout << "总测试数: " << totalTests << std::endl;
        std::cout << "通过: " << passedTests << std::endl;
        std::cout << "失败: " << failedTests << std::endl;
        std::cout << "成功率: " << (totalTests > 0 ? (passedTests * 100 / totalTests) : 0) << "%" << std::endl;
        std::cout << "========================================" << std::endl;
        
        if (failedTests == 0) {
            std::cout << "🎉 所有测试通过！基础通信功能验证成功！" << std::endl;
        } else {
            std::cout << "⚠️  有测试失败，需要检查实现。" << std::endl;
        }
    }
    
    bool allTestsPassed() const {
        return failedTests == 0;
    }
};

// 全局测试实例
TestFramework test;
RS485Interface rs485Interface;
EchoService echoService;
RS485Config testConfig;
RS485Stats testStats;

void setUp() {
    // 每个测试前初始化
    testConfig.enabled = true;
    testConfig.baudRate = 9600;
    testConfig.rxPin = 26;
    testConfig.txPin = 27;
    testConfig.bufferSize = 512;
    testConfig.timeout = 1000;
    testConfig.echoEnabled = true;
    testConfig.debugMode = false;
    
    rs485Interface.initialize(&testConfig, &testStats);
    echoService.initialize(&testStats, false);
    echoService.setEnabled(true);
}

void tearDown() {
    // 每个测试后清理
    rs485Interface.clearBuffers();
    echoService.setEnabled(true);
}

// 测试函数
void test_rs485_interface_initialization() {
    std::cout << "\n测试1: RS485接口初始化" << std::endl;
    
    test.assert_true(rs485Interface.isInitialized(), "RS485接口应该已初始化");
    test.assert_true(rs485Interface.isConnected(), "RS485接口应该已连接");
    test.assert_true(rs485Interface.areBuffersHealthy(), "缓冲区应该健康");
    
    RS485Stats stats = rs485Interface.getStats();
    test.assert_true(stats.connected, "统计信息应显示已连接");
    test.assert_equal_int(0, stats.messagesReceived, "初始接收消息数应为0");
    test.assert_equal_int(0, stats.messagesSent, "初始发送消息数应为0");
}

void test_basic_echo_functionality() {
    std::cout << "\n测试2: 基本回显功能" << std::endl;
    
    String input = "Hello World";
    String output = echoService.processEcho(input);
    
    test.assert_equal_string("Hello World", output.c_str(), "基本回显应返回相同内容");
}

void test_newline_preservation() {
    std::cout << "\n测试3: 换行符保持功能 (需求2.2)" << std::endl;
    
    String input = "Line 1\nLine 2";
    String output = echoService.processEcho(input);
    
    test.assert_equal_string("Line 1\nLine 2", output.c_str(), "应保持换行符");
}

void test_echo_command_processing() {
    std::cout << "\n测试4: ECHO指令处理" << std::endl;
    
    String command = "ECHO Hello World";
    String response;
    
    bool result = echoService.processEchoCommand(command, response);
    
    test.assert_true(result, "ECHO指令处理应成功");
    test.assert_equal_string("Hello World", response.c_str(), "ECHO指令应返回正确内容");
}

void test_message_send_receive() {
    std::cout << "\n测试5: 消息发送和接收" << std::endl;
    
    // 测试发送消息
    String testMessage = "Test Message";
    bool sendResult = rs485Interface.sendMessage(testMessage);
    test.assert_true(sendResult, "消息发送应成功");
    
    RS485Stats stats = rs485Interface.getStats();
    test.assert_equal_int(1, stats.messagesSent, "发送消息计数应为1");
    
    // 模拟接收消息
    rs485Interface.simulateReceive("Received Message");
    test.assert_true(rs485Interface.hasMessage(), "应检测到接收的消息");
    
    String receivedMessage = rs485Interface.receiveMessage();
    test.assert_equal_string("Received Message", receivedMessage.c_str(), "接收的消息应正确");
    
    stats = rs485Interface.getStats();
    test.assert_equal_int(1, stats.messagesReceived, "接收消息计数应为1");
}

void test_end_to_end_echo() {
    std::cout << "\n测试6: 端到端回显测试" << std::endl;
    
    // 模拟接收ECHO指令
    rs485Interface.simulateReceive("ECHO Test Echo");
    test.assert_true(rs485Interface.hasMessage(), "应检测到ECHO指令");
    
    // 接收并处理消息
    String receivedCommand = rs485Interface.receiveMessage();
    test.assert_equal_string("ECHO Test Echo", receivedCommand.c_str(), "接收的指令应正确");
    
    // 处理回显指令
    String response;
    bool result = echoService.processEchoCommand(receivedCommand, response);
    test.assert_true(result, "ECHO指令处理应成功");
    test.assert_equal_string("Test Echo", response.c_str(), "回显响应应正确");
    
    // 发送回复
    bool sendResult = rs485Interface.sendMessage(response);
    test.assert_true(sendResult, "响应发送应成功");
    
    // 验证统计信息
    RS485Stats stats = rs485Interface.getStats();
    test.assert_equal_int(1, stats.messagesReceived, "接收消息计数应为1");
    test.assert_equal_int(1, stats.messagesSent, "发送消息计数应为1");
}

void test_buffer_health() {
    std::cout << "\n测试7: 缓冲区健康状态" << std::endl;
    
    test.assert_true(rs485Interface.areBuffersHealthy(), "缓冲区应该健康");
    test.assert_equal_int(0, rs485Interface.getReceiveBufferSize(), "接收缓冲区应为空");
    test.assert_equal_int(0, rs485Interface.getTransmitBufferSize(), "发送缓冲区应为空");
    
    // 添加一些数据
    rs485Interface.sendMessage("Test");
    test.assert_true(rs485Interface.getTransmitBufferSize() > 0, "发送缓冲区应有数据");
}

void test_enable_disable_functionality() {
    std::cout << "\n测试8: 启用/禁用功能" << std::endl;
    
    // 测试启用状态
    echoService.setEnabled(true);
    test.assert_true(echoService.isEnabled(), "回显服务应已启用");
    
    String input = "Test message";
    String output = echoService.processEcho(input);
    test.assert_equal_string("Test message", output.c_str(), "启用时应正常回显");
    
    // 测试禁用状态
    echoService.setEnabled(false);
    test.assert_true(!echoService.isEnabled(), "回显服务应已禁用");
    
    output = echoService.processEcho(input);
    test.assert_equal_string("", output.c_str(), "禁用时应返回空字符串");
}

int main() {
    std::cout << "开始RS485控制功能检查点验证..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 运行所有测试
    setUp();
    test_rs485_interface_initialization();
    tearDown();
    
    setUp();
    test_basic_echo_functionality();
    tearDown();
    
    setUp();
    test_newline_preservation();
    tearDown();
    
    setUp();
    test_echo_command_processing();
    tearDown();
    
    setUp();
    test_message_send_receive();
    tearDown();
    
    setUp();
    test_end_to_end_echo();
    tearDown();
    
    setUp();
    test_buffer_health();
    tearDown();
    
    setUp();
    test_enable_disable_functionality();
    tearDown();
    
    // 打印测试总结
    test.printSummary();
    
    return test.allTestsPassed() ? 0 : 1;
}