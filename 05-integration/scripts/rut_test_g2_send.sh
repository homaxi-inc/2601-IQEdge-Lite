#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
uci set data_sender.32.enabled='1'
uci commit data_sender
/etc/init.d/data_sender restart
sleep 3
ubus call datasender.collection.31 send
sleep 5
logread | grep -iE 'data_sender|mqtt|g2' | tail -20
uci get data_sender.32.mqtt_msg_count 2>/dev/null || true
RUTEOF
