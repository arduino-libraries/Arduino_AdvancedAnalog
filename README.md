# AdvancedAnalogRedux 〰
Advanced Analog Library

## Pending issues

* Add support ADC internal channels (VBAT, VREF, TEMP).
* Timestamp DMA buffers in timer IRQs (currently timestamped after DMA transfer).
* Most of the ADC channels are shared between ADC1 and ADC2, the pin mapping will always return one or the other, therefore it's not possible to use 2 ADC instances at the same time.
* ADC is running at the slowest possible clock (PCLK), should probably set up a PLL and change the clock source.
