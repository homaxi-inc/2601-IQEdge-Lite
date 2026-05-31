#!/bin/bash
set -eu
PW='Homaxi@15310'
RUT=root@10.24.121.199

cat > /tmp/g2_energy_format.lua <<'LUA'
local json = require("luci.jsonc")
local io = require("io")

local function num(v)
  if v == nil then return nil end
  return tonumber(v)
end

local function metric_map(data)
  local m = {}
  if type(data) ~= "table" then return m end
  local rows = data
  if data[1] == nil and data.values then rows = { data } end
  for _, row in ipairs(rows) do
    if type(row) == "table" then
      local name = row.name or row.metric or row.request_name
      local val = row.raw_data or row.value or (row.values and row.values[1])
      if name and val ~= nil then m[name] = num(val) end
    end
  end
  return m
end

function handle_format_request(data)
  local f = io.open("/tmp/g2_format_in.json", "w")
  if f then f:write(json.stringify(data)); f:close() end

  local m = metric_map(data)
  local raw_v = m["Battery_Voltage"] or 0
  local raw_p = m["Battery_Power"] or 0
  local raw_soc = m["Battery_SoC"] or 0
  local raw_pv = m["PV_Power"] or 0
  local voltage_v = raw_v / 10.0
  local current_a = 0
  if voltage_v > 0 then current_a = raw_p / voltage_v end
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
  return json.stringify(payload)
end
LUA

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" "mkdir -p /etc/data_sender/lua"
sshpass -p "$PW" scp -o StrictHostKeyChecking=no /tmp/g2_energy_format.lua "$RUT:/etc/data_sender/lua/g2_energy_format.lua"

sshpass -p "$PW" ssh -o StrictHostKeyChecking=no "$RUT" sh <<'RUTEOF'
uci -q delete data_sender.30
uci -q delete data_sender.31
uci -q delete data_sender.32

uci set data_sender.30=input
uci set data_sender.30.name='g2_modbus_input'
uci set data_sender.30.enabled='1'
uci set data_sender.30.plugin='modbus'
uci set data_sender.30.format='json'
uci set data_sender.30.modbus_filter='all'
uci set data_sender.30.filter_list_modbus_filter_server_ip='1'

uci set data_sender.31=collection
uci set data_sender.31.name='g2_energy_telemetry'
uci set data_sender.31.enabled='1'
uci set data_sender.31.sender_id='3'
uci set data_sender.31.timer='period'
uci set data_sender.31.period='60'
uci set data_sender.31.format='lua'
uci set data_sender.31.lua_format='/etc/data_sender/lua/g2_energy_format.lua'
uci set data_sender.31.input='30'
uci set data_sender.31.output='32'

uci set data_sender.32=output
uci set data_sender.32.name='g2_energy_mqtt_output'
uci set data_sender.32.enabled='1'
uci set data_sender.32.plugin='mqtt'
uci set data_sender.32.mqtt_host='a3vcfgcj3um9l2-ats.iot.us-east-1.amazonaws.com'
uci set data_sender.32.mqtt_port='8883'
uci set data_sender.32.mqtt_topic='iqedge/g2/dev/energy/telemetry'
uci set data_sender.32.mqtt_tls='1'
uci set data_sender.32.mqtt_tls_type='cert'
uci set data_sender.32.mqtt_insecure='0'
uci set data_sender.32.mqtt_use_credentials='0'
uci set data_sender.32.mqtt_keepalive='60'
uci set data_sender.32.mqtt_qos='1'
uci set data_sender.32.mqtt_client_id='RUT241_6004727310_g2'
uci set data_sender.32.mqtt_cafile='/etc/certificates/cbid.data_sender.2.mqtt_cafileAmazonRootCA1.pem'
uci set data_sender.32.mqtt_certfile='/etc/certificates/cbid.data_sender.2.mqtt_certfilecertificate.pem.crt'
uci set data_sender.32.mqtt_keyfile='/etc/certificates/cbid.data_sender.2.mqtt_keyfileprivate.pem.key'
uci set data_sender.32.mqtt_device_files='0'

uci commit data_sender
/etc/init.d/data_sender restart
sleep 3
ubus call datasender.collection.31 send '{"data":""}' 2>/dev/null || true
sleep 2
cat /tmp/g2_format_in.json 2>/dev/null || echo NO_FORMAT_IN
ubus call datasender.collection.31 dump | grep -E 'success|err|mqtt_msg|running'
RUTEOF
