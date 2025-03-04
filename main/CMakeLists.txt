idf_component_register(
    SRCS "main.c"
         "src/button.c"
         "src/boot.c"
         "src/wifi_link.c"
         "src/stm_link.c"
         "src/link.c"
         "src/camera_server.c"
         "src/loco.c"
         "libdw1000/src/libdw1000.c"
         "libdw1000/src/libdw1000Spi.c"
    INCLUDE_DIRS "include"
                 "libdw1000/include")
