#include "wled.h"

// RS485 Control Usermod - Simplified Implementation
// This is a simplified version that avoids complex class dependencies

#define USERMOD_ID_RS485_CONTROL 59

// Configuration structure
struct RS485Config {
    bool enabled = true;
    uint32_t baudRate = 9600;
    int rxPin = 26;
    int txPin = 27;
    size_t bufferSize = 512;
    uint32_t timeout = 1000;
    bool echoEnabled = true;
    bool debugMode = false;
    uint8_t deviceAddress = 1;        // 设备地址 (1-255)
    bool respondToBroadcast = false;  // 是否响应广播命令
};

// Statistics structure
struct RS485Stats {
    uint32_t messagesReceived = 0;
    uint32_t messagesSent = 0;
    uint32_t parseErrors = 0;
    uint32_t bufferOverflows = 0;
    uint32_t timeouts = 0;
    uint32_t lastActivity = 0;
    bool connected = false;
};

class RS485ControlUsermod : public Usermod {
private:
    // Configuration and state
    RS485Config config;
    RS485Stats stats;
    bool initDone = false;
    unsigned long lastLoop = 0;
    HardwareSerial* serial = nullptr;
    int currentPreset = 1;
    
    // String constants for configuration
    static const char _name[];
    static const char _enabled[];
    static const char _baudRate[];
    static const char _rxPin[];
    static const char _txPin[];
    static const char _bufferSize[];
    static const char _timeout[];
    static const char _echoEnabled[];
    static const char _debugMode[];
    static const char _deviceAddress[];
    static const char _respondToBroadcast[];

    // Helper function to apply preset
    void applyWLEDPreset(int presetId) {
        if (presetId >= 1 && presetId <= 250) {  // WLED支持1-250个预设
            currentPreset = presetId;
            // Apply the preset using WLED's preset system
            applyPreset(presetId, CALL_MODE_BUTTON_PRESET);
        }
    }

public:
    RS485ControlUsermod() {
        initDone = false;
        lastLoop = 0;
        serial = nullptr;
        
        // Initialize config with defaults
        config.enabled = true;
        config.baudRate = 9600;
        config.rxPin = 26;
        config.txPin = 27;
        config.bufferSize = 512;
        config.timeout = 1000;
        config.echoEnabled = true;
        config.debugMode = false;
        config.deviceAddress = 1;
        config.respondToBroadcast = false;
        
        // Initialize stats
        stats.messagesReceived = 0;
        stats.messagesSent = 0;
        stats.parseErrors = 0;
        stats.bufferOverflows = 0;
        stats.timeouts = 0;
        stats.lastActivity = 0;
        stats.connected = false;
    }

    ~RS485ControlUsermod() {
        if (serial) {
            serial->end();
        }
    }

    void setup() override {
        if (initDone) return;
        
        DEBUG_PRINTLN(F("RS485 Control: Starting setup..."));
        
        if (config.enabled) {
            // Initialize serial communication
            serial = &Serial2;
            serial->begin(config.baudRate, SERIAL_8N1, config.rxPin, config.txPin);
            
            if (config.debugMode) {
                DEBUG_PRINTF("RS485 Control: Serial initialized - RX:%d, TX:%d, Baud:%d\n", 
                           config.rxPin, config.txPin, config.baudRate);
            }
        }
        
        initDone = true;
        DEBUG_PRINTLN(F("RS485 Control: Setup completed"));
    }

    void loop() override {
        if (!initDone || !config.enabled || !serial) return;
        
        unsigned long now = millis();
        if (now - lastLoop < 10) return; // Limit loop frequency
        lastLoop = now;
        
        // Check for incoming messages
        if (serial->available()) {
            String message = serial->readStringUntil('\n');
            message.trim();
            
            if (message.length() > 0) {
                stats.messagesReceived++;
                stats.lastActivity = now;
                
                if (config.debugMode) {
                    DEBUG_PRINTF("RS485 RX: %s\n", message.c_str());
                }
                
                // 临时禁用回环检测，先确保基本功能正常
                // TODO: 后续需要重新实现正确的回环检测
                
                // Process command
                String response = processCommand(message);
                
                if (response.length() > 0) {
                    serial->println(response);
                    stats.messagesSent++;
                    
                    if (config.debugMode) {
                        DEBUG_PRINTF("RS485 TX: %s\n", response.c_str());
                    }
                }
            }
        }
    }

    void connected() override {
        if (!initDone) return;
        stats.connected = true;
        DEBUG_PRINTLN(F("RS485 Control: Network connected"));
    }

    void addToConfig(JsonObject& root) override {
        JsonObject top = root.createNestedObject(FPSTR(_name));
        top[FPSTR(_enabled)] = config.enabled;
        top[FPSTR(_baudRate)] = config.baudRate;
        top[FPSTR(_rxPin)] = config.rxPin;
        top[FPSTR(_txPin)] = config.txPin;
        top[FPSTR(_bufferSize)] = config.bufferSize;
        top[FPSTR(_timeout)] = config.timeout;
        top[FPSTR(_echoEnabled)] = config.echoEnabled;
        top[FPSTR(_debugMode)] = config.debugMode;
        top[FPSTR(_deviceAddress)] = config.deviceAddress;
        top[FPSTR(_respondToBroadcast)] = config.respondToBroadcast;
    }

    bool readFromConfig(JsonObject& root) override {
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
        
        if (top[FPSTR(_deviceAddress)] != config.deviceAddress) {
            config.deviceAddress = top[FPSTR(_deviceAddress)] | config.deviceAddress;
            configChanged = true;
        }
        
        if (top[FPSTR(_respondToBroadcast)] != config.respondToBroadcast) {
            config.respondToBroadcast = top[FPSTR(_respondToBroadcast)] | config.respondToBroadcast;
            configChanged = true;
        }
        
        if (configChanged) {
            // Reinitialize if needed
            if (serial && config.enabled) {
                serial->end();
                serial->begin(config.baudRate, SERIAL_8N1, config.rxPin, config.txPin);
            }
        }
        
        return configChanged;
    }

    void appendConfigData() override {
        oappend(SET_F("addInfo('RS485Control:enabled',1,'Enable RS485 serial control');"));
        oappend(SET_F("addInfo('RS485Control:baudRate',1,'Baud rate (9600, 19200, 38400, 57600, 115200)');"));
        oappend(SET_F("addInfo('RS485Control:rxPin',1,'RX pin (GPIO number for receiving data)');"));
        oappend(SET_F("addInfo('RS485Control:txPin',1,'TX pin (GPIO number for transmitting data)');"));
        oappend(SET_F("addInfo('RS485Control:bufferSize',1,'Buffer size in bytes (256-2048)');"));
        oappend(SET_F("addInfo('RS485Control:timeout',1,'Timeout in milliseconds (500-5000)');"));
        oappend(SET_F("addInfo('RS485Control:echoEnabled',1,'Enable echo service for testing RS485 communication');"));
        oappend(SET_F("addInfo('RS485Control:debugMode',1,'Enable debug output to serial monitor');"));
        oappend(SET_F("addInfo('RS485Control:deviceAddress',1,'Device address (1-255, unique for each device)');"));
        oappend(SET_F("addInfo('RS485Control:respondToBroadcast',1,'Respond to broadcast commands (@0)');"));
    }

    void addToJsonInfo(JsonObject& root) override {
        JsonObject user = root["u"];
        if (user.isNull()) user = root.createNestedObject("u");
        
        JsonArray infoArr = user.createNestedArray(FPSTR(_name));
        
        String uiDomString = F("<button class=\"btn btn-xs\" onclick=\"requestJson({");
        uiDomString += FPSTR(_name);
        uiDomString += F(":{");
        uiDomString += FPSTR(_enabled);
        uiDomString += config.enabled ? F(":false}})\">Disable") : F(":true}})\">Enable");
        uiDomString += F("</button>");
        infoArr.add(uiDomString);
        
        if (config.enabled) {
            infoArr.add(F("RS485 Control Active"));
            infoArr.add(String(F("Device Address: ")) + String(config.deviceAddress));
            infoArr.add(String(F("Messages RX: ")) + String(stats.messagesReceived));
            infoArr.add(String(F("Messages TX: ")) + String(stats.messagesSent));
            infoArr.add(String(F("Parse Errors: ")) + String(stats.parseErrors));
            infoArr.add(String(F("Baud Rate: ")) + String(config.baudRate));
            infoArr.add(String(F("RX Pin: GPIO")) + String(config.rxPin));
            infoArr.add(String(F("TX Pin: GPIO")) + String(config.txPin));
            
            if (config.echoEnabled) {
                infoArr.add(F("Echo Service: Enabled"));
            }
            
            if (config.debugMode) {
                infoArr.add(F("Debug Mode: Active"));
            }
            
            if (config.respondToBroadcast) {
                infoArr.add(F("Broadcast Response: Enabled"));
            }
        } else {
            infoArr.add(F("RS485 Control Disabled"));
        }
    }

    void addToJsonState(JsonObject& root) override {
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

    void readFromJsonState(JsonObject& root) override {
        if (!initDone) return;
        
        JsonObject usermod = root[FPSTR(_name)];
        if (!usermod.isNull()) {
            if (usermod.containsKey("enabled")) {
                bool newEnabled = usermod["enabled"] | config.enabled;
                if (newEnabled != config.enabled) {
                    config.enabled = newEnabled;
                    if (serial) {
                        if (config.enabled) {
                            serial->begin(config.baudRate, SERIAL_8N1, config.rxPin, config.txPin);
                        } else {
                            serial->end();
                        }
                    }
                }
            }
        }
    }

    void onStateChange(uint8_t mode) override {
        if (!initDone || !config.enabled) return;
        
        if (config.debugMode) {
            DEBUG_PRINTF("RS485 Control: WLED state change - mode: %d\n", mode);
        }
    }

    uint16_t getId() override { 
        return USERMOD_ID_RS485_CONTROL; 
    }

private:
    String processCommand(const String& input) {
        String cmd = input;
        cmd.trim();
        
        if (config.debugMode) {
            DEBUG_PRINTF("RS485 Processing: %s\n", input.c_str());
        }
        
        // 简化处理：只检查是否是发给当前设备的命令
        if (cmd.startsWith("@")) {
            int spacePos = cmd.indexOf(' ');
            if (spacePos > 1) {
                String addrStr = cmd.substring(1, spacePos);
                uint8_t targetAddress = addrStr.toInt();
                
                if (config.debugMode) {
                    DEBUG_PRINTF("Target address: %d, My address: %d\n", targetAddress, config.deviceAddress);
                }
                
                // 处理发给当前设备的命令或广播命令
                if (targetAddress == config.deviceAddress || targetAddress == 0) {
                    String command = cmd.substring(spacePos + 1);
                    command.trim();
                    
                    if (config.debugMode) {
                        if (targetAddress == 0) {
                            DEBUG_PRINTF("Broadcast command: %s\n", command.c_str());
                        } else {
                            DEBUG_PRINTF("Command for me: %s\n", command.c_str());
                        }
                    }
                    
                    // 执行命令
                    String response = executeCommand(command);
                    
                    if (response.length() > 0) {
                        // 广播命令不回复，只有单播命令才回复
                        if (targetAddress != 0) {
                            return "@" + String(config.deviceAddress) + " " + response;
                        }
                    }
                }
            }
        }
        
        return "";  // 不回复
    }
    
    String executeCommand(const String& cmd) {
        String command = cmd;
        command.toUpperCase();
        command.trim();
        
        // 只处理PRESET命令，其他命令一概忽略
        if (command.startsWith("PRESET ")) {
            String presetStr = command.substring(7);
            int presetId = presetStr.toInt();
            
            if (presetId >= 1 && presetId <= 250) {
                // Apply preset
                applyWLEDPreset(presetId);
                return "OK: Preset " + String(presetId) + " activated";
            } else {
                return "ERROR: Invalid preset ID (1-250)";
            }
        }
        
        // 其他所有命令都不处理，不回复
        return "";
    }
};

// String constants definition
const char RS485ControlUsermod::_name[]        PROGMEM = "RS485Control";
const char RS485ControlUsermod::_enabled[]     PROGMEM = "enabled";
const char RS485ControlUsermod::_baudRate[]    PROGMEM = "baudRate";
const char RS485ControlUsermod::_rxPin[]       PROGMEM = "rxPin";
const char RS485ControlUsermod::_txPin[]       PROGMEM = "txPin";
const char RS485ControlUsermod::_bufferSize[]  PROGMEM = "bufferSize";
const char RS485ControlUsermod::_timeout[]     PROGMEM = "timeout";
const char RS485ControlUsermod::_echoEnabled[] PROGMEM = "echoEnabled";
const char RS485ControlUsermod::_debugMode[]   PROGMEM = "debugMode";
const char RS485ControlUsermod::_deviceAddress[] PROGMEM = "deviceAddress";
const char RS485ControlUsermod::_respondToBroadcast[] PROGMEM = "respondToBroadcast";

// Create static instance and register the usermod
static RS485ControlUsermod rs485_control_usermod;
REGISTER_USERMOD(rs485_control_usermod);