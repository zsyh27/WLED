#include <Arduino.h>
#include <HardwareSerial.h>

// Simple UART test program for ESP32
// This program tests basic UART2 functionality on GPIO26/27
// Use this to verify hardware connections before testing full usermod

void setup() {
    // Initialize USB Serial for debug output
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== ESP32 UART2 Test Program ===");
    Serial.println("Testing RS485 communication on GPIO26(RX)/GPIO27(TX)");
    Serial.println("Baud rate: 9600, 8N1");
    Serial.println("Commands: ECHO <message>, STATUS");
    Serial.println("=====================================");
    
    // Initialize UART2 for RS485 communication
    // GPIO26 = RX, GPIO27 = TX, 9600 baud, 8 data bits, no parity, 1 stop bit
    Serial2.begin(9600, SERIAL_8N1, 26, 27);
    
    Serial.println("UART2 initialized successfully");
    Serial.println("Waiting for RS485 commands...");
}

void loop() {
    // Check for incoming data on UART2 (RS485)
    if (Serial2.available()) {
        String message = "";
        
        // Read complete message (until newline or timeout)
        unsigned long startTime = millis();
        while (millis() - startTime < 100) { // 100ms timeout
            if (Serial2.available()) {
                char c = Serial2.read();
                if (c == '\n' || c == '\r') {
                    break; // End of message
                } else if (c >= 32 && c <= 126) { // Printable ASCII
                    message += c;
                }
            }
            delay(1);
        }
        
        if (message.length() > 0) {
            Serial.println("Received: '" + message + "'");
            
            // Process simple commands
            String response = "";
            message.toUpperCase();
            
            if (message.startsWith("ECHO ")) {
                response = message.substring(5); // Remove "ECHO " prefix
            } else if (message == "ECHO") {
                response = "ECHO command received";
            } else if (message == "STATUS") {
                response = "UART2 Test: OK, GPIO26/27, 9600 baud";
            } else if (message == "TEST") {
                response = "Test response from ESP32";
            } else {
                response = "Unknown command: " + message;
            }
            
            // Send response
            if (response.length() > 0) {
                Serial2.println(response);
                Serial.println("Sent: '" + response + "'");
            }
        }
    }
    
    // Check for commands from USB Serial (for testing)
    if (Serial.available()) {
        String usbCommand = Serial.readStringUntil('\n');
        usbCommand.trim();
        
        if (usbCommand.length() > 0) {
            Serial.println("USB Command: '" + usbCommand + "'");
            Serial2.println(usbCommand);
            Serial.println("Forwarded to UART2");
        }
    }
    
    // Periodic status output
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) { // Every 10 seconds
        Serial.println("Status: UART2 running, waiting for commands...");
        lastStatus = millis();
    }
    
    delay(10);
}