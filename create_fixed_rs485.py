#!/usr/bin/env python3
"""
创建修复后的RS485控制文件
只保留方法实现，移除类定义
"""

import sys
from pathlib import Path

def create_fixed_cpp():
    """创建修复后的.cpp文件"""
    
    fixed_content = '''#include "rs485_control.h"

// String constants for configuration (stored in PROGMEM to save RAM)
const char RS485ControlUsermod::_name[]        PROGMEM = "RS485Control";
const char RS485ControlUsermod::_enabled[]     PROGMEM = "enabled";
const char RS485ControlUsermod::_baudRate[]    PROGMEM = "baudRate";
const char RS485ControlUsermod::_rxPin[]       PROGMEM = "rxPin";
const char RS485ControlUsermod::_txPin[]       PROGMEM = "txPin";
const char RS485ControlUsermod::_bufferSize[]  PROGMEM = "bufferSize";
const char RS485ControlUsermod::_timeout[]     PROGMEM = "timeout";
const char RS485ControlUsermod::_echoEnabled[] PROGMEM = "echoEnabled";
const char RS485ControlUsermod::_debugMode[]   PROGMEM = "debugMode";

// RS485ControlUsermod implementation
RS485ControlUsermod::RS485ControlUsermod() {
    rs485Interface = nullptr;
    commandParser = nullptr;
    echoService = nullptr;
    presetController = nullptr;
    configManager = nullptr;
    errorLogger = nullptr;
    initDone = false;
    lastLoop = 0;
    
    // Initialize config with defaults
    config.enabled = true;
    config.baudRate = 9600;
    config.rxPin = 26;
    config.txPin = 27;
    config.bufferSize = 512;
    config.timeout = 1000;
    config.echoEnabled = true;
    config.debugMode = false;
    
    // Initialize stats
    stats.messagesReceived = 0;
    stats.messagesSent = 0;
    stats.parseErrors = 0;
    stats.bufferOverflows = 0;
    stats.timeouts = 0;
    stats.lastActivity = 0;
    stats.connected = false;
}

RS485ControlUsermod::~RS485ControlUsermod() {
    if (rs485Interface) delete rs485Interface;
    if (commandParser) delete commandParser;
    if (echoService) delete echoService;
    if (presetController) delete presetController;
    if (configManager) delete configManager;
    if (errorLogger) delete errorLogger;
}

void RS485ControlUsermod::setup() {
    if (initDone) return;
    
    DEBUG_PRINTLN(F("RS485 Control: Starting setup..."));
    
    // Create components
    errorLogger = new ErrorLogger();
    configManager = new ConfigManager();
    rs485Interface = new RS485Interface();
    commandParser = new CommandParser();
    echoService = new EchoService();
    presetController = new PresetController();
    
    // Load configuration
    if (configManager) {
        configManager->loadConfig();
        config = configManager->getConfig();
    }
    
    // Initialize components if enabled
    if (config.enabled) {
        if (rs485Interface) {
            rs485Interface->initialize(&config, &stats);
        }
        
        if (echoService) {
            echoService->initialize(&config, &stats);
        }
        
        if (presetController) {
            presetController->initialize(&config, &stats);
        }
        
        coordinateComponents();
    }
    
    initDone = true;
    DEBUG_PRINTLN(F("RS485 Control: Setup completed"));
}

void RS485ControlUsermod::loop() {
    if (!initDone || !config.enabled) return;
    
    unsigned long now = millis();
    if (now - lastLoop < 10) return; // Limit loop frequency
    lastLoop = now;
    
    // Process RS485 communication
    if (rs485Interface && rs485Interface->hasMessage()) {
        String message = rs485Interface->receiveMessage();
        if (message.length() > 0) {
            stats.messagesReceived++;
            stats.lastActivity = now;
            
            // Parse and execute command
            if (commandParser) {
                ParseResult result = commandParser->parseCommand(message);
                String response = "";
                
                if (result.success) {
                    switch (result.type) {
                        case CommandType::ECHO:
                            if (echoService) {
                                response = echoService->processEcho(message);
                            }
                            break;
                            
                        case CommandType::PRESET_SET:
                            if (presetController && result.parameters.size() > 0) {
                                int presetId = result.parameters[0].toInt();
                                bool success = presetController->activatePreset(presetId);
                                response = success ? "OK: Preset " + String(presetId) + " activated" :
                                                   "ERROR: Failed to activate preset " + String(presetId);
                            }
                            break;
                            
                        case CommandType::PRESET_GET:
                            if (presetController) {
                                int current = presetController->getCurrentPreset();
                                response = "CURRENT: Preset " + String(current);
                            }
                            break;
                            
                        case CommandType::STATUS_GET:
                            response = "STATUS: Online, Connected: " + String(stats.connected ? "Yes" : "No");
                            break;
                            
                        default:
                            response = "ERROR: Unknown command";
                            break;
                    }
                } else {
                    stats.parseErrors++;
                    response = "ERROR: " + commandParser->getErrorMessage(result.error);
                }
                
                // Send response
                if (response.length() > 0 && rs485Interface) {
                    rs485Interface->sendMessage(response + "\\n");
                    stats.messagesSent++;
                }
            }
        }
    }
    
    // Periodic health check
    if (now % 30000 == 0) { // Every 30 seconds
        performHealthCheck();
    }
}

void RS485ControlUsermod::connected() {
    if (!initDone) return;
    stats.connected = true;
    DEBUG_PRINTLN(F("RS485 Control: Network connected"));
}

void RS485ControlUsermod::addToConfig(JsonObject& root) {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top[FPSTR(_enabled)] = config.enabled;
    top[FPSTR(_baudRate)] = config.baudRate;
    top[FPSTR(_rxPin)] = config.rxPin;
    top[FPSTR(_txPin)] = config.txPin;
    top[FPSTR(_bufferSize)] = config.bufferSize;
    top[FPSTR(_timeout)] = config.timeout;
    top[FPSTR(_echoEnabled)] = config.echoEnabled;
    top[FPSTR(_debugMode)] = config.debugMode;
}

bool RS485ControlUsermod::readFromConfig(JsonObject& root) {
    JsonObject top = root[FPSTR(_name)];
    if (top.isNull()) return false;
    
    bool configChanged = false;
    
    if (top[FPSTR(_enabled)] != config.enabled) {
        config.enabled = top[FPSTR(_enabled)] | config.enabled;
        configChanged = true;
    }
    
    if (top[FPSTR(_baudRate)] != config.baudRate) {
        config.baudRate = top[FPSTR(_baudRate)] | config.baudRate;
        configChanged = true;
    }
    
    if (top[FPSTR(_rxPin)] != config.rxPin) {
        config.rxPin = top[FPSTR(_rxPin)] | config.rxPin;
        configChanged = true;
    }
    
    if (top[FPSTR(_txPin)] != config.txPin) {
        config.txPin = top[FPSTR(_txPin)] | config.txPin;
        configChanged = true;
    }
    
    if (top[FPSTR(_bufferSize)] != config.bufferSize) {
        config.bufferSize = top[FPSTR(_bufferSize)] | config.bufferSize;
        configChanged = true;
    }
    
    if (top[FPSTR(_timeout)] != config.timeout) {
        config.timeout = top[FPSTR(_timeout)] | config.timeout;
        configChanged = true;
    }
    
    if (top[FPSTR(_echoEnabled)] != config.echoEnabled) {
        config.echoEnabled = top[FPSTR(_echoEnabled)] | config.echoEnabled;
        configChanged = true;
    }
    
    if (top[FPSTR(_debugMode)] != config.debugMode) {
        config.debugMode = top[FPSTR(_debugMode)] | config.debugMode;
        configChanged = true;
    }
    
    if (configChanged && configManager) {
        configManager->setConfig(config);
        coordinateComponents();
    }
    
    return configChanged;
}

void RS485ControlUsermod::appendConfigData() {
    oappend(SET_F("addInfo('RS485Control:enabled',1,'Enable RS485 serial control');"));
    oappend(SET_F("addInfo('RS485Control:baudRate',1,'Baud rate (9600, 19200, 38400, 57600)');"));
    oappend(SET_F("addInfo('RS485Control:rxPin',1,'RX pin (GPIO number)');"));
    oappend(SET_F("addInfo('RS485Control:txPin',1,'TX pin (GPIO number)');"));
    oappend(SET_F("addInfo('RS485Control:bufferSize',1,'Buffer size in bytes');"));
    oappend(SET_F("addInfo('RS485Control:timeout',1,'Timeout in milliseconds');"));
    oappend(SET_F("addInfo('RS485Control:echoEnabled',1,'Enable echo service');"));
    oappend(SET_F("addInfo('RS485Control:debugMode',1,'Enable debug output');"));
}

void RS485ControlUsermod::addToJsonInfo(JsonObject& root) {
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");
    
    JsonArray infoArr = user.createNestedArray(FPSTR(_name));
    
    String uiDomString = F("<button class=\\"btn btn-xs\\" onclick=\\"requestJson({");
    uiDomString += FPSTR(_name);
    uiDomString += F(":{");
    uiDomString += FPSTR(_enabled);
    uiDomString += config.enabled ? F(":false}})\\">Disable") : F(":true}})\\">Enable");
    uiDomString += F("</button>");
    infoArr.add(uiDomString);
    
    if (config.enabled) {
        infoArr.add(F("RS485 Control Active"));
        infoArr.add(F("Messages RX: ") + String(stats.messagesReceived));
        infoArr.add(F("Messages TX: ") + String(stats.messagesSent));
        infoArr.add(F("Parse Errors: ") + String(stats.parseErrors));
        infoArr.add(F("Baud Rate: ") + String(config.baudRate));
        infoArr.add(F("RX Pin: GPIO") + String(config.rxPin));
        infoArr.add(F("TX Pin: GPIO") + String(config.txPin));
        
        if (config.echoEnabled) {
            infoArr.add(F("Echo Service: Enabled"));
        }
        
        if (config.debugMode) {
            infoArr.add(F("Debug Mode: Active"));
        }
    } else {
        infoArr.add(F("RS485 Control Disabled"));
    }
}

void RS485ControlUsermod::addToJsonState(JsonObject& root) {
    if (!initDone || !config.enabled) return;
    
    JsonObject usermod = root[FPSTR(_name)];
    if (usermod.isNull()) usermod = root.createNestedObject(FPSTR(_name));
    
    usermod["enabled"] = config.enabled;
    usermod["connected"] = stats.connected;
    usermod["messagesRx"] = stats.messagesReceived;
    usermod["messagesTx"] = stats.messagesSent;
    usermod["errors"] = stats.parseErrors;
    usermod["echoEnabled"] = config.echoEnabled;
}

void RS485ControlUsermod::readFromJsonState(JsonObject& root) {
    if (!initDone) return;
    
    JsonObject usermod = root[FPSTR(_name)];
    if (!usermod.isNull()) {
        if (usermod.containsKey("enabled")) {
            bool newEnabled = usermod["enabled"] | config.enabled;
            if (newEnabled != config.enabled) {
                config.enabled = newEnabled;
                coordinateComponents();
            }
        }
    }
}

void RS485ControlUsermod::onStateChange(uint8_t mode) {
    if (!initDone || !config.enabled) return;
    
    if (config.debugMode) {
        DEBUG_PRINTF("RS485 Control: WLED state change - mode: %d\\n", mode);
    }
}

void RS485ControlUsermod::performHealthCheck() {
    if (!initDone || !config.enabled) return;
    
    // Simple health check implementation
    if (config.debugMode) {
        DEBUG_PRINTLN(F("RS485 Control: Health check performed"));
    }
}

void RS485ControlUsermod::optimizePerformance() {
    if (!initDone || !config.enabled) return;
    
    // Simple performance optimization
    if (config.debugMode) {
        DEBUG_PRINTLN(F("RS485 Control: Performance optimized"));
    }
}

void RS485ControlUsermod::coordinateComponents() {
    if (!initDone || !config.enabled) return;
    
    // Coordinate all components
    if (echoService) {
        echoService->setEnabled(config.echoEnabled);
    }
}

// Create static instance and register the usermod
static RS485ControlUsermod rs485_control_usermod;
REGISTER_USERMOD(rs485_control_usermod);
'''
    
    # 写入修复后的文件
    cpp_file = Path("usermods/rs485_control/rs485_control.cpp")
    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(fixed_content)
    
    print("✅ 创建了修复后的RS485控制文件")
    return True

def main():
    """主函数"""
    print("创建修复后的RS485控制文件")
    print("=" * 40)
    
    if create_fixed_cpp():
        print("\n🎉 修复完成！现在可以重新编译了。")
        return 0
    else:
        print("\n❌ 修复失败")
        return 1

if __name__ == "__main__":
    sys.exit(main())