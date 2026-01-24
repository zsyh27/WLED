# RS485 Control Usermod

This usermod adds RS485 serial communication control to WLED, allowing remote control of LED effects through text commands sent over a 485 bus network.

## Features

- **RS485 Serial Communication**: Uses UART2 (GPIO26/27) for 485 communication
- **Text Command Interface**: Simple text-based commands for easy integration
- **Echo Service**: Built-in echo functionality for connection testing
- **Preset Control**: Activate and query WLED presets via serial commands
- **Configurable Parameters**: Adjustable baud rate, pins, and buffer sizes
- **Error Handling**: Comprehensive error reporting and recovery
- **Web Interface Integration**: Configuration through WLED settings page

## Hardware Requirements

- ESP32-WROOM-32E or compatible ESP32 module
- RS485 to TTL converter module (e.g., MAX485, SP3485)
- 485 bus network infrastructure

## Wiring

| ESP32 Pin | RS485 Module | Function |
|-----------|--------------|----------|
| GPIO26    | RO (RX)      | Receive  |
| GPIO27    | DI (TX)      | Transmit |
| 3.3V      | VCC          | Power    |
| GND       | GND          | Ground   |

## Supported Commands

### Echo Command
```
ECHO <message>
```
Returns the same message back to sender. Useful for testing connectivity.

### Preset Commands
```
PRESET SET <id>     # Activate preset (1-16)
PRESET GET          # Get current preset info
```

### Status Command
```
STATUS              # Get RS485 interface status
```

## Configuration

The usermod can be configured through the WLED web interface under "Usermod Settings":

- **Enabled**: Enable/disable the RS485 functionality
- **Baud Rate**: Communication speed (9600, 19200, 38400, 57600 bps)
- **RX Pin**: GPIO pin for receiving data (default: 26)
- **TX Pin**: GPIO pin for transmitting data (default: 27)
- **Buffer Size**: Internal buffer size in bytes (default: 512)
- **Timeout**: Communication timeout in milliseconds (default: 1000)
- **Echo Enabled**: Enable/disable echo service (default: true)
- **Debug Mode**: Enable debug output (default: false)

## Installation

1. Copy the `rs485_control` folder to your WLED `usermods/` directory
2. Add the usermod to your build by including it in your platformio.ini or build configuration
3. The usermod will be automatically registered and available in the settings

## Usage Example

1. Connect your RS485 hardware as described in the wiring section
2. Enable the usermod in WLED settings
3. Configure the desired baud rate and pins
4. Send commands from your central control system:

```
ECHO Hello World        # Test connectivity
PRESET SET 1           # Activate preset 1
PRESET GET             # Check current preset
STATUS                 # Get interface status
```

## Error Handling

The usermod provides comprehensive error handling:

- **Invalid Format**: Command syntax errors
- **Unknown Command**: Unsupported command types
- **Invalid Parameters**: Parameter validation failures
- **Buffer Overflow**: Automatic buffer clearing and recovery
- **Communication Timeout**: Connection monitoring and recovery

## Integration with Central Control Systems

This usermod is designed to work with central control systems that can communicate over RS485 networks. The simple text protocol makes it easy to integrate with:

- Building automation systems
- Lighting control panels
- Custom control software
- Industrial control systems

## Performance Considerations

- The usermod runs in the main WLED loop with minimal overhead
- Buffer sizes can be adjusted based on message volume
- Loop frequency is limited to prevent system overload
- All string constants are stored in PROGMEM to save RAM

## Troubleshooting

1. **No Response**: Check wiring and baud rate settings
2. **Garbled Messages**: Verify baud rate matches on both ends
3. **Buffer Overflows**: Increase buffer size or reduce message frequency
4. **Parse Errors**: Check command syntax and format

## Development Notes

This usermod follows WLED v2 usermod architecture and integrates cleanly with the existing WLED system without affecting other functionality.

## License

This usermod is released under the same license as WLED.