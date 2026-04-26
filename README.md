# Lumi Router

This firmware is a replacement for the original firmware for the __Zigbee__ chip JN5169 on __Xiaomi DGNWG05LM__ and __Aqara ZHWG11LM__ gateways which allows to use the gateway as a router (repeater-like) in any Zigbee network instead of the stock coordinator firmware for the propriate Xiaomi MiHome Network.

---

This instruction assumes that an alternative __OpenWRT__ firmware is already installed on the gateway. If you have not done this, use the following instruction [https://openlumi.github.io](https://openlumi.github.io)

## Firmware

1. Connect to device via SSH.
2. Issue the following commands in the command line.

```shell
wget https://github.com/igorlistopad/Lumi-Router-JN5169/releases/latest/download/LumiRouter.bin -P /tmp
jnflash /tmp/LumiRouter.bin
```

## Pairing

Issue the following command in the command line.

```shell
jntool erase_pdm
```

After this the device will automatically join.

## Restart

Issue the following command in the command line.

```shell
jntool soft_reset
```
