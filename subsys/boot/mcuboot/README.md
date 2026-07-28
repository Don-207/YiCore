# MCUboot integration

This directory is YiCore's porting layer for the unmodified MCUboot sources in
`third_party/mcuboot`. It provides the bootloader flash-map API and the
transport-independent `yi_mcuboot_upgrade` application module for one
updatable image.

The GCC port also supplies a fixed boot-time allocation arena and minimal C
runtime. It does not link Newlib or provide a general-purpose heap.

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

The application module erases the complete secondary slot, accepts sequential
flash-aligned blocks, checks CRC-16/MODBUS and the MCUboot image header, and
writes the secondary trailer magic only after validation. A product transport
(UART, CAN, USB, or radio) owns framing, hardware/version policy, retries, and
reset timing. Cryptographic authentication is deliberately performed again by
MCUboot before it replaces the primary image.

Typical setup creates a `yi_partition_t` for
`YI_MCUBOOT_SECONDARY_OFFSET`/`YI_MCUBOOT_SLOT_SIZE`, then initializes
`yi_mcuboot_update_t` with primary, secondary, and persistent-state
partitions. Call `begin`, sequential `write` operations, then `finish`. Only
`finish` commits a test-swap trailer. The update layer checkpoints completed
erase pages, resumes an identical package after reset, and erases a possibly
torn page before receiving it again.

The configuration uses TinyCrypt SHA-256 integrity, primary-slot validation,
and scratch swapping. No public/private key or signature authentication is
configured. The new image is first booted in test mode. The
application must call `yi_mcuboot_update_confirm()` only after its health
checks pass; otherwise the next boot restores the old primary image.
