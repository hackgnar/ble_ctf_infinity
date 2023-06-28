FROM espressif/idf:v3.3.1 as dev
RUN mkdir /ble_ctf_infinity
COPY ./code_gen /ble_ctf_infinity/code_gen
COPY ./gatt_servers /ble_ctf_infinity/gatt_servers
COPY ./Makefile /ble_ctf_infinity/
COPY ./sdkconfig.example /ble_ctf_infinity/sdkconfig
ENV IDF_PATH=/opt/esp/idf
ENV IDF_TOOLS_PATH=/opt/esp
RUN . $IDF_PATH/export.sh && cd /ble_ctf_infinity && make codegen && make
CMD ["bash"]
