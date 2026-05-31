#!/bin/bash
# Run on EC2 jump host — single SSH session into RUT
set -eu
RUT=root@10.24.121.199
PW='Homaxi@15310'

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
echo "=== CERBO CONNECTIVITY ==="
ping -c 2 192.168.20.236
wget -q -O /dev/null -T 3 http://192.168.20.236/ && echo "HTTP:80 OPEN" || echo "HTTP:80 FAIL"
echo | nc 192.168.20.236 502 && echo "TCP:502 OPEN" || echo "TCP:502 FAIL"

echo "=== MODBUS TAG VALUES (cached) ==="
for id in 1.2 1.3 1.4 1.5 1.6 1.7; do
  echo "TAG $id"
  ubus call modbus_client.rpc get_tag_value "{\"id\":\"$id\",\"index\":0,\"count\":1}"
done

echo "=== MODBUS TCP LIVE TEST (SOC reg 844) ==="
ubus call modbus_client.rpc tcp.test '{"id":1,"timeout":5,"function":3,"first_reg":844,"reg_count":"1","data_type":"16bit_uint_hi_first","no_brackets":1,"broadcast":0,"ip":"192.168.20.236","port":"502","delay":0}'

echo "=== MODBUS UCI SUMMARY ==="
uci show modbus_client | grep -E "enabled|name|dev_ipaddr|server_id|period|first_reg|data_type"

echo "=== DATA TO SERVER UCI SUMMARY ==="
uci show data_sender | grep -E "enabled|name|plugin|mqtt_|period|format|collection|topic|host|client_id"

echo "=== SERVICES ==="
/etc/init.d/modbus_client status
/etc/init.d/data_sender status

echo "=== FIRMWARE ==="
cat /etc/version
ubus call system board

echo "=== GSM ==="
gsmctl -q
gsmctl -E
RUTEOF
