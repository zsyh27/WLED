#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

// Mock Arduino/ESP32 functions for testing
unsigned long millis() { return 12345; }
void delay(int ms) {}
uint32_t ESP_getFreeHeap() { return 50000; }

// Mock WLED debug macros
#define DEBUG_PRINTLN(x) std::cout << x << std::endl
#define DEBUG_PRINTF(fmt, ...) printf(fmt, __VA_ARGS__)

// Mock String class for testing
class String {
private:
    std::string data;
public:
    String() {}
    String(const char* str) : data(str) {}
    String(const std::string& str) : data(str) {}
    String(int val) : data(std::to_string(val)) {}
    String(uint32_t val) : data(std::to_string(val)) {}
    String(bool val) : data(val ? "true" : "false") {}
    
    const char* c_str() const { return data.c_str(); }
    size_t length() const { return data.length(); }
    String operator+(const String& other) const { return String(data + other.data); }
    String& operator+=(const String& other) { data += other.data; return *this; }
    String& operator+=(char c) { data += c; return *this; }
    bool operator==(const String& other) const { return data == other.data; }
    bool operator!=(const String& other) const { return data != other.data; }
    void trim() { /* simplified */ }
    void toUpperCase() { /* simplified */ }
    int indexOf(char c) const { return data.find(c); }
    int indexOf(char c, int start) const { return data.find(c, start); }
    String substring(int start) const { return String(data.substr(start)); }
    String substring(int start, int end) const { return String(data.substr(start, end - start)); }
    void reserve(size_t size) { data.reserve(size); }
    int toInt() const { return std::stoi(data); }
};

// Mock ESP class
class ESP_Class {
public:
    uint32_t getFreeHeap() { return 50000; }
};
ESP_Class ESP;

// Include the error logging types and classes
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

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class ParseError {
    NONE,
    INVALID_FORMAT,
    UNKNOWN_COMMAND,
    INVALID_PARAMETERS,
    PARAMETER_OUT_OF_RANGE,
    BUFFER_OVERFLOW
};

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

struct DiagnosticInfo {
    uint32_t totalErrors;
    uint32_t errorsByType[10];
    uint32_t lastErrorTime;
    RS485ErrorType lastErrorType;
    
    uint8_t rxBufferHealth;
    uint8_t txBufferHealth;
    uint32_t totalBufferOverflows;
    
    bool uartHealthy;
    bool communicationActive;
    uint32_t consecutiveTimeouts;
    uint32_t averageResponseTime;
    
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

// Simple circular buffer for testing
template<typename T>
class CircularBuffer {
private:
    T* buffer;
    size_t capacity;
    size_t head, tail, count;
    size_t overflowCount;
    bool overflowOccurred;

public:
    CircularBuffer(size_t size) : capacity(size), head(0), tail(0), count(0), 
                                 overflowCount(0), overflowOccurred(false) {
        buffer = new T[capacity];
    }
    
    ~CircularBuffer() { delete[] buffer; }
    
    bool push(const T& item) {
        if (count >= capacity) {
            overflowCount++;
            overflowOccurred = true;
            return false;
        }
        buffer[head] = item;
        head = (head + 1) % capacity;
        count++;
        return true;
    }
    
    bool pushOverwrite(const T& item) {
        if (count >= capacity) {
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
    
    bool pop(T& item) {
        if (count == 0) return false;
        item = buffer[tail];
        tail = (tail + 1) % capacity;
        count--;
        return true;
    }
    
    bool peekAt(size_t offset, T& item) const {
        if (offset >= count) return false;
        size_t index = (tail + offset) % capacity;
        item = buffer[index];
        return true;
    }
    
    bool isFull() const { return count >= capacity; }
    bool isEmpty() const { return count == 0; }
    size_t size() const { return count; }
    size_t getCapacity() const { return capacity; }
    void clear() { head = tail = count = 0; overflowOccurred = false; }
    void resetOverflowCount() { overflowCount = 0; overflowOccurred = false; }
    size_t getOverflowCount() const { return overflowCount; }
    bool hasOverflowed() const { return overflowOccurred; }
};

// Include the ErrorLogger implementation (simplified version)
class ErrorLogger {
private:
    static const size_t MAX_LOG_ENTRIES = 50;
    CircularBuffer<ErrorLogEntry>* logBuffer;
    DiagnosticInfo diagnostics;
    bool diagnosticMode;
    bool wledIntegrationEnabled;
    uint32_t lastDiagnosticUpdate;
    
    String errorTypeToString(RS485ErrorType type) {
        switch (type) {
            case RS485ErrorType::NONE: return "NONE";
            case RS485ErrorType::UART_INIT_FAILED: return "UART_INIT_FAILED";
            case RS485ErrorType::BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
            case RS485ErrorType::COMMUNICATION_TIMEOUT: return "COMMUNICATION_TIMEOUT";
            case RS485ErrorType::PARSE_ERROR: return "PARSE_ERROR";
            case RS485ErrorType::HARDWARE_FAULT: return "HARDWARE_FAULT";
            case RS485ErrorType::MEMORY_ERROR: return "MEMORY_ERROR";
            case RS485ErrorType::CONFIG_ERROR: return "CONFIG_ERROR";
            case RS485ErrorType::PRESET_ERROR: return "PRESET_ERROR";
            case RS485ErrorType::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
            default: return "UNDEFINED";
        }
    }
    
    String severityToString(ErrorSeverity severity) {
        switch (severity) {
            case ErrorSeverity::INFO: return "INFO";
            case ErrorSeverity::WARNING: return "WARNING";
            case ErrorSeverity::ERROR: return "ERROR";
            case ErrorSeverity::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }

public:
    ErrorLogger() : logBuffer(nullptr), diagnosticMode(false), wledIntegrationEnabled(true), 
                   lastDiagnosticUpdate(0) {
        logBuffer = new CircularBuffer<ErrorLogEntry>(MAX_LOG_ENTRIES);
    }
    
    ~ErrorLogger() {
        if (logBuffer) delete logBuffer;
    }
    
    void logError(RS485ErrorType type, ErrorSeverity severity, const String& message, 
                  const String& details = "", uint32_t errorCode = 0) {
        if (!logBuffer) return;
        
        ErrorLogEntry entry(type, severity, message, details, errorCode);
        
        diagnostics.totalErrors++;
        if ((int)type < 10) {
            diagnostics.errorsByType[(int)type]++;
        }
        diagnostics.lastErrorTime = entry.timestamp;
        diagnostics.lastErrorType = type;
        
        if (!logBuffer->push(entry)) {
            logBuffer->pushOverwrite(entry);
        }
        
        if (wledIntegrationEnabled) {
            outputToWLEDLog(entry);
        }
    }
    
    void logInfo(const String& message, const String& details = "") {
        logError(RS485ErrorType::NONE, ErrorSeverity::INFO, message, details);
    }
    
    void logWarning(const String& message, const String& details = "") {
        logError(RS485ErrorType::UNKNOWN_ERROR, ErrorSeverity::WARNING, message, details);
    }
    
    void logCritical(const String& message, const String& details = "") {
        logError(RS485ErrorType::UNKNOWN_ERROR, ErrorSeverity::CRITICAL, message, details);
    }
    
    void setDiagnosticMode(bool enabled) {
        diagnosticMode = enabled;
        if (enabled) {
            logInfo("Diagnostic mode enabled", "Detailed logging activated");
        }
    }
    
    bool isDiagnosticModeEnabled() const {
        return diagnosticMode;
    }
    
    String getErrorSummary() {
        String summary = "Errors: " + String(diagnostics.totalErrors);
        if (diagnostics.totalErrors > 0) {
            summary += " (Last: " + errorTypeToString(diagnostics.lastErrorType) + ")";
        }
        summary += " | Timeouts: " + String(diagnostics.consecutiveTimeouts);
        summary += " | Overflows: " + String(diagnostics.totalBufferOverflows);
        return summary;
    }
    
    void clearLog() {
        if (logBuffer) {
            logBuffer->clear();
        }
        diagnostics.totalErrors = 0;
        diagnostics.lastErrorTime = 0;
        diagnostics.lastErrorType = RS485ErrorType::NONE;
        diagnostics.totalBufferOverflows = 0;
        diagnostics.consecutiveTimeouts = 0;
        memset(diagnostics.errorsByType, 0, sizeof(diagnostics.errorsByType));
        
        logInfo("Error log cleared", "All statistics reset");
    }
    
    size_t getLogSize() const {
        return logBuffer ? logBuffer->size() : 0;
    }
    
    void outputToWLEDLog(const ErrorLogEntry& entry) {
        String logMessage = "RS485: " + severityToString(entry.severity) + " - " + 
                           errorTypeToString(entry.errorType) + ": " + entry.message;
        
        if (entry.details.length() > 0) {
            logMessage += " (" + entry.details + ")";
        }
        
        switch (entry.severity) {
            case ErrorSeverity::INFO:
                if (diagnosticMode) {
                    DEBUG_PRINTLN(logMessage);
                }
                break;
            case ErrorSeverity::WARNING:
            case ErrorSeverity::ERROR:
            case ErrorSeverity::CRITICAL:
                DEBUG_PRINTLN(logMessage);
                break;
        }
    }
    
    void recordBufferOverflow(bool isReceiveBuffer) {
        diagnostics.totalBufferOverflows++;
        
        String bufferType = isReceiveBuffer ? "RX" : "TX";
        logError(RS485ErrorType::BUFFER_OVERFLOW, ErrorSeverity::WARNING,
                "Buffer overflow detected", bufferType + " buffer full");
    }
    
    void recordTimeout() {
        diagnostics.consecutiveTimeouts++;
        
        logError(RS485ErrorType::COMMUNICATION_TIMEOUT, ErrorSeverity::WARNING,
                "Communication timeout", "Consecutive: " + String(diagnostics.consecutiveTimeouts));
    }
    
    void recordParseError(ParseError error) {
        String errorMsg = "Parse error: ";
        switch (error) {
            case ParseError::INVALID_FORMAT: errorMsg += "Invalid format"; break;
            case ParseError::UNKNOWN_COMMAND: errorMsg += "Unknown command"; break;
            case ParseError::INVALID_PARAMETERS: errorMsg += "Invalid parameters"; break;
            case ParseError::PARAMETER_OUT_OF_RANGE: errorMsg += "Parameter out of range"; break;
            case ParseError::BUFFER_OVERFLOW: errorMsg += "Buffer overflow"; break;
            default: errorMsg += "Unknown"; break;
        }
        
        logError(RS485ErrorType::PARSE_ERROR, ErrorSeverity::WARNING, errorMsg);
    }
};

// Test functions
void testBasicLogging() {
    std::cout << "\n=== Testing Basic Logging ===" << std::endl;
    
    ErrorLogger logger;
    
    // Test basic logging
    logger.logInfo("System started", "Initialization complete");
    logger.logWarning("Low memory", "Free heap: 1024 bytes");
    logger.logError(RS485ErrorType::UART_INIT_FAILED, ErrorSeverity::ERROR, 
                   "UART initialization failed", "GPIO pins invalid");
    logger.logCritical("System failure", "Critical error occurred");
    
    std::cout << "Log size: " << logger.getLogSize() << std::endl;
    std::cout << "Error summary: " << logger.getErrorSummary().c_str() << std::endl;
}

void testDiagnosticMode() {
    std::cout << "\n=== Testing Diagnostic Mode ===" << std::endl;
    
    ErrorLogger logger;
    logger.setDiagnosticMode(true);
    
    logger.logInfo("Diagnostic test", "This should be visible");
    logger.recordBufferOverflow(true);
    logger.recordTimeout();
    logger.recordParseError(ParseError::INVALID_FORMAT);
    
    std::cout << "Diagnostic mode enabled: " << (logger.isDiagnosticModeEnabled() ? "Yes" : "No") << std::endl;
}

void testErrorRecording() {
    std::cout << "\n=== Testing Error Recording ===" << std::endl;
    
    ErrorLogger logger;
    
    // Test different error types
    logger.recordBufferOverflow(true);   // RX buffer
    logger.recordBufferOverflow(false);  // TX buffer
    logger.recordTimeout();
    logger.recordTimeout();
    logger.recordParseError(ParseError::UNKNOWN_COMMAND);
    logger.recordParseError(ParseError::PARAMETER_OUT_OF_RANGE);
    
    std::cout << "Final error summary: " << logger.getErrorSummary().c_str() << std::endl;
}

void testLogClearing() {
    std::cout << "\n=== Testing Log Clearing ===" << std::endl;
    
    ErrorLogger logger;
    
    // Add some errors
    logger.logError(RS485ErrorType::PARSE_ERROR, ErrorSeverity::WARNING, "Test error 1");
    logger.logError(RS485ErrorType::BUFFER_OVERFLOW, ErrorSeverity::ERROR, "Test error 2");
    
    std::cout << "Before clear - Log size: " << logger.getLogSize() << std::endl;
    std::cout << "Before clear - Error summary: " << logger.getErrorSummary().c_str() << std::endl;
    
    logger.clearLog();
    
    std::cout << "After clear - Log size: " << logger.getLogSize() << std::endl;
    std::cout << "After clear - Error summary: " << logger.getErrorSummary().c_str() << std::endl;
}

int main() {
    std::cout << "RS485 Error Logger Test Suite" << std::endl;
    std::cout << "=============================" << std::endl;
    
    testBasicLogging();
    testDiagnosticMode();
    testErrorRecording();
    testLogClearing();
    
    std::cout << "\n=== All Tests Completed ===" << std::endl;
    return 0;
}