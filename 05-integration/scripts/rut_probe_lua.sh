#!/bin/bash
set -eu
RUT=root@10.24.121.199
PW='Homaxi@15310'
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
echo "=== LUA PATHS ==="
find /etc /usr/share /www -iname '*lua*' 2>/dev/null | grep -iE 'data.sender|datasender|format' | head -30
echo "=== EXISTING DS LUA ==="
grep -r lua /etc/config/data_sender 2>/dev/null || true
echo "=== UCI data_sender 10-12 ==="
uci show data_sender.10
uci show data_sender.11
uci show data_sender.12
RUTEOF
