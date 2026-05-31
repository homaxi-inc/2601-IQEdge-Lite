#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199
sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
ubus call datasender.collection.31 dump
ubus call datasender.collection.31 send '{"data":""}'
sleep 3
logread | grep -i data_sender | tail -10
RUTEOF
