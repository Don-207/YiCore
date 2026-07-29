"""YiCore test yi build info gen utility.

Author: Don
Date: 2026-07-26
Version: 1.0.0
"""

import datetime as dt
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_build_info_gen import generate


class BuildInfoGeneratorTests(unittest.TestCase):
    def test_generates_stable_machine_readable_metadata(self):
        timestamp = dt.datetime(2026, 7, 26, 18, 9, 7,
                                tzinfo=dt.timezone(dt.timedelta(hours=8)))
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "yi_build_info.c"
            generate("application", "2.3.4", output, timestamp,
                     project_name="YiECG", project_id=1)
            source = output.read_text(encoding="utf-8")

        self.assertIn('.image = "application"', source)
        self.assertIn('.version = "2.3.4"', source)
        self.assertIn('.build_date = "2026-07-26"', source)
        self.assertIn('.build_time = "18:09:07"', source)
        self.assertIn('.project_name = "YiECG"', source)
        self.assertIn('.project_id = 1U', source)
        self.assertIn('section(".yi_build_info")', source)

    def test_rejects_fields_that_do_not_fit_binary_record(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "yi_build_info.c"
            with self.assertRaises(ValueError):
                generate("image-name-is-too-long", "1.0.0", output)
            with self.assertRaises(ValueError):
                generate("application", "1.0.0", output,
                         project_name="project-name-too-long")


if __name__ == "__main__":
    unittest.main()
