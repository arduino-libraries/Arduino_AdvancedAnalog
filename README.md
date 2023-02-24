`Arduino_AdvancedAnalog` 〰
===========================
[![Compile Examples status](https://github.com/bcmi-labs/AdvancedAnalogRedux/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/bcmi-labs/AdvancedAnalogRedux/actions/workflows/compile-examples.yml)
[![Spell Check status](https://github.com/bcmi-labs/AdvancedAnalogRedux/actions/workflows/spell-check.yml/badge.svg)](https://github.com/bcmi-labs/AdvancedAnalogRedux/actions/workflows/spell-check.yml)
[![Sync Labels status](https://github.com/bcmi-labs/AdvancedAnalogRedux/actions/workflows/sync-labels.yml/badge.svg)](https://github.com/bcmi-labs/AdvancedAnalogRedux/actions/workflows/sync-labels.yml)
[![Arduino Lint](https://github.com/bcmi-labs/AdvancedAnalogRedux/workflows/Arduino%20Lint/badge.svg)](https://github.com/bcmi-labs/AdvancedAnalogRedux/actions?workflow=Arduino+Lint)

Advanced Analog Library for Arduino Giga.

## Known issues
* ADC is running at the slowest possible clock (PCLK), should probably set up a PLL and change the clock source.
