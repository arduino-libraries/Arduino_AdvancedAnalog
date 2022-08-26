# AdvancedAnalogRedux 〰
Advanced Analog Library

## Pending issues

* Add support ADC internal channels (VBAT, VREF, TEMP).
* Timestamp DMA buffers in timer IRQs (currently timestamped after DMA transfer).
* Support not allocating a pool for DAC.
* Most of the ADC channels are shared between ADC1 and ADC2, the pin mapping will always return one or the other, therefore it's not possible to use 2 ADC instances at the same time.
