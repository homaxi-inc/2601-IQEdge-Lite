# VRM API v2 · 本地凭据（集成参考）

> **读者**: 007 · 008 · Bob  
> **性质**: 台架 / 研究 **交叉校验** — **非** G2 生产 ingest 路径（生产走 Cerbo Modbus → RUT → MQTT · ADR-012）

---

## 存放位置

| 文件 | 说明 |
|------|------|
| [`../.env`](../.env) | **本地凭据**（gitignored · 已自 `003_IQTrailer/.env` 同步） |
| [`../.env.example`](../.env.example) | 变量名模板（可提交） |
| `003_IQTrailer/.env` | 研究仓 **源**（IQTrailer 实验室） |

**禁止** 将 token 写入 `09-contract/`、Registry、MQTT payload 或提交 Git。

---

## 变量

| 变量 | 用途 |
|------|------|
| `VRM_API_BASE_URL` | `https://vrmapi.victronenergy.com/v2` |
| `VRM_ACCESS_TOKEN` | API token |
| `VRM_USER_ID` | Victron 用户 id（`/users/{idUser}/...`） |
| `VRM_X_AUTHORIZATION` | HTTP 头 `x-authorization: Token …` |
| `VRM_USERS_ME_URL` | 快捷：`GET /users/me` |
| `VRM_USER_INSTALLATIONS_URL` | 快捷：用户安装列表 |

---

## 集成用途（允许）

- 台架 Modbus 读数与 VRM 门户 **对照**（SOC、电压、产量语义）  
- OI-002：确认 system 级 yield 是否与 VRM stats 一致  
- 007/008 脚本开发时加载 `05-integration/.env`（dotenv / `$env:`）

## 集成用途（禁止）

- G2 Timestream / Fleet API **生产 ingest**  
- 在 `05-integration/` 文档中 **粘贴** 明文 token  
- 提交 `.env` 到 GitHub

---

## 预检脚本（007 交接）

```bash
python 05-integration/scripts/vrm_cerbo_preflight.py
```

报告：[`deliveries/VRM_CERBO_PREFLIGHT_007_HANDOFF.md`](deliveries/VRM_CERBO_PREFLIGHT_007_HANDOFF.md)

---

## 加载示例（PowerShell）

```powershell
Get-Content "05-integration\.env" | ForEach-Object {
  if ($_ -match '^\s*([^#=]+)=(.*)$') {
    Set-Item -Path "env:$($matches[1].Trim())" -Value $matches[2].Trim()
  }
}
```

---

*Agent 008 · 2026-05-30 · synced from 003_IQTrailer*
