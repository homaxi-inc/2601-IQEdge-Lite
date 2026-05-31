-- G2 energy.telemetry.v1 formatter for RutOS Data to Server (collection format = lua)
-- Input: modbus plugin, format json (see /tmp/g2_format_in.json shape on device)
local json = require("luci.jsonc")

local SYS_ID = "IQ-26-60001"
local COMPONENT_ID = "c0619ab6be37"

local function parse_rows(data)
  local rows = {}
  if type(data) ~= "table" then
    return rows
  end
  for _, v in pairs(data) do
    if type(v) == "string" then
      local ok, arr = pcall(json.parse, v)
      if ok and type(arr) == "table" then
        for _, row in ipairs(arr) do
          rows[#rows + 1] = row
        end
      end
    elseif type(v) == "table" then
      for _, row in ipairs(v) do
        rows[#rows + 1] = row
      end
    end
  end
  return rows
end

local function metric_map(data)
  local m = {}
  for _, row in ipairs(parse_rows(data)) do
    local name = row.name
    local val = tonumber(row.data)
    if name and val ~= nil then
      m[name] = val
    end
  end
  return m
end

function handle_format_request(data)
  local m = metric_map(data)
  local raw_v = m["Battery_Voltage"] or 0
  local raw_p = m["Battery_Power"] or 0
  local raw_soc = m["Battery_SoC"] or 0
  local raw_pv = m["PV_Power"] or 0
  local voltage_v = raw_v / 10.0
  local current_a = 0
  if voltage_v > 0 then
    current_a = raw_p / voltage_v
  end
  local load_w = raw_pv - raw_p
  if load_w < 0 then
    load_w = 0
  end

  local payload = {
    schema_version = "energy.telemetry.v1",
    sys_id = SYS_ID,
    component_id = COMPONENT_ID,
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
        soc_pct = raw_soc,
      },
      solar = { power_w = raw_pv },
      load = { power_w = load_w, status = load_w > 0 and "ON" or "OFF" },
    },
  }
  return json.stringify(payload)
end
