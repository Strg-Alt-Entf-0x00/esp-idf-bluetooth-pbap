#include "pbap_client.h"

#include <cstring>
#include <cstdio>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
extern "C" {
#include "esp_pbac_api.h"
#include "esp_pba_defs.h"
}

namespace pbap_client {

namespace {
constexpr char TAG[] = "PBAP";
constexpr char PHONEBOOK_PATH[] = "telecom/pb.vcf";
esp_pbac_conn_hdl_t connection_handle = ESP_PBAC_INVALID_HANDLE;
bool initialized = false;
bool connected = false;
bool connecting = false;
bool sync_in_progress = false;
bool deinitializing = false;
EventCallback event_callback = nullptr;
DataCallback data_callback = nullptr;

void emit_event(const char* message) {
    if (event_callback && message) event_callback(message);
}

void connect_task(void* argument) {
    uint8_t address[6];
    std::memcpy(address, argument, sizeof(address));
    delete[] static_cast<uint8_t*>(argument);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_err_t result = esp_pbac_connect(address);
    connecting = false;
    char status[96];
    std::snprintf(status, sizeof(status),
                  "{\"event\":\"pbap_connect_result\",\"ok\":%s,\"code\":%d}",
                  result == ESP_OK ? "true" : "false", result);
    emit_event(status);
    vTaskDelete(nullptr);
}

}

static void callback(esp_pbac_event_t event, esp_pbac_param_t* param) {
    if (!param) return;

    switch (event) {
    case ESP_PBAC_INIT_EVT:
        initialized = true;
        deinitializing = false;
        break;
    case ESP_PBAC_DEINIT_EVT:
        initialized = false;
        connected = false;
        connecting = false;
        sync_in_progress = false;
        deinitializing = false;
        connection_handle = ESP_PBAC_INVALID_HANDLE;
        break;
    case ESP_PBAC_CONNECTION_STATE_EVT:
        connected = param->conn_stat.connected;
        connection_handle = connected ? param->conn_stat.handle : ESP_PBAC_INVALID_HANDLE;
        ESP_LOGI(TAG, "PBAP %s, reason=%d", connected ? "connected" : "disconnected",
                 param->conn_stat.reason);
        if (!connected) {
            emit_event("{\"event\":\"pbap_disconnected\"}");
        }
        break;
    case ESP_PBAC_PULL_PHONE_BOOK_RESPONSE_EVT: {
        const auto& response = param->pull_phone_book_rsp;
        if (response.result != ESP_PBAC_SUCCESS) {
            sync_in_progress = false;
            char status[96];
            std::snprintf(status, sizeof(status),
                          "{\"event\":\"pbap_error\",\"code\":%d}", response.result);
            emit_event(status);
            break;
        }
        if ((response.data && response.data_len > 0) || response.final) {
            if (data_callback) {
                data_callback(response.data, response.data_len, response.final);
            }
        }
        if (response.final) {
            sync_in_progress = false;
            emit_event("{\"event\":\"pbap_sync_complete\"}");
        }
        break;
    }
    default:
        break;
    }
}

esp_err_t init() {
    if (initialized) return ESP_OK;
    deinitializing = false;
    esp_err_t result = esp_pbac_register_callback(callback);
    if (result != ESP_OK) return result;
    result = esp_pbac_init();
    if (result == ESP_OK) initialized = true;
    return result;
}

esp_err_t deinit() {
    if (!initialized) return ESP_OK;
    deinitializing = true;
    sync_in_progress = false;
    connecting = false;
    const esp_err_t result = esp_pbac_deinit();
    if (result != ESP_OK) {
        deinitializing = false;
        return result;
    }
    initialized = false;
    connected = false;
    connection_handle = ESP_PBAC_INVALID_HANDLE;
    return ESP_OK;
}

esp_err_t connect(const uint8_t* address) {
    if (!initialized || deinitializing) return ESP_ERR_INVALID_STATE;
    if (!address) return ESP_ERR_INVALID_ARG;
    if (connected || connecting) return ESP_OK;
    auto* pending_address = new uint8_t[6];
    std::memcpy(pending_address, address, 6);
    connecting = true;
    if (xTaskCreate(connect_task, "pbap-connect", 3072, pending_address, 4, nullptr) != pdPASS) {
        connecting = false;
        delete[] pending_address;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t disconnect() {
    if (!initialized) return ESP_ERR_INVALID_STATE;
    if (!connected || connection_handle == ESP_PBAC_INVALID_HANDLE) return ESP_OK;
    esp_err_t result = esp_pbac_disconnect(connection_handle);
    connected = false;
    connecting = false;
    connection_handle = ESP_PBAC_INVALID_HANDLE;
    return result;
}

esp_err_t sync_contacts() {
    if (!connected || connection_handle == ESP_PBAC_INVALID_HANDLE)
        return ESP_ERR_INVALID_STATE;
    if (sync_in_progress)
        return ESP_ERR_INVALID_STATE;
    esp_pbac_pull_phone_book_app_param_t params = {};
    params.include_format = 1;
    params.format = 0x01;
    const esp_err_t result = esp_pbac_pull_phone_book(connection_handle, PHONEBOOK_PATH, &params);
    sync_in_progress = result == ESP_OK;
    return result;
}

bool is_connected() {
    return connected;
}

void register_event_callback(EventCallback cb) {
    event_callback = cb;
}

void register_data_callback(DataCallback cb) {
    data_callback = cb;
}

} // namespace pbap_client
