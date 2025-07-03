# Permission-Enhanced Sensor Discovery

This enhancement adds detailed device permission checking and **dynamic device discovery** to the sensor discovery system. When sensors fail to open due to permission issues, the application now shows a comprehensive list of available devices with their current permissions compared to desired ones.

## New Features

### 1. Dynamic Device Discovery

The system now **automatically scans `/dev`** for serial devices instead of relying on hardcoded device names:
- **macOS**: Discovers all `cu.*` and `tty.*` devices matching USB serial patterns
- **Linux**: Discovers all `ttyUSB*`, `ttyACM*`, and other serial device patterns
- **Smart Pattern Matching**: Recognizes common USB-to-serial chipsets and device naming conventions
- **SDS011-Specific Patterns**: Includes known device patterns that work with SDS011 sensors

### 2. Enhanced Device Permission Checking

The system now provides detailed information about each discovered device:
- **Existence**: Whether the device file exists
- **Read/Write Permissions**: Current user's access rights
- **Owner/Group**: Device ownership information
- **Permission String**: Unix-style permission string (e.g., "rw-rw----")
- **Error Messages**: Specific permission-related error descriptions

### 3. Improved TUI Display

When no working sensors are detected, the interactive TUI now shows:
- A list of all **discovered** serial devices (not just hardcoded paths)
- Their current permissions in Unix format
- Owner and group information
- Access status for current user
- Specific error messages explaining permission issues
- Helpful commands to fix permission problems

### 3. Enhanced Error Messages

When sensor initialization fails, error messages now include:
- Specific permission details
- Suggested fix commands
- Clear indication of what permissions are missing

## Example Output

When the system dynamically discovers devices:

```
No working sensors detected.

Scanning /dev for serial devices...

Found 4 serial device(s) with permission details:
Port                 Permissions  Owner:Group  Access     Issue
----------------------------------------------------------------
/dev/cu.usbserial-1140    rw-rw-rw-    root:wheel   R/W Access OK
/dev/tty.usbserial-1140   rw-rw-rw-    root:wheel   R/W Access OK
/dev/cu.Bluetooth-Port    rw-rw-rw-    root:wheel   R/W Access OK
/dev/tty.Bluetooth-Port   rw-rw-rw-    root:wheel   R/W Access OK

To fix permission issues:
  sudo chmod 666 /dev/tty.* /dev/cu.*
  sudo usermod -a -G dialout $USER
  (then logout and login again)
```

## Implementation Details

### New Data Structures

- `DevicePermissions`: Stores comprehensive permission information
- Enhanced `SensorInfo`: Now includes device permission details

### New Methods

- `SensorRegistry::discoverSerialDevices()`: **Dynamic device discovery by scanning `/dev`**
- `SensorRegistry::checkDevicePermissions()`: Analyzes device permissions
- `SensorRegistry::discoverAllDevices()`: Enhanced discovery with permissions
- `SDS011Plugin::getKnownDevicePatterns()`: **SDS011-specific device patterns**
- `DevicePermissions::getPermissionString()`: Formats permissions for display
- `DevicePermissions::getStatusString()`: Human-readable access status

### Supported Device Patterns

#### macOS
- `cu.usbserial*` and `tty.usbserial*` (like `/dev/tty.usbserial-1140`)
- `cu.usbmodem*` and `tty.usbmodem*`
- `cu.SLAB_USBtoUART*` and `tty.SLAB_USBtoUART*`
- `cu.CH34*`, `cu.CP210*`, `cu.FT*` patterns
- Any `cu.usb*` or `tty.usb*` device

#### Linux
- `ttyUSB*` (standard USB-to-serial)
- `ttyACM*` (Arduino-compatible devices)
- `ttyS*` (built-in serial ports)
- `ttyAMA*` (ARM serial ports)

### Key Benefits

1. **Better User Experience**: Users immediately see why sensors aren't working
2. **Clear Fix Instructions**: Specific commands provided to resolve issues
3. **Permission Awareness**: Shows exactly what permissions are needed vs. current
4. **No False Negatives**: Distinguishes between missing devices and permission issues

## Usage

The enhanced permission checking works automatically:

1. Run the sensor reader: `./sensor_reader`
2. If no sensors work due to permissions, the detailed device list appears
3. Follow the suggested commands to fix permission issues
4. Press 'r' to refresh and retry sensor detection

## Platform Support

- **Linux**: Full support for checking dialout group membership and standard permissions
- **macOS**: Full support for checking device ownership and permissions
- **Cross-platform**: Works with different serial device naming conventions

This enhancement makes the sensor reader much more user-friendly, especially for new users who haven't configured device permissions yet.
