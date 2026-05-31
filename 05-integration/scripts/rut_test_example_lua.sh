#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
uci set data_sender.30.lua_script='/etc/data_sender/modules/input/lua/example_input_lua.lua'
uci commit data_sender
/etc/init.d/data_sender restart
sleep 2
rm -f /tmp/lua_input_called.txt
ubus call datasender.collection.31 send '{"data":""}'
sleep 1
ubus call datasender.collection.31 dump
RUTEOF
