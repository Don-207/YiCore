# UART drivers

The UART interface supports polling and backend-specific asynchronous
operation. `yi_uart_dma_lwrb` combines UART DMA reception with the lightweight
ring buffer library so application reads are decoupled from DMA wraparound.

DMA buffers and ring-buffer storage must remain valid for the lifetime of the
device. Call the generated UART/DMA interrupt handlers from the corresponding
SoC IRQ entry points.
