#!/bin/bash
# EDGE-T2: modbus 60s, G2 MQTT, disable iqcloud legacy, keep iot/rut241/status
set -eu
RUT=root@10.24.121.199
PW='Homaxi@15310'
LUA_FILE="${1:-/tmp/g2_energy_format.lua}"

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" "mkdir -p /etc/data_sender/lua"
sshpass -p "$PW" scp -o StrictHostKeyChecking=no \
  "$LUA_FILE" \
  "$RUT:/etc/data_sender/lua/g2_energy_format.lua"

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
set -eu
cp /etc/data_sender/lua/g2_energy_format.lua /etc/data_sender/format.lua

# Modbus poll 300 -> 60 s
uci set modbus_client.1.period='60'
uci commit modbus_client

# Legacy router status: keep, align period 60 s
uci set data_sender.1.period='60'

# Remove iqcloud/energy_telemetry (collection 10 + output 11 + input 12)
uci set data_sender.10.enabled='0'
uci set data_sender.11.enabled='0'
uci set data_sender.12.enabled='0'

# G2 modbus input
uci set data_sender.30=input
uci set data_sender.30.name='g2_modbus_input'
uci set data_sender.30.enabled='1'
uci set data_sender.30.plugin='modbus'
uci set data_sender.30.format='json'
uci set data_sender.30.modbus_filter='all'
uci set data_sender.30.filter_list_modbus_filter_server_ip='1'
uci set data_sender.30.modbus_segments='50'
uci set data_sender.30.na_str='N/A'

# G2 collection -> MQTT
uci set data_sender.31=collection
uci set data_sender.31.name='g2_energy_telemetry'
uci set data_sender.31.enabled='1'
uci set data_sender.31.sender_id='3'
uci set data_sender.31.timer='period'
uci set data_sender.31.period='60'
uci set data_sender.31.format='lua'
uci set data_sender.31.lua_script='/etc/data_sender/format.lua'
uci delete data_sender.31.lua_format 2>/dev/null || true
uci set data_sender.31.input='30'
uci set data_sender.31.output='32'

uci set data_sender.32=output
uci set data_sender.32.name='g2_energy_mqtt_output'
uci set data_sender.32.enabled='1'
uci set data_sender.32.plugin='mqtt'
uci set data_sender.32.mqtt_host='a3vcfgcj3um9l2-ats.iot.us-east-1.amazonaws.com'
uci set data_sender.32.mqtt_port='8883'
uci set data_sender.32.mqtt_topic='iqedge/g2/dev/energy/telemetry'
uci set data_sender.32.mqtt_tls='1'
uci set data_sender.32.mqtt_tls_type='cert'
uci set data_sender.32.mqtt_insecure='0'
uci set data_sender.32.mqtt_use_credentials='0'
uci set data_sender.32.mqtt_keepalive='60'
uci set data_sender.32.mqtt_qos='1'
uci set data_sender.32.mqtt_client_id='RUT241_6004727310_g2'
uci set data_sender.32.mqtt_cafile='/etc/certificates/cbid.data_sender.2.mqtt_cafileAmazonRootCA1.pem'
uci set data_sender.32.mqtt_certfile='/etc/certificates/cbid.data_sender.2.mqtt_certfilecertificate.pem.crt'
uci set data_sender.32.mqtt_keyfile='/etc/certificates/cbid.data_sender.2.mqtt_keyfileprivate.pem.key'
uci set data_sender.32.mqtt_device_files='0'

uci commit data_sender
uci commit modbus_client

/etc/init.d/modbus_client restart
/etc/init.d/data_sender restart
sleep 5

echo "=== VERIFY ==="
echo "modbus period: $(uci get modbus_client.1.period)"
echo "awsmqtt period: $(uci get data_sender.1.period)"
echo "iqcloud enabled: $(uci get data_sender.10.enabled) $(uci get data_sender.11.enabled)"
echo "g2 enabled: $(uci get data_sender.31.enabled)"
echo "g2 topic: $(uci get data_sender.32.mqtt_topic)"
ubus call datasender.collection.31 send '{"data":""}'
sleep 3
ubus call datasender.collection.31 dump | grep -E '"success"|"err"|mqtt_topic|period'
RUTEOF
