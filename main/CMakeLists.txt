idf_component_register(SRCS "wii_remote_host.c" "wii_mitm.c" "main.c" "wii_controller.c" "fake_wii_remote.c"
                    INCLUDE_DIRS ""
                    REQUIRES "bthci" "nvs_flash" "bt")