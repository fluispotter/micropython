idf_component_register(SRCS "hooks.c"
                       PRIV_REQUIRES hal soc esp_rom)
# Force the weak bootloader hook symbol to resolve to our object.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u bootloader_before_init")
