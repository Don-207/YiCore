# GPIO LED driver

The LED driver wraps a GPIO output and handles active-high or active-low
wiring. It provides initialization plus on, off, and toggle operations.

```dts
led0: led0 {
    compatible = "yi,gpio-led";
    gpios = <&led0_gpio>;
    active-low;
    status = "okay";
};
```

The referenced GPIO must be configured as an output.
