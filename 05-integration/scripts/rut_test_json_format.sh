#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
uci set data_sender.31.format='json'
uci delete data_sender.31.format_str 2>/dev/null || true
uci commit data_sender
/etc/init.d/data_sender restart
sleep 3
ubus call datasender.collection.31 send '{"data":""}'
sleep 5
ubus call datasender.collection.31 dump | grep -E 'success|err|mqtt_msg'
logread | grep -i data_sender | tail -5
RUTEOF
