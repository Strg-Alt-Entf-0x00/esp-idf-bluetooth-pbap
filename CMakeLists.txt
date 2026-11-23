idf_component_register(
    SRCS "src/pbap_client.cpp"
    INCLUDE_DIRS "include"
    REQUIRES bt freertos esp_system
)
