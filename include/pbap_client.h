#pragma once

#include <cstddef>
#include <cstdint>
#include "esp_err.h"

namespace pbap_client {

using EventCallback = void (*)(const char* message);
using DataCallback = void (*)(const uint8_t* data, size_t length, bool final);

esp_err_t init();
esp_err_t deinit();
esp_err_t connect(const uint8_t* address);
esp_err_t disconnect();
esp_err_t sync_contacts();
bool is_connected();

void register_event_callback(EventCallback cb);
void register_data_callback(DataCallback cb);

} // namespace pbap_client
