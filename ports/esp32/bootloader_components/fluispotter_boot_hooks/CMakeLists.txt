idf_component_register(SRCS "hooks.c")
# Force the weak bootloader hook symbol to resolve to our object.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u bootloader_before_init")
