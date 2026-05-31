#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
# Remove broken debug sections
uci -q delete data_sender.97
uci -q delete data_sender.98
uci -q delete data_sender.99
uci commit data_sender
/etc/init.d/data_sender restart
sleep 2
/etc/init.d/data_sender status
ubus list | grep datasender
echo "=== grep lua in running config ==="
grep lua /etc/config/data_sender || true
cat /etc/config/data_sender | head -120
RUTEOF
