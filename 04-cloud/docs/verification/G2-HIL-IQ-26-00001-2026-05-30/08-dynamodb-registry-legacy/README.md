# 08 — DynamoDB · Registry + Legacy

---

## A. G2 Registry

**表**: `iqedge-g2-dev-table-registry`  
**键**: `sys_id` = `IQ-26-00001`

### 核对项

- [ ] `track` = `g2`
- [ ] `aliases.mppt_serial` = `HQ2513A69PJ`
- [ ] `firmware_version` 接近 `v2.3.003`

**证据**: `registry-item.json` *(待添加)*

---

## B. Legacy DeviceLatestStatus

**表**: `DeviceLatestStatus`  
**键**: `deviceId` = `HQ2513A69PJ`

### 核对项

- [ ] `firmware_version` = `v2.3.003`
- [ ] `soc` / `solar_power` 与 G2 Timestream 量级一致

**证据**: `legacy-item.json` *(待添加)*

---

## 截图（可选）

- `screenshot-registry.png`
- `screenshot-legacy.png`
