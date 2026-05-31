#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
ubus -v list datasender.collection.31
/etc/init.d/data_sender status
ps | grep data_sender
RUTEOF
