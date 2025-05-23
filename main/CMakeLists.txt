set(srcs "main.cpp"
        )

idf_component_register(SRCS "mqttclient.cpp" "configmanager.cpp" "wifimanager.cpp" "taskmanager.cpp" "timerevent.cpp" "clockmanager.cpp" "cmdmanager.cpp" "blemanager.cpp" "${srcs}"
                       INCLUDE_DIRS ".")
