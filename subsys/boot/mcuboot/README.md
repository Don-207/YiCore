# MCUboot integration

This directory is YiCore's porting layer for the unmodified MCUboot sources in
`third_party/mcuboot`. It currently provides the flash-map API required by
MCUboot for a single updatable image.

A bootloader application must initialize YiCore devices, declare its board
partitions, and register its flash map before calling `boot_go()`:

```c
static const yi_partition_t slot0 = {
    .name = "image-0",
    .flash = YI_DT_GET(FLASH0),
    .offset = 0x00008000,
    .size = 0x00018000,
};

static const struct flash_area areas[] = {
    {
        .fa_id = FLASH_AREA_IMAGE_PRIMARY(0),
        .fa_device_id = 0,
        .fa_off = 0x00008000,
        .fa_size = 0x00018000,
        .partition = &slot0,
    },
};

yi_device_init_all();
yi_mcuboot_flash_map_set(areas, sizeof(areas) / sizeof(areas[0]));
```

The real layout belongs to the board/application and must match the bootloader
and application linker scripts. Partition offsets are relative to the backing
flash device, not absolute CPU addresses. All partitions must be aligned to the
backing device's erase size.

The initial configuration uses TinyCrypt, EC-P256 signatures, primary-slot
validation, and overwrite-only upgrades. The remaining integration steps are
to create the STM32F103 bootloader executable, generate a project-owned signing
key and compiled-in public key, and add signed-image generation to the
application build.
