#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199

cat > /tmp/g2_test_input.lua <<'LUA'
local json = require("luci.jsonc")
local ubus = require("ubus")

function plugin_init()
  return {
    { name = "g2_json", help = "G2 payload JSON", type = "STRING" }
  }
end

local function tag(conn, id)
  local r = conn:call("modbus_client.rpc", "get_tag_value", { id = id, index = 0, count = 1 })
  if r and r.values and r.values[1] then
    return tonumber(r.values[1])
  end
  return nil
end

function handle_data_request()
  local conn = ubus.connect()
  local raw_v = tag(conn, "1.3") or 0
  local raw_p = tag(conn, "1.4") or 0
  local raw_soc = tag(conn, "1.2") or 0
  local raw_pv = tag(conn, "1.5") or 0
  local voltage_v = raw_v / 10.0
  local current_a = 0
  if voltage_v > 0 then
    current_a = raw_p / voltage_v
  end
  local load_w = raw_pv - raw_p
  if load_w < 0 then load_w = 0 end
  local payload = {
    schema_version = "energy.telemetry.v1",
    sys_id = "IQ-26-60001",
    component_id = "c0619ab6be37",
    component_role = "cerbo",
    domain = "energy",
    system_type = "iqtrailer",
    timestamp = os.date("!%Y-%m-%dT%H:%M:%SZ"),
    ingest_mode = "live",
    status = "running",
    state = "NORMAL",
    data_stale = false,
    reporting_mode = "NORMAL",
    measures = {
      battery = {
        voltage_v = voltage_v,
        current_a = current_a,
        soc_pct = raw_soc
      },
      solar = { power_w = raw_pv },
      load = { power_w = load_w, status = load_w > 0 and "ON" or "OFF" }
    }
  }
  return { g2_json = json.stringify(payload) }
end
LUA

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" "mkdir -p /etc/data_sender/lua"
sshpass -p "$PW" scp -o StrictHostKeyChecking=no /tmp/g2_test_input.lua "$RUT:/etc/data_sender/lua/g2_test_input.lua"

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
# cleanup prior debug
uci -q delete data_sender.97; uci -q delete data_sender.98; uci -q delete data_sender.99

uci set data_sender.30=input
uci set data_sender.30.name='g2_lua_input'
uci set data_sender.30.enabled='1'
uci set data_sender.30.plugin='lua'
uci set data_sender.30.format='json'
uci set data_sender.30.lua_script='/etc/data_sender/lua/g2_test_input.lua'

uci set data_sender.31=collection
uci set data_sender.31.name='g2_test_collection'
uci set data_sender.31.enabled='1'
uci set data_sender.31.sender_id='3'
uci set data_sender.31.timer='period'
uci set data_sender.31.period='300'
uci set data_sender.31.format='custom'
uci set data_sender.31.format_str='%g2_json%'
uci set data_sender.31.input='30'
uci set data_sender.31.output='32'

uci set data_sender.32=output
uci set data_sender.32.name='g2_test_mqtt'
uci set data_sender.32.enabled='0'
uci set data_sender.32.plugin='mqtt'
uci set data_sender.32.mqtt_host='a3vcfgcj3um9l2-ats.iot.us-east-1.amazonaws.com'
uci set data_sender.32.mqtt_port='8883'
uci set data_sender.32.mqtt_topic='iqedge/g2/dev/energy/telemetry'
uci set data_sender.32.mqtt_tls='1'
uci set data_sender.32.mqtt_tls_type='cert'
uci set data_sender.32.mqtt_cafile='/etc/certificates/cbid.data_sender.2.mqtt_cafileAmazonRootCA1.pem'
uci set data_sender.32.mqtt_certfile='/etc/certificates/cbid.data_sender.2.mqtt_certfilecertificate.pem.crt'
uci set data_sender.32.mqtt_keyfile='/etc/certificates/cbid.data_sender.2.mqtt_keyfileprivate.pem.key'
uci set data_sender.32.mqtt_client_id='RUT241_6004727310_g2test'
uci set data_sender.32.mqtt_qos='1'

uci commit data_sender
/etc/init.d/data_sender restart
sleep 3
logread | grep -i data_sender | tail -15
ubus list | grep datasender
RUTEOF
