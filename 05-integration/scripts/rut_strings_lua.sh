#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
strings /usr/sbin/data_sender 2>/dev/null | grep -i lua | head -30
grep -r "plugin.*lua" /etc/uci-defaults 2>/dev/null | head -5
uci show data_sender.30
RUTEOF
