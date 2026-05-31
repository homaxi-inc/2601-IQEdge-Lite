# RUT241 VPN 远程访问（007 现场集成）

> **读者**: 007 · Bob  
> **性质**: IQTrailer / Cerbo 台架与现网 RUT **远程 SSH 诊断** — 经 AWS WireGuard 跳板，**非** G2 MQTT ingest 路径  
> **权威 VPN 配置**: `002_IQEdge/vpn/`（Peer 账本 · wg0 · 生产 SOP）

---

## 拓扑

```text
本机 (PowerShell)
    │  ssh -i iqwatch-vpn-key.pem
    ▼
AWS EC2 VPN 跳板  52.5.62.66  (WireGuard wg0 · 10.24.121.1/24)
    │  WireGuard 隧道
    ▼
RUT241 VPN IP     10.24.121.x/32  (例: 10.24.121.199)
    │  LAN 192.168.20.0/24
    ▼
Cerbo / IPC / 其他下挂设备
```

与 **IQWatch  fleet** 共用同一 VPN 网关；IQTrailer 测试机 `Homaxi-test-6004727310` 使用 **`10.24.121.199/32`**（见 `002_IQEdge/vpn/wg0_conf.md`）。

---

## 前置条件

| 项 | 说明 |
|----|------|
| SSH 私钥 | `C:\AWS_Cloud\iqwatch-vpn-key.pem`（EC2 跳板 · 与 IQWatch VPN 运维一致） |
| 网络 | 本机可 SSH 至 `52.5.62.66:22` |
| RUT root 密码 | 见 `002_IQEdge/vpn/` SOP · **勿写入 Git** |
| Peer 已 provision | `002_IQEdge/vpn/wg0_conf.md` 中目标 VPN IP 与 PublicKey 已写入 EC2 `/etc/wireguard/wg0.conf` |

---

## 标准工作流（三步）

### 1) 检查 VPN 隧道（EC2 端）

```powershell
ssh -i "C:\AWS_Cloud\iqwatch-vpn-key.pem" -o StrictHostKeyChecking=no ubuntu@52.5.62.66 `
  "sudo wg show wg0 | grep -A 6 '10.24.121.199'"
```

**通过标准**: `latest handshake` < 3 分钟 · `transfer` 有收发计数。

### 2) Ping 目标 RUT

```powershell
ssh -i "C:\AWS_Cloud\iqwatch-vpn-key.pem" -o StrictHostKeyChecking=no ubuntu@52.5.62.66 `
  "ping -c 3 -W 2 10.24.121.199"
```

**通过标准**: 0% packet loss（蜂窝 RTT 150–300 ms 属正常）。

### 3) SSH 登录 RUT（经跳板）

```powershell
ssh -i "C:\AWS_Cloud\iqwatch-vpn-key.pem" -o StrictHostKeyChecking=no ubuntu@52.5.62.66 `
  "sshpass -p '<RUT_ROOT_PASSWORD>' ssh -o StrictHostKeyChecking=no root@10.24.121.199 <command>"
```

常用诊断命令（`<command>` 示例）：

```bash
wg show wg1
cat /tmp/dhcp.leases
ubus call system board
```

> EC2 已预装 `sshpass`。若需交互式 shell，先 SSH 到 EC2，再 `ssh root@10.24.121.199`。

---

## 2026-05-30 实连记录 · `10.24.121.199`

| 字段 | 值 |
|------|-----|
| 台账标识 | `Homaxi-test-6004727310` |
| RUT SN | `6004727310` |
| VPN IP | `10.24.121.199/32` |
| LAN MAC | `20:97:27:60:59:fd` |
| 型号 | Teltonika RUT2M (RUT241) · OpenWrt 21.02.0 |
| WireGuard 实例 | `wg1` · PublicKey `/80Ux28UQ2iTgL7kW3MdyYuq3dj+OZdR9LQTHPd5/HI=` |
| Endpoint | `52.5.62.66:51820` · Keepalive 25s |

**Layer 1 结果（007 会话验证）**

| 检查 | 结果 |
|------|------|
| EC2 `wg show` handshake | ✅ ~1–2 min |
| Ping EC2 → 10.24.121.199 | ✅ 0% loss · RTT 153–282 ms |
| SSH root@10.24.121.199 | ✅ |
| RUT `wg show wg1` | ✅ handshake ~21s · transfer 正常 |

**LAN DHCP（实连快照）**

| IP | MAC | 主机名 |
|----|-----|--------|
| 192.168.20.236 | c0:61:9a:b6:be:37 | einstein |
| 192.168.20.184 | 00:46:b8:13:87:46 | IPC |

**用途**: IQTrailer Cerbo Modbus 台架 · EDGE-T2 RUT Data to Server 联调前的远程可达性基线。

---

## 故障排查（简表）

| 现象 | 可能原因 | 动作 |
|------|----------|------|
| EC2 SSH 失败 | 本机网络 / 密钥路径 | 确认 `iqwatch-vpn-key.pem` 与 52.5.62.66 可达 |
| `wg show` 无 handshake | RUT 端 WireGuard 未启 / 蜂窝差 | RMS 重启 RUT 或检查 wg1 UCI · 对照 `002_IQEdge/vpn/SKILL.md` |
| Ping 超时但 MQTT 仍上报 | Split-brain（出站通、入站 VPN 断） | 检查 EC2 Peer 块是否与 RUT PublicKey 一致 |
| SSH 拒绝 | 密码错误或 root 登录关闭 | 核对 SOP 密码 · Web UI `System → Administration` |

---

## 相关文档

| 文档 | 路径 |
|------|------|
| WireGuard 生产配置指南 | `002_IQEdge/vpn/RUT241-VPN-Configuration-Guide-for-Production-Engineers.md` |
| EC2 wg0 Peer 账本 | `002_IQEdge/vpn/wg0_conf.md` |
| VPN 审计 SKILL | `002_IQEdge/vpn/SKILL.md` |
| RUT Modbus Client 摘要 | [`../cerbo/docs/G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md`](../cerbo/docs/G2_Cerbo_Modbus_Register_and_RUT_Client_Summary.md) |

---

*Agent 007 · 2026-05-30 · first successful VPN session to Homaxi-test RUT for IQTrailer integration*
