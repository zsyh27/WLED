#include <Arduino.h>
#include <unity.h>

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

// 简化的循环缓冲区实现用于测试
template<typename T>
class CircularBuffer {
private:
    T* buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    size_t overflowCount;

public:
    CircularBuffer(size_t size) : capacity(size), head(0), tail(0), count(0), overflowCount(0) {
        buffer = new T[capacity];
    }
    
    ~CircularBuffer() {
        delete[] buffer;
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

// EchoService类的测试版本
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

// 简化的RS485Interface类用于测试
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
        if (rxBuffer) delete rxBuffer;
        if (txBuffer) delete txBuffer;
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

// 测试实例
RS485Interface rs485Interface;
EchoService echoService;
RS485Config testConfig;
RS485Stats testStats;

void setUp(void) {
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

void tearDown(void) {
    // 每个测试后清理
    rs485Interface.clearBuffers();
    echoService.setEnabled(true);
}

// 测试1: RS485接口初始化
void test_rs485_interface_initialization() {
    Serial.println("测试RS485接口初始化...");
    
    TEST_ASSERT_TRUE(rs485Interface.isInitialized());
    TEST_ASSERT_TRUE(rs485Interface.isConnected());
    TEST_ASSERT_TRUE(rs485Interface.areBuffersHealthy());
    
    RS485Stats stats = rs485Interface.getStats();
    TEST_ASSERT_TRUE(stats.connected);
    TEST_ASSERT_EQUAL(0, stats.messagesReceived);
    TEST_ASSERT_EQUAL(0, stats.messagesSent);
    
    Serial.println("✓ RS485接口初始化测试通过");
}

// 测试2: 基本回显功能
void test_basic_echo_functionality() {
    Serial.println("测试基本回显功能...");
    
    String input = "Hello World";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Hello World", output.c_str());
    
    Serial.println("✓ 基本回显功能测试通过");
}

// 测试3: 换行符保持功能 (需求2.2)
void test_newline_preservation() {
    Serial.println("测试换行符保持功能...");
    
    String input = "Line 1\nLine 2";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Line 1\nLine 2", output.c_str());
    
    Serial.println("✓ 换行符保持功能测试通过");
}

// 测试4: ECHO指令处理
void test_echo_command_processing() {
    Serial.println("测试ECHO指令处理...");
    
    String command = "ECHO Hello World";
    String response;
    
    bool result = echoService.processEchoCommand(command, response);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Hello World", response.c_str());
    
    Serial.println("✓ ECHO指令处理测试通过");
}

// 测试5: 消息发送和接收
void test_message_send_receive() {
    Serial.println("测试消息发送和接收...");
    
    // 测试发送消息
    String testMessage = "Test Message";
    bool sendResult = rs485Interface.sendMessage(testMessage);
    TEST_ASSERT_TRUE(sendResult);
    
    RS485Stats stats = rs485Interface.getStats();
    TEST_ASSERT_EQUAL(1, stats.messagesSent);
    
    // 模拟接收消息
    rs485Interface.simulateReceive("Received Message");
    TEST_ASSERT_TRUE(rs485Interface.hasMessage());
    
    String receivedMessage = rs485Interface.receiveMessage();
    TEST_ASSERT_EQUAL_STRING("Received Message", receivedMessage.c_str());
    
    stats = rs485Interface.getStats();
    TEST_ASSERT_EQUAL(1, stats.messagesReceived);
    
    Serial.println("✓ 消息发送和接收测试通过");
}

// 测试6: 端到端回显测试
void test_end_to_end_echo() {
    Serial.println("测试端到端回显功能...");
    
    // 模拟接收ECHO指令
    rs485Interface.simulateReceive("ECHO Test Echo");
    TEST_ASSERT_TRUE(rs485Interface.hasMessage());
    
    // 接收并处理消息
    String receivedCommand = rs485Interface.receiveMessage();
    TEST_ASSERT_EQUAL_STRING("ECHO Test Echo", receivedCommand.c_str());
    
    // 处理回显指令
    String response;
    bool result = echoService.processEchoCommand(receivedCommand, response);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Test Echo", response.c_str());
    
    // 发送回复
    bool sendResult = rs485Interface.sendMessage(response);
    TEST_ASSERT_TRUE(sendResult);
    
    // 验证统计信息
    RS485Stats stats = rs485Interface.getStats();
    TEST_ASSERT_EQUAL(1, stats.messagesReceived);
    TEST_ASSERT_EQUAL(1, stats.messagesSent);
    
    Serial.println("✓ 端到端回显功能测试通过");
}

// 测试7: 缓冲区健康状态
void test_buffer_health() {
    Serial.println("测试缓冲区健康状态...");
    
    TEST_ASSERT_TRUE(rs485Interface.areBuffersHealthy());
    TEST_ASSERT_EQUAL(0, rs485Interface.getReceiveBufferSize());
    TEST_ASSERT_EQUAL(0, rs485Interface.getTransmitBufferSize());
    
    // 添加一些数据
    rs485Interface.sendMessage("Test");
    TEST_ASSERT_TRUE(rs485Interface.getTransmitBufferSize() > 0);
    
    Serial.println("✓ 缓冲区健康状态测试通过");
}

// 测试8: 启用/禁用功能
void test_enable_disable_functionality() {
    Serial.println("测试启用/禁用功能...");
    
    // 测试启用状态
    echoService.setEnabled(true);
    TEST_ASSERT_TRUE(echoService.isEnabled());
    
    String input = "Test message";
    String output = echoService.processEcho(input);
    TEST_ASSERT_EQUAL_STRING("Test message", output.c_str());
    
    // 测试禁用状态
    echoService.setEnabled(false);
    TEST_ASSERT_FALSE(echoService.isEnabled());
    
    output = echoService.processEcho(input);
    TEST_ASSERT_EQUAL_STRING("", output.c_str());
    
    Serial.println("✓ 启用/禁用功能测试通过");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("开始RS485控制功能检查点验证...");
    Serial.println("========================================");
    
    UNITY_BEGIN();
    
    // 运行所有测试
    RUN_TEST(test_rs485_interface_initialization);
    RUN_TEST(test_basic_echo_functionality);
    RUN_TEST(test_newline_preservation);
    RUN_TEST(test_echo_command_processing);
    RUN_TEST(test_message_send_receive);
    RUN_TEST(test_end_to_end_echo);
    RUN_TEST(test_buffer_health);
    RUN_TEST(test_enable_disable_functionality);
    
    UNITY_END();
    
    Serial.println("========================================");
    Serial.println("RS485控制功能检查点验证完成！");
}

void loop() {
    // 空循环
}