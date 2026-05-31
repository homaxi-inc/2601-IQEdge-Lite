#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199

cat > /tmp/g2_debug_format.lua <<'LUA'
local json = require("luci.jsonc")
local io = require("io")
function handle_format_request(data)
  local f = io.open("/tmp/g2_format_debug.json", "w")
  if f then f:write(json.stringify(data)); f:close() end
  return json.stringify(data)
end
LUA

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" "mkdir -p /etc/data_sender/lua"
sshpass -p "$PW" scp -o StrictHostKeyChecking=no /tmp/g2_debug_format.lua "$RUT:/etc/data_sender/lua/g2_debug_format.lua"

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
mkdir -p /etc/data_sender/lua
uci set data_sender.99=input
uci set data_sender.99.name='g2_debug_modbus'
uci set data_sender.99.enabled='1'
uci set data_sender.99.plugin='modbus'
uci set data_sender.99.format='json'
uci set data_sender.99.modbus_filter='all'
uci set data_sender.99.filter_list_modbus_filter_server_ip='1'
uci set data_sender.98=collection
uci set data_sender.98.name='g2_debug_collection'
uci set data_sender.98.enabled='1'
uci set data_sender.98.sender_id='99'
uci set data_sender.98.timer='period'
uci set data_sender.98.period='300'
uci set data_sender.98.format='lua'
uci set data_sender.98.input='99'
uci set data_sender.98.output='97'
uci set data_sender.98.lua_format='/etc/data_sender/lua/g2_debug_format.lua'
uci set data_sender.97=output
uci set data_sender.97.name='g2_debug_output'
uci set data_sender.97.enabled='0'
uci set data_sender.97.plugin='base'
uci commit data_sender
/etc/init.d/data_sender restart
sleep 2
ubus call datasender.collection.98 send 2>/dev/null || true
sleep 2
cat /tmp/g2_format_debug.json 2>/dev/null || echo NO_DEBUG_FILE
RUTEOF
