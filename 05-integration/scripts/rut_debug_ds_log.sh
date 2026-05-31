#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
echo "=== UCI 98 ==="
uci show data_sender.98
echo "=== LOG ==="
logread | grep -iE 'data.sender|datasender|lua|error' | tail -30
echo "=== UBUS ==="
ubus list | grep datasender
ubus -v list datasender.collection.98 2>/dev/null || true
RUTEOF
