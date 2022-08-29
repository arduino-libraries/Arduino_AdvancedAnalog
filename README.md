# AdvancedAnalogRedux 〰
Advanced Analog Library

## Pending issues

* Add support ADC internal channels (VBAT, VREF, TEMP).
* Timestamp DMA buffers in timer IRQs (currently timestamped after DMA transfer).
* ~ADC is running at the slowest possible clock (PCLK), should probably set up a PLL and change the clock source.~ (WON'T FIX).
