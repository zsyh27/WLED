#include <unity.h>
#include <Arduino.h>

// Mock the WLED dependencies for testing
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)

// Include the RS485Stats structure for testing
struct RS485Stats {
    uint32_t messagesReceived = 0;
    uint32_t messagesSent = 0;
    uint32_t parseErrors = 0;
    uint32_t bufferOverflows = 0;
    uint32_t timeouts = 0;
    uint32_t lastActivity = 0;
    bool connected = false;
};

// EchoService class for testing (extracted from the main implementation)
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
        if (!enabled) {
            if (debugMode && stats) {
                DEBUG_PRINTLN(F("EchoService: Echo disabled, message ignored"));
            }
            return "";
        }
        
        if (message.length() == 0) {
            if (debugMode) {
                DEBUG_PRINTLN(F("EchoService: Empty message received"));
            }
            return "";
        }
        
        // Process the message to handle special characters and preserve formatting
        String processedMessage = processSpecialCharacters(message);
        
        if (debugMode) {
            DEBUG_PRINTF("EchoService: Processing echo - Input: '%s', Output: '%s'\n", 
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
        
        if (debugMode) {
            DEBUG_PRINTF("EchoService: Echo command processed - Response: '%s'\n", response.c_str());
        }
        
        return true;
    }
    
    void setEnabled(bool enable) { 
        enabled = enable; 
        if (debugMode) {
            DEBUG_PRINTF("EchoService: Echo service %s\n", enable ? "enabled" : "disabled");
        }
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
    
    void reset() {
        if (debugMode) {
            DEBUG_PRINTLN(F("EchoService: Service reset"));
        }
    }
};

// Test instances
EchoService echoService;
RS485Stats testStats;

void setUp(void) {
    // Initialize before each test
    echoService.initialize(&testStats, false);
    echoService.setEnabled(true);
}

void tearDown(void) {
    // Clean up after each test
    echoService.reset();
}

// Test basic echo functionality
void test_basic_echo() {
    String input = "Hello World";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Hello World", output.c_str());
}

// Test empty message handling
void test_empty_message() {
    String input = "";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("", output.c_str());
}

// Test enable/disable functionality
void test_enable_disable() {
    String input = "Test message";
    
    // Test enabled
    echoService.setEnabled(true);
    TEST_ASSERT_TRUE(echoService.isEnabled());
    String output = echoService.processEcho(input);
    TEST_ASSERT_EQUAL_STRING("Test message", output.c_str());
    
    // Test disabled
    echoService.setEnabled(false);
    TEST_ASSERT_FALSE(echoService.isEnabled());
    output = echoService.processEcho(input);
    TEST_ASSERT_EQUAL_STRING("", output.c_str());
}

// Test newline preservation (Requirement 2.2)
void test_newline_preservation() {
    String input = "Line 1\nLine 2";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Line 1\nLine 2", output.c_str());
}

// Test carriage return preservation
void test_carriage_return_preservation() {
    String input = "Line 1\r\nLine 2";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Line 1\r\nLine 2", output.c_str());
}

// Test tab character preservation
void test_tab_preservation() {
    String input = "Column1\tColumn2";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Column1\tColumn2", output.c_str());
}

// Test escape sequence handling
void test_escape_sequences() {
    String input = "Line 1\\nLine 2";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Line 1\nLine 2", output.c_str());
}

// Test backslash escape
void test_backslash_escape() {
    String input = "Path\\\\to\\\\file";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("Path\\to\\file", output.c_str());
}

// Test echo command processing
void test_echo_command_processing() {
    String command = "ECHO Hello World";
    String response;
    
    bool result = echoService.processEchoCommand(command, response);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Hello World", response.c_str());
}

// Test echo command when disabled
void test_echo_command_disabled() {
    echoService.setEnabled(false);
    
    String command = "ECHO Hello World";
    String response;
    
    bool result = echoService.processEchoCommand(command, response);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("ERROR: Echo service is disabled", response.c_str());
}

// Test special characters filtering
void test_special_characters_filtering() {
    // Test with non-printable characters (should be filtered out)
    String input = "Hello\x01\x02World";
    String output = echoService.processEcho(input);
    
    TEST_ASSERT_EQUAL_STRING("HelloWorld", output.c_str());
}

// Test status reporting
void test_status_reporting() {
    echoService.setEnabled(true);
    String status = echoService.getStatus();
    TEST_ASSERT_TRUE(status.indexOf("Enabled") >= 0);
    
    echoService.setEnabled(false);
    status = echoService.getStatus();
    TEST_ASSERT_TRUE(status.indexOf("Disabled") >= 0);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    UNITY_BEGIN();
    
    // Run unit tests
    RUN_TEST(test_basic_echo);
    RUN_TEST(test_empty_message);
    RUN_TEST(test_enable_disable);
    RUN_TEST(test_newline_preservation);
    RUN_TEST(test_carriage_return_preservation);
    RUN_TEST(test_tab_preservation);
    RUN_TEST(test_escape_sequences);
    RUN_TEST(test_backslash_escape);
    RUN_TEST(test_echo_command_processing);
    RUN_TEST(test_echo_command_disabled);
    RUN_TEST(test_special_characters_filtering);
    RUN_TEST(test_status_reporting);
    
    UNITY_END();
}

void loop() {
    // Empty loop
}