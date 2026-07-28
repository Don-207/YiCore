"""YiCore test yi mcuboot layout utility.

Author: Don
Date: 2026-07-26
Version: 1.0.0
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
LAYOUT = ROOT / "subsys/boot/mcuboot/yi_mcuboot_layout_stm32f103xe.h"


def layout_values():
    text = LAYOUT.read_text(encoding="utf-8")
    return {
        name: int(value, 16)
        for name, value in re.findall(
            r"#define\s+(YI_MCUBOOT_[A-Z_]+)\s+(0x[0-9A-Fa-f]+)U", text
        )
    }


class McubootLayoutTests(unittest.TestCase):
    def test_internal_flash_layout_is_contiguous_and_page_aligned(self):
        values = layout_values()
        boot_end = values["YI_MCUBOOT_BOOT_OFFSET"] + values["YI_MCUBOOT_BOOT_SIZE"]
        primary_end = values["YI_MCUBOOT_PRIMARY_OFFSET"] + values["YI_MCUBOOT_SLOT_SIZE"]
        secondary_end = values["YI_MCUBOOT_SECONDARY_OFFSET"] + values["YI_MCUBOOT_SLOT_SIZE"]
        state_end = values["YI_MCUBOOT_UPDATE_STATE_OFFSET"] + values["YI_MCUBOOT_UPDATE_STATE_SIZE"]
        scratch_end = values["YI_MCUBOOT_SCRATCH_OFFSET"] + values["YI_MCUBOOT_SCRATCH_SIZE"]

        self.assertEqual(boot_end, values["YI_MCUBOOT_PRIMARY_OFFSET"])
        self.assertEqual(primary_end, values["YI_MCUBOOT_SECONDARY_OFFSET"])
        self.assertEqual(secondary_end, values["YI_MCUBOOT_UPDATE_STATE_OFFSET"])
        self.assertEqual(state_end, values["YI_MCUBOOT_SCRATCH_OFFSET"])
        self.assertEqual(scratch_end, values["YI_MCUBOOT_FLASH_SIZE"])
        for key in ("YI_MCUBOOT_BOOT_OFFSET", "YI_MCUBOOT_BOOT_SIZE",
                    "YI_MCUBOOT_PRIMARY_OFFSET", "YI_MCUBOOT_SECONDARY_OFFSET",
                    "YI_MCUBOOT_SLOT_SIZE", "YI_MCUBOOT_UPDATE_STATE_OFFSET",
                    "YI_MCUBOOT_UPDATE_STATE_SIZE", "YI_MCUBOOT_SCRATCH_OFFSET",
                    "YI_MCUBOOT_SCRATCH_SIZE"):
            self.assertEqual(values[key] % values["YI_MCUBOOT_ERASE_SIZE"], 0)

    def test_scatter_files_match_layout(self):
        values = layout_values()
        boot_sct = (ROOT / "linker/armclang/mcuboot-stm32f103xe.sct").read_text()
        app_sct = (ROOT / "linker/armclang/app-slot0-stm32f103xe.sct").read_text()
        app_address = (values["YI_MCUBOOT_FLASH_BASE"] +
                       values["YI_MCUBOOT_PRIMARY_OFFSET"] +
                       values["YI_MCUBOOT_IMAGE_HEADER_SIZE"])
        app_size = (values["YI_MCUBOOT_SLOT_SIZE"] -
                    values["YI_MCUBOOT_IMAGE_HEADER_SIZE"] -
                    values["YI_MCUBOOT_ERASE_SIZE"])

        self.assertIn(f"0x{values['YI_MCUBOOT_FLASH_BASE']:08X} 0x{values['YI_MCUBOOT_BOOT_SIZE']:08X}", boot_sct)
        self.assertIn(f"0x{app_address:08X} 0x{app_size:08X}", app_sct)
        build_info_address = (values["YI_MCUBOOT_FLASH_BASE"] +
                              values["YI_MCUBOOT_BOOT_OFFSET"] +
                              values["YI_MCUBOOT_BOOT_SIZE"] -
                              values["YI_MCUBOOT_ERASE_SIZE"])
        self.assertIn(f"yi_build_info 0x{build_info_address:08X} FIXED",
                      boot_sct)


if __name__ == "__main__":
    unittest.main()
