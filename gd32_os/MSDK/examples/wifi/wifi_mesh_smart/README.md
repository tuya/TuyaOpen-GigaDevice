# WiFi Mesh-Smart Example

This example demonstrates the self-organizing Mesh network feature (WiFi Mesh-Smart) on GD32VW55x. Each node automatically scans for nearby Mesh networks on startup, joins the network at the appropriate level, and opens its own SoftAP for downstream nodes to connect, forming a tree-shaped Mesh network.

---

## Features

| Feature | Description |
|---------|-------------|
| Node roles | ROOT / ROUTER / LEAF, negotiated automatically |
| Maximum levels | 4 by default (`CONFIG_MESH_SMART_MAX_LEVEL`) |
| Max clients per node | `MAX_STA_NUM - 1` |
| SoftAP SSID / Password | `GD-MeshSmartSoftAP` / `12345678` (configurable) |
| Network identifier OUI | `76:BA:ED` (vendor_id + network_id, configurable) |
| Node discovery | Beacon Vendor IE carries node status, level, and load information |
| IP segment conflict avoidance | Automatically adjusts when STA and SoftAP share the same subnet |
| Reconnection | Exponential backoff, max interval 10 s, max 4 retries |
| Health monitoring | Checks upstream node Vendor IE every second; restarts if level changes |
| LED indication | GPIOB11/12/13 (RGB) indicates the current node level |
| Configuration persistence | Config and Root AP credentials stored in NVDS Flash |

---

## Node State Machine

```
INIT ──► SCAN ──► STA_CONNECTING ──► STA_CONNECTED_SOFTAP_NOT_STARTED
                    │                          │
                    │ (no node found)           ▼
                    │               STA_CONNECTED_SOFTAP_STARTED
                    │
                    ▼
              SOFTAP_PROVISIONING (waiting for Root AP configuration)
                    │
                    ▼
              STA_CONNECTING (connecting to Root AP)
```

---

## Getting Started

### 1. Build Configuration

Ensure the following macro is enabled in the project configuration (`wlan_config.h` / CMake):

```c
#define CONFIG_WIFI_MESH_SMART   1
```

### 2. Modify Default Parameters (Optional)

Edit `MSDK/wifi_manager/wifi_mesh_smart/wifi_mesh_smart_config.c`:

```c
#define DEFAULT_MESH_SMART_SOFTAP_SSID      "GD-MeshSmartSoftAP"  // Internal Mesh SoftAP SSID
#define DEFAULT_MESH_SMART_SOFTAP_PASSWORD  "12345678"             // Internal Mesh SoftAP password
#define DEFAULT_VENDOR_IE_OUI_0             0x76                   // Network identifier byte 0
#define DEFAULT_VENDOR_IE_OUI_1             0xBA                   // Network identifier byte 1
#define DEFAULT_MESH_SMART_NETWORK_ID       0xED                   // Network ID (byte 3 of OUI)
#define CONFIG_MESH_SMART_MAX_LEVEL         4                      // Maximum mesh level depth
```

### 3. Configure the Root AP (First Boot)

On first boot, if no Root AP credentials are stored in Flash, the node enters SoftAP Provisioning mode (SSID: `GD-MeshSmartSoftAP`).

Connect to that SoftAP and call the configuration API to set the upstream router credentials:

```c
// Call via AT command or application code
wifi_mesh_smart_config_rootap_info("your_router_ssid", "your_router_password");
```

After the credentials are written to Flash, the node automatically switches to ROOT role and connects to the specified router.

### 4. Joining Additional Nodes

Other nodes (ROUTER/LEAF) automatically scan for the identified Mesh SoftAP on startup. Once a node with status `STA_CONNECTED_SOFTAP_STARTED` and available capacity is found, the new node joins it automatically — no manual intervention required.

---

## API Reference

| Function | Description |
|----------|-------------|
| `wifi_mesh_smart_network_init()` | Start Mesh-Smart (called from `application_init`; internally waits for WiFi ready asynchronously) |
| `wifi_mesh_smart_config_rootap_info(ssid, pwd)` | Configure Root AP credentials and trigger connection (used during initial provisioning) |
| `wifi_mesh_smart_status_print()` | Print current node role, level, ID, and status |
| `wifi_mesh_smart_softap_stop()` | Stop the node's SoftAP |

---

## LED Level Indication (GPIOB 11/12/13)

| Level | RED | GREEN | BLUE |
|-------|-----|-------|------|
| 1 (ROOT) | ● | ● | ● |
| 2 | ● | ○ | ○ |
| 3 | ○ | ● | ○ |
| 4 | ○ | ○ | ● |
| 5 | ○ | ● | ● |
| Not connected | ○ | ○ | ○ |

---

## Building the Project

This example is located under `MSDK/examples/wifi/wifi_mesh_smart` and must not be moved to another directory, as the project paths are hard-coded.

### Using GD32 Embedded Builder
1. Launch `Embedded Builder.exe` and select `MSDK/examples/wifi` as the workspace.
2. Import the project from `MSDK/examples/wifi/wifi_mesh_smart/Eclipse_project`.
3. Configure the ToolChain and Build Tool.
4. Build the project; the image will be generated in `MSDK/examples/wifi/wifi_mesh_smart/image`.
5. Download the image using GDLINK/JLINK or by dragging it onto the USB drive.

For detailed build instructions, refer to *AN154 GD32VW553 Quick Development Guide.docx*.

---

## Notes

- All Mesh nodes must use **identical** SoftAP SSID/password and OUI network identifier; otherwise nodes will not recognise each other.
- Root AP credentials are persisted in NVDS Flash and survive power cycles. To re-provision, erase the NVDS partition.
- The recommended maximum number of nodes in a single network is `MAX_STA_NUM ^ level_count`. Nodes that exceed the maximum level limit are automatically demoted to LEAF role (no longer forward traffic).
