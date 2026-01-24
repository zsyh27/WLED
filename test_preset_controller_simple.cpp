/*
 * 简单的PresetController功能验证脚本
 * 这个文件用于验证PresetController类的基本功能
 */

#include <Arduino.h>
#include <iostream>
#include <string>

// 模拟WLED环境
byte currentPreset = 0;

// 模拟预设存在状态（用于测试）
bool mockPresetExists[251] = {false};
std::string mockPresetNames[251];

// 模拟WLED函数
bool applyPreset(byte index, byte callMode = 0) {
    if (index >= 1 && index <= 250 && mockPresetExists[index]) {
        currentPreset = index;
        return true;
    }
    return false;
}

bool getPresetName(byte index, String& name) {
    if (index >= 1 && index <= 250 && mockPresetExists[index]) {
        name = String(mockPresetNames[index].c_str());
        return true;
    }
    name = "";
    return false;
}

// 简化的String类（用于测试）
class String {
public:
    std::string data;
    
    String() {}
    String(const char* str) : data(str) {}
    String(const std::string& str) : data(str) {}
    String(int num) : data(std::to_string(num)) {}
    
    const char* c_str() const { return data.c_str(); }
    size_t length() const { return data.length(); }
    
    String operator+(const String& other) const {
        return String(data + other.data);
    }
    
    bool startsWith(const String& prefix) const {
        return data.substr(0, prefix.length()) == prefix.data;
    }
    
    int indexOf(const String& substr) const {
        size_t pos = data.find(substr.data);
        return (pos != std::string::npos) ? (int)pos : -1;
    }
    
    String toLowerCase() const {
        std::string lower = data;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return String(lower);
    }
};

// 简化的统计结构
struct RS485Stats {
    uint32_t messagesReceived = 0;
    uint32_t messagesSent = 0;
    uint32_t parseErrors = 0;
    uint32_t bufferOverflows = 0;
    uint32_t timeouts = 0;
    uint32_t lastActivity = 0;
    bool connected = false;
};

// 模拟millis()函数
unsigned long millis() {
    return 1000; // 固定值用于测试
}

// PresetController类的简化版本（用于验证）
class PresetController {
private:
    RS485Stats* stats;
    bool debugMode;
    unsigned long lastActivationTime;
    int lastActivatedPreset;
    
    bool validatePresetRange(int presetId) const {
        return presetId >= 1 && presetId <= 250;
    }
    
    void logPresetActivity(int presetId, const String& action, bool success) {
        if (debugMode) {
            std::cout << "PresetController: " << action.c_str() << " preset " << presetId 
                     << " - " << (success ? "SUCCESS" : "FAILED") << std::endl;
        }
        
        if (stats) {
            if (success) {
                stats->lastActivity = millis();
            } else {
                stats->parseErrors++;
            }
        }
    }

public:
    PresetController() : stats(nullptr), debugMode(false), lastActivationTime(0), lastActivatedPreset(0) {}
    
    void initialize(RS485Stats* st, bool debug = false) {
        stats = st;
        debugMode = debug;
        lastActivationTime = 0;
        lastActivatedPreset = 0;
        
        if (debugMode) {
            std::cout << "PresetController: Initialized" << std::endl;
        }
    }
    
    bool activatePreset(int presetId) {
        if (!validatePresetRange(presetId)) {
            logPresetActivity(presetId, "activate", false);
            if (debugMode) {
                std::cout << "PresetController: Invalid preset ID " << presetId 
                         << " (valid range: 1-250)" << std::endl;
            }
            return false;
        }
        
        String presetName;
        bool presetExists = getPresetName(presetId, presetName);
        
        if (!presetExists) {
            logPresetActivity(presetId, "activate", false);
            if (debugMode) {
                std::cout << "PresetController: Preset " << presetId << " does not exist" << std::endl;
            }
            return false;
        }
        
        bool success = applyPreset(presetId, 0);
        
        if (success) {
            lastActivationTime = millis();
            lastActivatedPreset = presetId;
            logPresetActivity(presetId, "activate", true);
            
            if (debugMode) {
                std::cout << "PresetController: Successfully activated preset " << presetId 
                         << " (" << presetName.c_str() << ")" << std::endl;
            }
        } else {
            logPresetActivity(presetId, "activate", false);
            if (debugMode) {
                std::cout << "PresetController: Failed to activate preset " << presetId << std::endl;
            }
        }
        
        return success;
    }
    
    int getCurrentPreset() {
        return currentPreset;
    }
    
    String getPresetName(int presetId) {
        if (!validatePresetRange(presetId)) {
            return String("");
        }
        
        String name;
        bool success = ::getPresetName(presetId, name);
        
        if (success && name.length() > 0) {
            return name;
        } else {
            return String("");
        }
    }
    
    bool getPresetName(int presetId, String& name) {
        if (!validatePresetRange(presetId)) {
            name = String("");
            return false;
        }
        
        bool success = ::getPresetName(presetId, name);
        return success && name.length() > 0;
    }
    
    bool isValidPreset(int presetId) {
        if (!validatePresetRange(presetId)) {
            return false;
        }
        
        String name;
        return getPresetName(presetId, name);
    }
    
    String formatPresetResponse(int presetId, bool success, const String& operation) {
        String response;
        
        if (success) {
            String name;
            if (getPresetName(presetId, name)) {
                response = String("OK: ") + operation + String(" preset ") + String(presetId);
                if (name.length() > 0) {
                    response = response + String(" (") + name + String(")");
                }
            } else {
                response = String("OK: ") + operation + String(" preset ") + String(presetId);
            }
        } else {
            if (!validatePresetRange(presetId)) {
                response = String("ERROR: Invalid preset ID ") + String(presetId) + String(" (valid range: 1-250)");
            } else if (!isValidPreset(presetId)) {
                response = String("ERROR: Preset ") + String(presetId) + String(" does not exist");
            } else {
                response = String("ERROR: Failed to ") + operation.toLowerCase() + String(" preset ") + String(presetId);
            }
        }
        
        return response;
    }
    
    void setDebugMode(bool debug) {
        debugMode = debug;
        if (debugMode) {
            std::cout << "PresetController: Debug mode enabled" << std::endl;
        }
    }
};

// 测试函数
void runTests() {
    std::cout << "=== PresetController 功能验证测试 ===" << std::endl;
    
    // 设置测试数据
    mockPresetExists[1] = true;
    mockPresetNames[1] = "测试预设1";
    mockPresetExists[5] = true;
    mockPresetNames[5] = "彩虹效果";
    mockPresetExists[10] = true;
    mockPresetNames[10] = "纯色";
    
    // 创建控制器
    RS485Stats stats;
    PresetController controller;
    controller.initialize(&stats, true);
    
    std::cout << "\n--- 测试1: 激活有效预设 ---" << std::endl;
    bool result = controller.activatePreset(1);
    std::cout << "激活预设1结果: " << (result ? "成功" : "失败") << std::endl;
    std::cout << "当前预设: " << controller.getCurrentPreset() << std::endl;
    
    std::cout << "\n--- 测试2: 激活不存在的预设 ---" << std::endl;
    result = controller.activatePreset(99);
    std::cout << "激活预设99结果: " << (result ? "成功" : "失败") << std::endl;
    
    std::cout << "\n--- 测试3: 无效预设ID ---" << std::endl;
    result = controller.activatePreset(0);
    std::cout << "激活预设0结果: " << (result ? "成功" : "失败") << std::endl;
    result = controller.activatePreset(251);
    std::cout << "激活预设251结果: " << (result ? "成功" : "失败") << std::endl;
    
    std::cout << "\n--- 测试4: 获取预设名称 ---" << std::endl;
    String name = controller.getPresetName(1);
    std::cout << "预设1名称: " << name.c_str() << std::endl;
    name = controller.getPresetName(99);
    std::cout << "预设99名称: " << (name.length() > 0 ? name.c_str() : "(空)") << std::endl;
    
    std::cout << "\n--- 测试5: 预设验证 ---" << std::endl;
    std::cout << "预设1有效: " << (controller.isValidPreset(1) ? "是" : "否") << std::endl;
    std::cout << "预设99有效: " << (controller.isValidPreset(99) ? "是" : "否") << std::endl;
    std::cout << "预设0有效: " << (controller.isValidPreset(0) ? "是" : "否") << std::endl;
    
    std::cout << "\n--- 测试6: 响应格式化 ---" << std::endl;
    String response = controller.formatPresetResponse(1, true, String("Activated"));
    std::cout << "成功响应: " << response.c_str() << std::endl;
    
    response = controller.formatPresetResponse(99, false, String("Activated"));
    std::cout << "失败响应: " << response.c_str() << std::endl;
    
    response = controller.formatPresetResponse(0, false, String("Activated"));
    std::cout << "无效ID响应: " << response.c_str() << std::endl;
    
    std::cout << "\n--- 测试7: 激活另一个预设 ---" << std::endl;
    result = controller.activatePreset(5);
    std::cout << "激活预设5结果: " << (result ? "成功" : "失败") << std::endl;
    std::cout << "当前预设: " << controller.getCurrentPreset() << std::endl;
    
    std::cout << "\n=== 所有测试完成 ===" << std::endl;
    std::cout << "解析错误计数: " << stats.parseErrors << std::endl;
}

int main() {
    runTests();
    return 0;
}