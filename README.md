# Lumi Router

This firmware replaces the original firmware for the __Zigbee__ JN5169 chip in __Xiaomi DGNWG05LM__ and __Aqara ZHWG11LM__ gateways. It allows the gateway to operate as a Zigbee router in any Zigbee network instead of using the stock coordinator firmware for the proprietary Xiaomi Mi Home network.

---

These instructions assume that alternative __OpenWrt__ firmware is already installed on the gateway. If it is not, follow the guide at [https://openlumi.github.io](https://openlumi.github.io).

## Firmware

#### Web interface

1. Go to `LuCI -> System -> Zigbee Tools`
2. Click the `Upload Firmware…` button.
3. Select the firmware file to upload.
4. Click the `Upload` button.

#### Command-line

1. Connect to the device via SSH.
2. Run the following commands:

```shell
wget https://github.com/igorlistopad/Lumi-Router-JN5169/releases/latest/download/LumiRouter.bin -P /tmp
jnflash /tmp/LumiRouter.bin
```

## Pairing

#### Web interface

Go to `LuCI -> System -> Zigbee Tools` and click the `Erase PDM` button.

#### Command-line

```shell
jntool erase_pdm
```

After the reset, the device will start the join process automatically.

## Restart

#### Web interface

Go to `LuCI -> System -> Zigbee Tools` and click the `Soft reset` button.

#### Command-line

```shell
jntool soft_reset
```

## Building Firmware

Open this repository in a preconfigured environment using GitHub Codespaces or VS Code Dev Containers. Click one of the buttons below to use either option.

[![Open in GitHub Codespaces](https://img.shields.io/static/v1?style=for-the-badge&label=GitHub+Codespaces&message=Open&color=lightgrey&logo=github)](https://codespaces.new/igorlistopad/Lumi-Router-JN5169)
[![Open in Dev Container](https://img.shields.io/static/v1?style=for-the-badge&label=Dev%20Containers&message=Open&color=blue)](https://vscode.dev/redirect?url=vscode://ms-vscode-remote.remote-containers/cloneInVolume?url=https://github.com/igorlistopad/Lumi-Router-JN5169)

### Linux

Supported architectures: `aarch64` (ARM 64-bit), `amd64` (x86_64)

Prerequisites:
- make
- Python 3.x with the `xmltodict` and `pycryptodome` libraries installed

```bash
git clone https://github.com/igorlistopad/Lumi-Router-JN5169.git
make install
make
```

The firmware binary is located at `build/LumiRouter.bin`.
