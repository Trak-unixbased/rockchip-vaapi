# Installation guide — rockchip-vaapi

## Option A: Install from Debian package (recommended)

```bash
sudo dpkg -i rockchip-vaapi_1.0.1-1_arm64.deb
```

This installs `rockchip_drv_video.so` to
`/usr/lib/aarch64-linux-gnu/dri/` and registers the package with `dpkg`.
To uninstall: `sudo dpkg -r rockchip-vaapi`.

## Option B: Build and install from source

### 1. Install build dependencies

```bash
sudo apt install gcc pkg-config libva-dev librockchip-mpp-dev
```

### 2. Build

```bash
cd /path/to/rockchip-vaapi
make
```

Expected output: `rockchip_drv_video.so` in the project root.

### 3. Install

```bash
sudo make install
# installs to /usr/lib/aarch64-linux-gnu/dri/
```

To install to a custom path:

```bash
sudo install -m 755 rockchip_drv_video.so /your/path/rockchip_drv_video.so
```

## Option C: Build your own Debian package

```bash
# Create orig tarball (exclude build artifacts and debian/)
tar --exclude='rockchip-vaapi/debian' \
    --exclude='rockchip-vaapi/*.so' \
    --exclude='rockchip-vaapi/src/*.o' \
    -czf rockchip-vaapi_1.0.1.orig.tar.gz rockchip-vaapi/

# Build binary package
cd rockchip-vaapi
dpkg-buildpackage -us -uc -b

# Build source package
dpkg-buildpackage -us -uc -S
```

## Verifying the installation

```bash
# Check the driver file is in place
ls -l /usr/lib/aarch64-linux-gnu/dri/rockchip_drv_video.so

# Verify VA-API sees the driver
LIBVA_DRIVER_NAME=rockchip \
LIBVA_DRIVERS_PATH=/usr/lib/aarch64-linux-gnu/dri \
vainfo
```

Expected `vainfo` output:

```
libva info: VA-API version 1.20.0
libva info: User environment variable requested driver 'rockchip'
libva info: Trying to open /usr/lib/aarch64-linux-gnu/dri/rockchip_drv_video.so
libva info: Found init function __vaDriverInit_1_20
...
VA profile VAProfileH264Main               : VAEntrypointVLD
VA profile VAProfileH264High               : VAEntrypointVLD
VA profile VAProfileHEVCMain               : VAEntrypointVLD
...
```

## Configuring Firefox

### Environment variables (required)

| Variable | Value | Reason |
|----------|-------|--------|
| `LIBVA_DRIVER_NAME` | `rockchip` | Selects this driver |
| `LIBVA_DRIVERS_PATH` | `/usr/lib/aarch64-linux-gnu/dri` | Driver search path |
| `MOZ_DISABLE_RDD_SANDBOX=1` | `1` | Allows RDD process to open `/dev/dri` |

### about:config (required)

Open `about:config` in Firefox and set:

| Key | Value |
|-----|-------|
| `media.hardware-video-decoding.enabled` | `true` |
| `media.ffmpeg.vaapi.enabled` | `true` |
| `media.rdd-ffmpeg.enabled` | `true` |

### Permanent launcher

```bash
sudo tee /usr/local/bin/firefox-hw > /dev/null <<'EOF'
#!/bin/sh
export LIBVA_DRIVER_NAME=rockchip
export LIBVA_DRIVERS_PATH=/usr/lib/aarch64-linux-gnu/dri
export MOZ_DISABLE_RDD_SANDBOX=1
exec /usr/bin/firefox "$@"
EOF
sudo chmod +x /usr/local/bin/firefox-hw
```

## Troubleshooting

**`vainfo` reports "driver not found"**
Verify `LIBVA_DRIVER_NAME=rockchip` and that the `.so` exists at
`/usr/lib/aarch64-linux-gnu/dri/rockchip_drv_video.so`.

**Firefox still uses SWDEC**
Check `about:support` → Media → Video Decoder. If it shows `FFmpegVideo`,
hardware decode is active. If it shows `Softpipe` or similar, verify the three
`about:config` keys and that `MOZ_DISABLE_RDD_SANDBOX=1` is set.

**`/dev/dri` permission denied in RDD process**
Add your user to the `video` and `render` groups:
```bash
sudo usermod -aG video,render $USER
```
Then log out and back in (or use `newgrp video`).

**No frames decoded / black screen**
Enable verbose logging:
```bash
LIBVA_DRIVER_NAME=rockchip MOZ_DISABLE_RDD_SANDBOX=1 firefox 2>&1 | grep rk-vaapi
```
Look for errors after `BeginPicture` or `EndPicture`. Missing SPS/PPS or
MPP decode errors will appear there.
