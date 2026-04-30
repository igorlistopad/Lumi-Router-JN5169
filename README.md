# Lumi Router

This firmware is a replacement for the original firmware for the __Zigbee__ chip JN5169 on __Xiaomi DGNWG05LM__ and __Aqara ZHWG11LM__ gateways which allows to use the gateway as a router (repeater-like) in any Zigbee network instead of the stock coordinator firmware for the propriate Xiaomi MiHome Network.

---

This instruction assumes that an alternative __OpenWRT__ firmware is already installed on the gateway. If you have not done this, use the following instruction [https://openlumi.github.io](https://openlumi.github.io)

## Firmware

#### Web interface

1. Go to `LuCI -> System -> Zigbee Tools`
2. Click the `Upload Firmware…` button.
3. Select the firmware file to upload.
4. Click the `Upload` button.

#### Command-line

1. Connect to device via SSH.
2. Issue the following commands.

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

After this the device will automatically join.

## Restart

#### Web interface

Go to `LuCI -> System -> Zigbee Tools` and click the `Soft reset` button.

#### Command-line

```shell
jntool soft_reset
```

## Building Firmware

You can open this repo with the configured environment using GitHub Codespaces or VS Code Dev Containers. Click on one of the buttons below to open this repo in one of those options.

[![Open in GitHub Codespaces](https://img.shields.io/static/v1?style=for-the-badge&label=GitHub+Codespaces&message=Open&color=lightgrey&logo=github)](https://codespaces.new/igorlistopad/Lumi-Router-JN5169)
[![Open in Dev Container](https://img.shields.io/static/v1?style=for-the-badge&label=Dev%20Containers&message=Open&color=blue)](https://vscode.dev/redirect?url=vscode://ms-vscode-remote.remote-containers/cloneInVolume?url=https://github.com/igorlistopad/Lumi-Router-JN5169)

### Linux

Supported architectures: `aarch64` (ARM 64-bit), `amd64` (x86_64)

Prerequisites:
 - make
 - Python 3.x with `xmltodict` and `pycryptodome` libraries installed

```bash
git clone https://github.com/igorlistopad/Lumi-Router-JN5169.git
make install
make
```

Firmware binary is located at `build/LumiRouter.bin`
