# msi-gm70-linux-driver

A lightweight userspace daemon to fix mouse tracking for the MSI GM70 Gaming Mouse in wireless mode on Linux.

## Overview
When connected via the 2.4GHz wireless USB dongle, the MSI GM70 firmware exposes multiple interfaces. Standard drivers (`libinput`/`evdev`) bind to a low-frequency endpoint, causing a restricted cursor crawl. 

This driver resolves the issue by opening an exclusive `EVIOCGRAB` handle on the high-frequency raw pointer endpoint (Interface 02), accumulating relative coordinates, applying a configurable float scaling multiplier, and outputting synchronized frames via a virtual `uinput` mouse.

## Installation

```sh
git clone https://github.com/zehl/msi-gm70-linux-driver.git
cd msi-gm70-linux-driver
sudo make install
sudo systemctl enable --now msi-mouse-driver.service
```

Once installed, the build directory can be safely removed.

## Configuration
To adjust the tracking sensitivity, modify the scaling factor variable at the top of `msi_gm70_driver.c`:

```c
#define SCALE_MULTIPLIER 0.75f
```

Recompile and reinstall the service after changing the value:
```sh
sudo make install
sudo systemctl restart msi-mouse-driver.service
```

## Uninstallation
```sh
sudo make uninstall
```

## Compatibility
Tested exclusively on Arch Linux / KDE Wayland.
