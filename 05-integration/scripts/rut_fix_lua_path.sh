#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
cp /etc/data_sender/lua/g2_energy_format.lua /etc/data_sender/format.lua
uci set data_sender.31.lua_script='/etc/data_sender/format.lua'
uci delete data_sender.31.lua_format 2>/dev/null || true
uci commit data_sender
/etc/init.d/data_sender restart
sleep 3
ubus list | grep datasender
logread | grep data_sender | tail -5
ubus call datasender.collection.31 send '{"data":""}'
sleep 2
cat /tmp/g2_format_in.json 2>/dev/null | head -c 500 || echo NO_IN
ubus call datasender.collection.31 dump | grep -E 'success|err|mqtt_msg'
RUTEOF
