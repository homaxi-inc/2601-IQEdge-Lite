#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
cat > /tmp/g2_test_input2.lua <<'LUA'
local io = require("io")
function plugin_init()
  return { { name = "g2_json", type = "STRING" } }
end
function handle_data_request()
  local f = io.open("/tmp/lua_input_called.txt", "a")
  if f then f:write(os.date() .. "\n"); f:close() end
  return { g2_json = '{"test":1}' }
end
LUA
sshpass -p "$PW" scp -o StrictHostKeyChecking=no /tmp/g2_test_input2.lua "$RUT:/etc/data_sender/lua/g2_test_input.lua"
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
/etc/init.d/data_sender restart
sleep 2
ubus call datasender.collection.31 send '{"data":""}'
sleep 1
cat /tmp/lua_input_called.txt 2>/dev/null || echo NOT_CALLED
ubus call datasender.collection.31 dump | grep -A2 '"plugin": "lua"'
RUTEOF
