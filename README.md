# ESP-IDF Bluetooth PBAP

An ESP-IDF Bluetooth Phone Book Access Profile client component for ESP32 Classic Bluetooth.

## Status

This is the initial extracted component release (`0.1.0`). It targets ESP-IDF 5.x and the Bluedroid PBAP APIs.

The component is independent of application UART and PC transport code. Phone-book data and status events are delivered through callbacks.

## Requirements

- ESP-IDF 5.x
- ESP32 target with Bluetooth Classic support
- Bluedroid enabled in the project configuration
- PBAP support enabled by the selected ESP-IDF version

## Integration

Include the public header:

```cpp
#include "pbap_client.h"
```

Register callbacks before connecting:

```cpp
pbap_client::register_event_callback(on_event);
pbap_client::register_data_callback(on_phonebook_data);
pbap_client::init();
pbap_client::connect(remote_address);
```

Phone-book data is delivered in chunks. The `final` argument marks the last chunk of a transfer. See `include/pbap_client.h` for the complete API.

## License

Licensed under the Apache License, Version 2.0. See `LICENSE` for the full license text.
