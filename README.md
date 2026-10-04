# Lumi Router

This firmware replaces the stock firmware on the **JN5169 Zigbee chip** in **Xiaomi DGNWG05LM** and **Aqara ZHWG11LM** gateways. Instead of serving as a coordinator for the proprietary Xiaomi Mi Home network, the gateway can operate as a router in any Zigbee network.

These instructions assume **OpenWrt** is already installed on the gateway. If not, follow the [OpenLumi guide](https://openlumi.github.io/).

## Firmware

| Device | Firmware file |
|---|---|
| Xiaomi DGNWG05LM | `LumiRouter-DGNWG05LM.bin` |
| Aqara ZHWG11LM | `LumiRouter-ZHWG11LM.bin` |

**Web interface**

1. Download the firmware file for your device model from [Releases](https://github.com/igorlistopad/Lumi-Router-JN5169/releases).
2. Go to `LuCI -> System -> Zigbee Tools`.
3. Click the `Upload Firmware…` button.
4. Select the downloaded file and click `Upload`.

**Command line**

Connect to the device via SSH and run the commands for your model.

*For Xiaomi:*

```shell
wget https://github.com/igorlistopad/Lumi-Router-JN5169/releases/latest/download/LumiRouter-DGNWG05LM.bin -O /tmp/LumiRouter-DGNWG05LM.bin
jnflash /tmp/LumiRouter-DGNWG05LM.bin
```

*For Aqara:*

```shell
wget https://github.com/igorlistopad/Lumi-Router-JN5169/releases/latest/download/LumiRouter-ZHWG11LM.bin -O /tmp/LumiRouter-ZHWG11LM.bin
jnflash /tmp/LumiRouter-ZHWG11LM.bin
```

## Reset and pairing

To reset the Zigbee settings, perform a PDM erase. After the reset, the device automatically searches for a network open for joining.

**Web interface**

Go to `LuCI -> System -> Zigbee Tools` and click the `Erase PDM` button.

**Command line**

```shell
jntool erase_pdm
```

## Restart

**Web interface**

Go to `LuCI -> System -> Zigbee Tools` and click the `Soft reset` button.

**Command line**

```shell
jntool soft_reset
```

## Building firmware

Use GitHub Codespaces or VS Code Dev Containers for a preconfigured environment,
or follow the local build instructions below.

[![Open in GitHub Codespaces](https://img.shields.io/static/v1?style=for-the-badge&label=GitHub+Codespaces&message=Open&color=lightgrey&logo=github)](https://codespaces.new/igorlistopad/Lumi-Router-JN5169)
[![Open in Dev Container](https://img.shields.io/static/v1?style=for-the-badge&label=Dev%20Containers&message=Open&color=blue)](https://vscode.dev/redirect?url=vscode://ms-vscode-remote.remote-containers/cloneInVolume?url=https://github.com/igorlistopad/Lumi-Router-JN5169)

### Local setup

Supported platforms:

- macOS: AMD64, ARM64
- Linux: AMD64, ARM64
- Windows: AMD64 (MSYS2)

Prerequisites:

- Git, make, and curl
- Python 3.5 or later

Clone the repository and install the SDK and toolchain:

```shell
git clone --recurse-submodules https://github.com/igorlistopad/Lumi-Router-JN5169.git
cd Lumi-Router-JN5169
make install
```

### Build

Build the firmware with `BOARD=DGNWG05LM` for Xiaomi or
`BOARD=ZHWG11LM` for Aqara:

```shell
make BOARD=DGNWG05LM
```

The firmware is generated as `build/LumiRouter-<BOARD>.bin`.

Run `make clean` before switching boards. This also deletes previously
generated firmware files.
