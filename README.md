<img src="https://content.arduino.cc/website/Arduino_logo_teal.svg" height="100" align="right" />

`Arduino_AdvancedAnalog` 〰
===========================
[![Compile Examples status](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/actions/workflows/compile-examples.yml)
[![Spell Check status](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/actions/workflows/spell-check.yml/badge.svg)](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/actions/workflows/spell-check.yml)
[![Sync Labels status](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/actions/workflows/sync-labels.yml/badge.svg)](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/actions/workflows/sync-labels.yml)
[![Arduino Lint](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/workflows/Arduino%20Lint/badge.svg)](https://github.com/arduino-libraries/Arduino_AdvancedAnalog/actions?workflow=Arduino+Lint)

Advanced Analog Library for Arduino Giga.

## :mag_right: Resources

* [How to install a library](https://www.arduino.cc/en/guide/libraries)
* [Help Center](https://support.arduino.cc/)
* [Forum](https://forum.arduino.cc)

## :bug: Bugs & Issues

If you want to report an issue with this library, you can submit it to the [issue tracker](https://github.com/arduino-libraries/Arduino_Braccio_plusplus/issues) of this repository. Remember to include as much detail as you can about your hardware set-up, code and steps for reproducing the issue. Make sure you're using an original Arduino board.

## :technologist: Development

There are many ways to contribute:

* Improve documentation and examples
* Fix a bug
* Test open Pull Requests
* Implement a new feature
* Discuss potential ways to improve this library

You can submit your patches directly to this repository as Pull Requests. Please provide a detailed description of the problem you're trying to solve and make sure you test on real hardware.

## :yellow_heart: Donations

This open-source code is maintained by Arduino with the help of the community. We invest a considerable amount of time in testing code, optimizing it and introducing new features. Please consider [donating](https://www.arduino.cc/en/donate/) or [sponsoring](https://github.com/sponsors/arduino) to support our work, as well as [buying original Arduino boards](https://store.arduino.cc/) which is the best way to make sure our effort can continue in the long term.

## Known issues
* ADC is running at the slowest possible clock (PCLK), should probably set up a PLL and change the clock source.
