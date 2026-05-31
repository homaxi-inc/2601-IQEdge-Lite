#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
find /usr/lib /usr/bin -iname '*data*sender*' 2>/dev/null | head -20
ls /usr/lib/data-sender 2>/dev/null || ls /usr/lib/datasender 2>/dev/null || true
logread | grep -i datasender | tail -5
RUTEOF
