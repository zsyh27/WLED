#pragma once

#include "wled.h"

// RS485 Control Usermod ID
#define USERMOD_ID_RS485_CONTROL 59

// Configuration structure for RS485 interface
struct RS485Config {
    bool enabled = true;  // 默认启用
    uint32_t baudRate = 9600;
    int rxPin = 26;
    int txPin = 27;
    size_t bufferSize = 512;
    uint32_t timeout = 1000;
    bool echoEnabled = true;
    bool debugMode = false;
};

// Statistics structure for RS485 interface
struct RS485Stats {
    uint32_t messagesReceived = 0;
    uint32_t messagesSent = 0;
    uint32_t parseErrors = 0;
    uint32_t bufferOverflows = 0;
    uint32_t timeouts = 0;
    uint32_t lastActivity = 0;
    bool connected = false;
};

// Command types supported by the parser
enum class CommandType {
    ECHO,
    PRESET_SET,
    PRESET_GET,
    STATUS_GET,
    INVALID
};

// Parse error types
enum class ParseError {
    NONE,
    INVALID_FORMAT,
    UNKNOWN_COMMAND,
    INVALID_PARAMETERS,
    PARAMETER_OUT_OF_RANGE,
    BUFFER_OVERFLOW
};

// Error types for logging system
enum class RS485ErrorType {
    NONE,
    UART_INIT_FAILED,
    BUFFER_OVERFLOW,
    COMMUNICATION_TIMEOUT,
    PARSE_ERROR,
    HARDWARE_FAULT,
    MEMORY_ERROR,
    CONFIG_ERROR,
    PRESET_ERROR,
    UNKNOWN_ERROR
};

// Error severity levels
enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// Error log entry structure
struct ErrorLogEntry {
    uint32_t timestamp;
    RS485ErrorType errorType;
    ErrorSeverity severity;
    String message;
    String details;
    uint32_t errorCode;
    
    ErrorLogEntry() : timestamp(0), errorType(RS485ErrorType::NONE), 
                     severity(ErrorSeverity::INFO), errorCode(0) {}
    
    ErrorLogEntry(RS485ErrorType type, ErrorSeverity sev, const String& msg, 
                  const String& det = "", uint32_t code = 0) 
        : timestamp(millis()), errorType(type), severity(sev), 
          message(msg), details(det), errorCode(code) {}
};

// Diagnostic information structure
struct DiagnosticInfo {
    // Communication statistics
    uint32_t totalErrors;
    uint32_t errorsByType[10]; // Index corresponds to RS485ErrorType enum
    uint32_t lastErrorTime;
    RS485ErrorType lastErrorType;
    
    // Buffer health
    uint8_t rxBufferHealth;  // 0-100%
    uint8_t txBufferHealth;  // 0-100%
    uint32_t totalBufferOverflows;
    
    // Communication health
    bool uartHealthy;
    bool communicationActive;
    uint32_t consecutiveTimeouts;
    uint32_t averageResponseTime;
    
    // System health
    uint32_t freeHeap;
    uint32_t uptime;
    bool systemStable;
    
    DiagnosticInfo() : totalErrors(0), lastErrorTime(0), lastErrorType(RS485ErrorType::NONE),
                      rxBufferHealth(100), txBufferHealth(100), totalBufferOverflows(0),
                      uartHealthy(true), communicationActive(false), consecutiveTimeouts(0),
                      averageResponseTime(0), freeHeap(0), uptime(0), systemStable(true) {
        memset(errorsByType, 0, sizeof(errorsByType));
    }
};

// Parse result structure
struct ParseResult {
    bool success = false;
    CommandType type = CommandType::INVALID;
    std::vector<String> parameters;
    ParseError error = ParseError::NONE;
};

// Command structure
struct Command {
    CommandType type;
    String raw;
    std::vector<String> params;
    uint32_t timestamp;
};

// Enhanced circular buffer template with overflow handling and monitoring
template<typename T>
class CircularBuffer {
private:
    T* buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    size_t overflowCount;
    bool overflowOccurred;

public:
    CircularBuffer(size_t size) : capacity(size), head(0), tail(0), count(0), 
                                 overflowCount(0), overflowOccurred(false) {
        buffer = new T[capacity];
        if (!buffer) {
            capacity = 0;
        }
    }
    
    ~CircularBuffer() {
        if (buffer) {
            delete[] buffer;
        }
    }
    
    // Core buffer operations
    bool push(const T& item) {
        if (!buffer) return false;
        
        if (count >= capacity) {
            // Buffer is full - handle overflow
            overflowCount++;
            overflowOccurred = true;
            return false;
        }
        
        buffer[head] = item;
        head = (head + 1) % capacity;
        count++;
        return true;
    }
    
    bool pop(T& item) {
        if (!buffer || count == 0) return false;
        
        item = buffer[tail];
        tail = (tail + 1) % capacity;
        count--;
        return true;
    }
    
    // Forced push with overflow handling (overwrites oldest data)
    bool pushOverwrite(const T& item) {
        if (!buffer) return false;
        
        if (count >= capacity) {
            // Overwrite oldest data
            tail = (tail + 1) % capacity;
            count--;
            overflowCount++;
            overflowOccurred = true;
        }
        
        buffer[head] = item;
        head = (head + 1) % capacity;
        count++;
        return true;
    }
    
    // Peek at next item without removing it
    bool peek(T& item) const {
        if (!buffer || count == 0) return false;
        
        item = buffer[tail];
        return true;
    }
    
    // Peek at item at specific offset from tail
    bool peekAt(size_t offset, T& item) const {
        if (!buffer || offset >= count) return false;
        
        size_t index = (tail + offset) % capacity;
        item = buffer[index];
        return true;
    }
    
    // Status methods
    bool isFull() const { return count >= capacity; }
    bool isEmpty() const { return count == 0; }
    size_t size() const { return count; }
    size_t getCapacity() const { return capacity; }
    size_t available() const { return capacity - count; }
    
    // Overflow monitoring
    size_t getOverflowCount() const { return overflowCount; }
    bool hasOverflowed() const { return overflowOccurred; }
    void clearOverflowFlag() { overflowOccurred = false; }
    void resetOverflowCount() { overflowCount = 0; overflowOccurred = false; }
    
    // Buffer management
    void clear() { 
        head = tail = count = 0; 
        overflowOccurred = false;
    }
    
    void reset() {
        clear();
        overflowCount = 0;
    }
    
    // Get buffer utilization percentage (0-100)
    uint8_t getUtilization() const {
        if (capacity == 0) return 100;
        return (count * 100) / capacity;
    }
    
    // Check if buffer is valid (memory allocated successfully)
    bool isValid() const { return buffer != nullptr && capacity > 0; }
    
    // Find item in buffer (returns index from tail, or -1 if not found)
    int find(const T& item) const {
        if (!buffer || count == 0) return -1;
        
        for (size_t i = 0; i < count; i++) {
            size_t index = (tail + i) % capacity;
            if (buffer[index] == item) {
                return (int)i;
            }
        }
        return -1;
    }
    
    // Copy data to external buffer (useful for debugging)
    size_t copyTo(T* dest, size_t maxItems) const {
        if (!buffer || !dest || count == 0) return 0;
        
        size_t itemsToCopy = (maxItems < count) ? maxItems : count;
        
        for (size_t i = 0; i < itemsToCopy; i++) {
            size_t index = (tail + i) % capacity;
            dest[i] = buffer[index];
        }
        
        return itemsToCopy;
    }
};

// Configuration validation results
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

// Forward declarations
class EchoService;
class PresetController;
class ConfigManager;
class ErrorLogger;

// ErrorLogger class declaration
class ErrorLogger {
private:
    static const size_t MAX_LOG_ENTRIES = 50;
    CircularBuffer<ErrorLogEntry>* logBuffer;
    DiagnosticInfo diagnostics;
    bool diagnosticMode;
    uint32_t lastDiagnosticUpdate;
    
    // Helper methods
    String errorTypeToString(RS485ErrorType type);
    String severityToString(ErrorSeverity severity);
    void updateDiagnostics();
    void pruneOldEntries();

public:
    ErrorLogger();
    ~ErrorLogger();
    
    // Core logging methods
    void logError(RS485ErrorType type, ErrorSeverity severity, const String& message, 
                  const String& details = "", uint32_t errorCode = 0);
    void logInfo(const String& message, const String& details = "");
    void logWarning(const String& message, const String& details = "");
    void logCritical(const String& message, const String& details = "");
    
    // Diagnostic methods
    void setDiagnosticMode(bool enabled);
    bool isDiagnosticModeEnabled() const;
    DiagnosticInfo getDiagnostics();
    String getDiagnosticReport();
    String getErrorSummary();
    
    // Log management
    void clearLog();
    size_t getLogSize() const;
    std::vector<ErrorLogEntry> getRecentErrors(size_t count = 10);
    String getLogAsString(size_t maxEntries = 20);
    
    // Integration with WLED logging
    void outputToWLEDLog(const ErrorLogEntry& entry);
    void enableWLEDIntegration(bool enable);
    
    // Statistics
    void updateStats(const RS485Stats& stats);
    void recordBufferOverflow(bool isReceiveBuffer);
    void recordTimeout();
    void recordParseError(ParseError error);
    void recordCommunicationActivity();
};

// CommandParser class declaration
class CommandParser {
public:
    ParseResult parseCommand(const String& input);
    String getErrorMessage(ParseError error);
    bool isValidCommand(const String& input);
    String getCommandHelp();
    String getParserStats();
    
private:
    bool validateCommand(const String& cmd);
    CommandType getCommandType(const String& cmd);
    std::vector<String> extractParameters(const String& cmd);
    String processSpecialCharacters(const String& input);
    bool validatePresetId(const String& presetIdStr);
};

// RS485Interface class declaration
class RS485Interface {
private:
    HardwareSerial* serial;
    CircularBuffer<char>* rxBuffer;
    CircularBuffer<char>* txBuffer;
    RS485Config* config;
    RS485Stats* stats;
    bool initialized;
    unsigned long lastReceiveTime;

public:
    RS485Interface();
    ~RS485Interface();
    
    // Core interface methods
    bool initialize(int rxPin, int txPin, uint32_t baudRate);
    bool initialize(RS485Config* cfg, RS485Stats* st);
    void loop();
    bool sendMessage(const String& message);
    bool hasMessage();
    String receiveMessage();
    void setConfig(const RS485Config& config);
    RS485Stats getStats();
    
    // Status methods
    bool isInitialized() const;
    bool isConnected() const;
    void resetStats();
    
    // Buffer management
    void clearBuffers();
    size_t getReceiveBufferSize() const;
    size_t getTransmitBufferSize() const;
    bool isReceiveBufferFull() const;
    bool isTransmitBufferFull() const;
    
    // Enhanced buffer monitoring
    uint8_t getReceiveBufferUtilization() const;
    uint8_t getTransmitBufferUtilization() const;
    size_t getReceiveBufferOverflows() const;
    size_t getTransmitBufferOverflows() const;
    void resetBufferStats();
    bool areBuffersHealthy() const;
    String getBufferStatus() const;
};

// Main RS485 Control Usermod class
class RS485ControlUsermod : public Usermod {
private:
    // Core components
    RS485Interface* rs485Interface;
    CommandParser* commandParser;
    EchoService* echoService;
    PresetController* presetController;
    ConfigManager* configManager;
    ErrorLogger* errorLogger;
    
    // Configuration and state
    RS485Config config;
    RS485Stats stats;
    bool initDone = false;
    unsigned long lastLoop = 0;
    
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

public:
    // Constructor and destructor
    RS485ControlUsermod();
    ~RS485ControlUsermod();
    
    // Usermod interface methods
    void setup() override;
    void loop() override;
    void connected() override;
    
    // Configuration methods
    void addToConfig(JsonObject& root) override;
    bool readFromConfig(JsonObject& root) override;
    void appendConfigData() override;
    
    // JSON API methods
    void addToJsonInfo(JsonObject& root) override;
    void addToJsonState(JsonObject& root) override;
    void readFromJsonState(JsonObject& root) override;
    
    // State change handling
    void onStateChange(uint8_t mode) override;
    
    // Usermod identification
    uint16_t getId() override { return USERMOD_ID_RS485_CONTROL; }
    
    // Public interface methods
    bool isEnabled() const { return config.enabled; }
    void enable(bool enable) { config.enabled = enable; }
    const RS485Stats& getStats() const { return stats; }
    const RS485Config& getConfig() const { return config; }
    
    // Error logging interface
    ErrorLogger* getErrorLogger() { return errorLogger; }
    String getErrorSummary() { return errorLogger ? errorLogger->getErrorSummary() : ""; }
    String getDiagnosticReport() { return errorLogger ? errorLogger->getDiagnosticReport() : ""; }
    void clearErrorLog() { if (errorLogger) errorLogger->clearLog(); }
    
    // Component coordination and performance monitoring
    void performHealthCheck();
    void optimizePerformance();
    void coordinateComponents();
};