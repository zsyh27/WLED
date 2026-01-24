// 简化的RS485 usermod测试版本
// 用于验证usermod是否正确加载和初始化

#include "wled.h"

class TestRS485Usermod : public Usermod {
private:
    bool initDone = false;
    unsigned long lastLoop = 0;
    unsigned long setupTime = 0;

public:
    TestRS485Usermod() {
        Serial.println("TestRS485Usermod: Constructor called");
    }
    
    void setup() override {
        Serial.println("TestRS485Usermod: setup() called");
        setupTime = millis();
        
        // 初始化UART2进行简单测试
        Serial2.begin(9600, SERIAL_8N1, 26, 27);
        Serial.println("TestRS485Usermod: UART2 initialized on GPIO26/27 @ 9600 baud");
        
        initDone = true;
        Serial.println("TestRS485Usermod: setup() completed");
    }
    
    void loop() override {
        if (!initDone) return;
        
        unsigned long now = millis();
        if (now - lastLoop < 100) return; // 限制循环频率
        lastLoop = now;
        
        // 每10秒输出一次状态
        static unsigned long lastStatus = 0;
        if (now - lastStatus > 10000) {
            Serial.printf("TestRS485Usermod: Loop running, uptime: %lu ms\n", now - setupTime);
            lastStatus = now;
        }
        
        // 检查UART2接收
        if (Serial2.available()) {
            String message = "";
            while (Serial2.available()) {
                char c = Serial2.read();
                if (c == '\n' || c == '\r') {
                    break;
                } else if (c >= 32 && c <= 126) {
                    message += c;
                }
                delay(1);
            }
            
            if (message.length() > 0) {
                Serial.printf("TestRS485Usermod: Received: '%s'\n", message.c_str());
                
                // 简单回显
                String response = "ECHO: " + message;
                Serial2.println(response);
                Serial.printf("TestRS485Usermod: Sent: '%s'\n", response.c_str());
            }
        }
    }
    
    void connected() override {
        Serial.println("TestRS485Usermod: connected() called");
    }
    
    uint16_t getId() override {
        return 59; // 使用相同的ID
    }
};

// 创建实例并注册
static TestRS485Usermod test_rs485_usermod;
REGISTER_USERMOD(test_rs485_usermod);