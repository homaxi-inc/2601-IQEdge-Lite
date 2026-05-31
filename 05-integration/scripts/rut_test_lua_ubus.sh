#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
which lua lua5.1 luac 2>/dev/null || true
lua -e 'print("lua ok")' 2>&1 || true
lua -e 'local ok,ubus=pcall(require,"ubus"); print("ubus",ok); if ok then local c=ubus.connect(); local r=c:call("modbus_client.rpc","get_tag_value",{id="1.2",index=0,count=1}); print(require("luci.jsonc").stringify(r)) end' 2>&1 || true
RUTEOF
