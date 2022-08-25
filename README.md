# AdvancedAnalogRedux
Advanced Analog Library

## Fixed/Done
* Optimize how pins are stored in the ADC/DAC class (currently using vectors).
* Static assert number of ADC/DAC pins/channels passed to the constructor.
* Add DMA buffer flags (discontinuous and interleaved).
* Add number of channels to DMA buffers.
* Fix DAC timer/DMA start bug.
* Fix DAC double buffering bug (a buffer must be ready immediately after the first transfer.
* Fix DAC 2nd channel configuration.
* Add more examples.

## Pending

* Add support ADC internal channels (VBAT, VREF, TEMP).
* Timestamp DMA buffers in timer IRQs (currently timestamped after DMA transfer).
* Support not allocating a pool for DAC.

## Can't fix

* There's an issue on the ADC side, so most of the channels are shared between ADC1 and ADC2, and if you create ADC(A0) it will use by default ADC1, and then ADC(A1) it likely also try to use ADC1 which will fail because it's already used by A0  It all depends on how mbed functions map these pins to ADC:
