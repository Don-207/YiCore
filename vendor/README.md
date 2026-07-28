# Vendor packages

YiCore keeps upstream device libraries below this directory and keeps
YiCore-owned adaptation code below `soc/`. Vendor files must not contain
product code.

Package source, version, readiness and required-file checks are declared in
`scripts/yi_vendor_packages.json`. Run:

```text
python scripts/yi_vendor_verify.py
python scripts/yi_platform_verify.py
```

Only packages marked `ready` may be selected by supported targets. A new
package remains `pending` until its official source, license, startup/system
files, device headers and required peripheral sources have been imported and
verified.

## Layout

```text
vendor/
├── st/
│   ├── cmsis/
│   └── stm32cube/
└── gigadevice/
    └── gd32f30x/
```

Preserve the upstream directory structure where practical. Do not edit vendor
sources to implement YiCore behavior; place wrappers and workarounds in the
matching `soc/<vendor>/<family>/` backend. Any unavoidable upstream patch must
be documented with its source package revision and reason.

GD32F30x support is initially registered against the official firmware library
3.0.3. It is intentionally pending until that package is imported and its
license is reviewed.

Platform declarations are separate from package declarations:

- `scripts/yi_vendor_packages.json` describes upstream source availability.
- `scripts/yi_platforms.json` maps architecture/vendor/family to a YiCore SoC
  backend.
- `scripts/yi_supported_targets.json` lists only complete MCU targets that can
  generate a buildable project.

A reserved platform must not be added to supported targets until its vendor
package, SoC backend, startup/linker files, board description and build
template are all ready.
