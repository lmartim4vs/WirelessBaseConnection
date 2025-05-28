# BLE GATTS Server - Refactored

This is a refactored version of the ESP-IDF BLE GATTS server demo, organized into multiple files for better maintainability and readability.

## File Structure

```
├── main.c                  # Main application entry point
├── ble_config.h            # Configuration constants and defines
├── ble_gap_handler.h/.c    # GAP (advertising) functionality
├── ble_gatts_server.h/.c   # Main GATTS server logic
├── ble_profile_a.h/.c      # Profile A implementation
├── ble_profile_b.h/.c      # Profile B implementation
├── CMakeLists.txt          # Build configuration
└── README.md              # This file
```

## Architecture Overview

### `main.c`
- Application entry point
- System initialization (NVS, Bluetooth stack)
- Coordinates initialization of all BLE components

### `ble_config.h`
- Central configuration file
- Contains all UUIDs, constants, and configuration parameters
- Makes it easy to modify settings in one place

### `ble_gap_handler.h/.c`
- Handles GAP (Generic Access Profile) functionality
- Manages advertising data and parameters
- Handles GAP events (advertising start/stop, connection parameters)

### `ble_gatts_server.h/.c`
- Core GATTS server functionality
- Main event dispatcher
- Common utility functions (prepare write handling)
- Server initialization and registration

### `ble_profile_a.h/.c` & `ble_profile_b.h/.c`
- Individual GATT profile implementations
- Each profile manages its own services and characteristics
- Separate event handlers for cleaner code organization

## Key Improvements

1. **Separation of Concerns**: Each file has a specific responsibility
2. **Modularity**: Profiles can be easily added, removed, or modified
3. **Maintainability**: Related functionality is grouped together
4. **Readability**: Smaller, focused files are easier to understand
5. **Configuration Management**: Central config file for easy parameter changes

## Usage

This refactored version maintains the same functionality as the original monolithic file:

- Two GATT profiles (A and B) with services and characteristics
- Support for read, write, notify, and indicate operations
- Proper advertising and connection handling
- Prepare write support for long writes

## Building

Use the standard ESP-IDF build process:

```bash
idf.py build
idf.py flash monitor
```

## Adding New Profiles

To add a new profile:

1. Create `ble_profile_x.h/.c` files
2. Add the profile to `ble_config.h` (APP_ID, UUIDs, etc.)
3. Initialize the profile in `main.c`
4. Update `CMakeLists.txt` to include the new source files

This modular structure makes extending the BLE server much easier than the original monolithic approach.