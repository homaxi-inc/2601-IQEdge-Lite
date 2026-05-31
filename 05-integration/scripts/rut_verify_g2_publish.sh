#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
grep SYS_ID /etc/data_sender/format.lua
/etc/init.d/data_sender status
ubus call datasender.collection.31 send '{"data":""}'
sleep 65
ubus call datasender.collection.31 dump | grep -E 'success|err|mqtt_topic|running'
logread | grep -iE 'data_sender|mqtt' | tail -15
RUTEOF
