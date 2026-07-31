# CH32H4xx SoC backend

This backend initially supports the CH32H417 V3F core using the pinned
YiHAL-WCH vendor package. The first-stage runtime configures the vendor 25 MHz
HSE clock profile and exposes YiCore system/interrupt primitives.

V5F startup, inter-core boot, HSEM, shared memory, cache maintenance, and
production timer services are intentionally deferred until the V3F image has
passed board bring-up.
