#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
ubus call datasender.collection.10 send '{"data":""}'
sleep 3
ubus call datasender.collection.10 dump | grep -E 'success|err|mqtt_msg'
RUTEOF
