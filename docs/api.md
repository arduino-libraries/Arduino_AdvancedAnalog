# Arduino Advanced Analog Library

## AdvancedADC

### `AdvancedADC`

Creates an ADC object using the specified pin(s). The ADC object can sample a single channel or multiple channels successively if more than one pin is passed to the constructor. In this case, data from multiple channels will be interleaved in sample buffers. Additionally, the ADC channel of the first pin determines the ADC instance used, and the remaining channels (if any) must all belong to the same ADC instance.

#### Syntax

```
AdvancedADC adc(analogPin);
```

#### Parameters

- Pin `A0` through `A11` can be used.

#### Returns

`void`.

### `AdvancedADC.begin()`

Initializes and configures the ADC with the specified parameters. To reconfigure the ADC, `stop()` must be called first.

#### Syntax

```
adc0.begin(resolution, sample_rate, n_samples, n_buffers)
```

#### Parameters

- `enum` - **resolution** the sampling resolution (can be 8, 10, 12, 14 or 16 bits).
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
  - `AN_RESOLUTION_14`
  - `AN_RESOLUTION_16`
- `int` - **sample_rate** - the sampling rate / frequency in Hertz, e.g. `16000`.
- `int` - **n_samples** - the number of samples per sample buffer. See [SampleBuffer](#samplebuffer) for more details.
- `int` - **n_buffers** - the number of sample buffers in the queue. See [SampleBuffer](#samplebuffer) for more details.
- `bool` - **start** - if true (the default) the ADC will start sampling immediately, otherwise `start()` can be called later to start the ADC.
- `enum` - **sample_time** - the sampling time in cycles (the default is 8.5 cycles).
  - `AN_ADC_SAMPLETIME_1_5`
  - `AN_ADC_SAMPLETIME_2_5`
  - `AN_ADC_SAMPLETIME_8_5`
  - `AN_ADC_SAMPLETIME_16_5`
  - `AN_ADC_SAMPLETIME_32_5`
  - `AN_ADC_SAMPLETIME_64_5`
  - `AN_ADC_SAMPLETIME_387_5`
  - `AN_ADC_SAMPLETIME_810_5`

#### Returns

1 on success, 0 on failure.

### `AdvancedADC.available()`

Checks if the ADC is readable.

#### Syntax

```
if(adc0.available()){}
```

#### Parameters

None.

#### Returns

Returns true, if there's at least one sample buffer in the read queue, otherwise returns false.

### `AdvancedADC.read()`

Returns a sample buffer from the queue for reading.

### `AdvancedADC.start()`

Starts the ADC sampling.

#### Syntax

```
adc.start()
```

#### Returns

1 on success, 0 on failure.

### `AdvancedADC.stop()`

Stops the ADC and releases all of its resources.

#### Syntax

```
adc.stop()
```

#### Returns

- `1`

## AdvancedADCDual

### `AdvancedADCDual`

The AdvancedADCDual class enables the configuration of two ADCs in Dual ADC mode. In this mode, the two ADCs are synchronized, and can be sampled simultaneously, with one ADC acting as the master ADC. Note: This mode is only supported on ADC1 and ADC2, and they must be passed to `begin()` in that order.

#### Syntax

```
AdvancedADCDual adc_dual(adc1, adc2);
```

#### Parameters

- `AdvancedADC` - **adc1** - the first ADC (must be ADC1).
- `AdvancedADC` - **adc2** - the second ADC (must be ADC2).

#### Returns

`void`.

### `AdvancedADCDual.begin()`

Initializes and starts the two ADCs with the specified parameters.

#### Syntax

```
adc_dual.begin(resolution, sample_rate, n_samples, n_buffers)
```

#### Parameters

- `enum` - **resolution** the sampling resolution (can be 8, 10, 12, 14 or 16 bits).
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
  - `AN_RESOLUTION_14`
  - `AN_RESOLUTION_16`
- `int` - **sample_rate** - the sampling rate / frequency in Hertz, e.g. `16000`.
- `int` - **n_samples** - the number of samples per sample buffer. See [SampleBuffer](#samplebuffer) for more details.
- `int` - **n_buffers** - the number of sample buffers in the queue. See [SampleBuffer](#samplebuffer) for more details.
- `enum` - **sample_time** - the sampling time in cycles (the default is 8.5 cycles).
  - `AN_ADC_SAMPLETIME_1_5`
  - `AN_ADC_SAMPLETIME_2_5`
  - `AN_ADC_SAMPLETIME_8_5`
  - `AN_ADC_SAMPLETIME_16_5`
  - `AN_ADC_SAMPLETIME_32_5`
  - `AN_ADC_SAMPLETIME_64_5`
  - `AN_ADC_SAMPLETIME_387_5`
  - `AN_ADC_SAMPLETIME_810_5`

#### Returns

1 on success, 0 on failure.

### `AdvancedADCDual.stop()`

Stops the dual ADCs and releases all resources.

## AdvancedDAC

### `AdvancedDAC`

Creates a DAC object using the specified pin.

#### Syntax

```
AdvancedDAC dac0(A12);
AdvancedDAC dac1(A13);
```

#### Parameters

- `A12` or `A13` (DAC0 or DAC1 channels).

#### Returns

`void`.

### `AdvancedDAC.begin()`

Initializes the DAC with the specified parameters. To reconfigure the DAC, `stop()` must be called first. The DAC has a special mode called _loop mode_ enabled by setting `loop` parameter to `true`. In loop mode, the DAC will start automatically after all buffers are filled, and continuously cycle through over all buffers.

#### Syntax

```
dac.begin(resolution, frequency, n_samples, n_buffers, loop=false)
```

#### Parameters

- `enum` - resolution (can be 8, 10, 12 bits).
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
- `int` - **frequency** - the output frequency in Hertz, e.g. `8000`.
- `int` - **n_samples** - the number of samples per sample buffer. See [SampleBuffer](#samplebuffer) for more details.
- `int` - **n_buffers** - the number of sample buffers in the queue. See [SampleBuffer](#samplebuffer) for more details.
- `bool`- **loop** - enables loop mode.

#### Returns

1 on success, 0 on failure.

### `AdvancedDAC.available()`

Checks if the DAC is writable.

#### Syntax

```
if(dac0.available()){}
```

#### Parameters

None.

#### Returns

Returns true, if there's at least one free sample buffer in the write queue, otherwise returns false.

### `AdvancedDAC.dequeue()`

Returns a sample buffer from the queue for writing. 

#### Syntax

```
SampleBuffer buf = dac.dequeue();

for (size_t i=0; i<buf.size(); i++) {
    buf[i] =  SAMPLES_BUFFER[i];
}

dac1.write(buf);
```

### `AdvancedDAC.write()`

Writes the sample buffer back to the DAC.

#### Syntax

```
dac1.write(buf);
```

#### Parameters

- A sample buffer. See [SampleBuffer](#samplebuffer) for more details.

### `AdvancedDAC.stop()`

Stops the DAC and releases all of its resources.

#### Syntax

```
dac.stop()
```

#### Returns

- `1`

### `AdvancedDAC.frequency()`

Sets the DAC's frequency. This function can only be used after `begin()` has been called.

#### Syntax

```
int frequency = 20000;

dac.frequency(frequency);
```

#### Parameters

- `int` - frequency in Hertz (Hz).

## AdvancedI2S

### `AdvancedI2S`

Creates an I2S object using the specified pins. The I2S object can be configured in input, output or full-duplex (in/out) modes to read/write PCM samples.

#### Syntax

```
AdvancedI2S i2s(WS, CK, SDI, SDO, MCK);
```

#### Parameters

- `WS` I2S word select (LR clock).
- `CK` I2S bit-clock (BRCLK clock).
- `SDI` I2S data input (can be `NC` in half-duplex output mode).
- `SDO` I2S data output (can be `NC` in half-duplex input mode).
- `MCK` Master clock (can be `NC` if no MCLK is required).

#### Returns

`void`.

### `AdvancedI2S.begin()`

Initializes and starts the I2S device.

#### Syntax

```
i2s.begin(mode, resolution, frequency, n_samples, n_buffers)
```

#### Parameters

- `enum` - **mode** - The I2S mode.
  - `AN_I2S_MODE_IN`
  - `AN_I2S_MODE_OUT`
  - `AN_I2S_MODE_INOUT`
- `int` - **sample_rate** - The sample rate / frequency in Hertz, e.g. `16000`.
- `int` - **n_samples** - the number of samples per sample buffer. See [SampleBuffer](#samplebuffer) for more details.
- `int` - **n_buffers** - the number of sample buffers in the queue. See [SampleBuffer](#samplebuffer) for more details.

#### Returns

1 on success, 0 on failure.

### `AdvancedI2S.available()`

Checks if the I2S is readable, writable or both (in full-duplex mode).

#### Syntax

```
if(i2s.available()){}
```

#### Parameters

None.

#### Returns

Returns true if I2S is readable, writable or both (in full-duplex mode), false if not.

### `AdvancedI2S.read()`

Returns a sample buffer from the queue for reading. This function can be called in half-duplex input or full-duplex modes. When the buffer is no longer needed, it should be returned to I2S by calling `release()`, see [SampleBuffer](#release) for more details.

#### Syntax

```
SampleBuffer buf = i2s.read();

for (size_t i=0; i<buf.size(); i++) {
    Serial.println(buf[i]);
}

i2s.release(buf);
```

### `AdvancedI2S.dequeue()`

Returns a sample buffer from the queue for writing. This function can be called in half-duplex output or full-duplex modes. When the buffer is done, it can be added to the I2S write queue by calling `write()` (see [I2S](#write))

#### Syntax

```
SampleBuffer buf = i2s.dequeue();

for (size_t i=0; i<buf.size(); i++) {
    buf[i] =  SAMPLES_BUFFER[i];
}

i2s.write(buf);
```

### `AdvancedI2S.write()`

Writes a sample buffer to I2S.

#### Syntax

```
i2s.write(buf);
```

#### Parameters

- A buffer containing the samples (see [SampleBuffer](#samplebuffer)).

### `AdvancedI2S.stop()`

Stops the I2S and releases all of its resources.

#### Syntax

```
i2s.stop()
```

#### Returns

- `1`

## WavReader

### `WavReader`

Creates a WAV file reader.

### `WavReader.begin()`

Initializes the WAV reader, opens the WAV file and reads the WAV header.

#### Syntax

```
wav.begin(path, n_samples, n_buffers, loop)
```

#### Parameters

- `string` - **path** - the path to the WAV file.
- `int` - **n_samples** - the number of samples per sample buffer. See [SampleBuffer](#samplebuffer) for more details.
- `int` - **n_buffers** - the number of sample buffers in the queue. See [SampleBuffer](#samplebuffer) for more details.
- `bool` - **loop* - if true, the WAV reader will loop back to the start of the file, when the end file is reached.

### `WavReader.stop()`

Stops the WAV file and releases all of its resources (include the WAV file handle).

### `WavReader.available()`

Returns true if the WAV file has more data to be read.

### `WavReader.read()`

Returns a sample buffer from the queue for reading.

### `WavReader.rewind()`

If `loop` is false, this functions restarts the file read position.

## SampleBuffer

### `Sample`

Represents a single data item in a [`SampleBuffer`](#samplebuffer-1). The `Sample` type is pre-defined by the library as an unsigned short (`uint16_t`) type.

### `SampleBuffer`

Sample buffer objects are used throughout the library to store data samples for reading or writing. They can be used directly by the DMA, as they're cache-line-aligned, and their cache coherency is maintained by the library. The memory for sample buffers is allocated internally from memory pools and managed by the library. Each sample buffer belongs to a single memory pool, from which it was allocated, and the memory pool assigns it to the read or write queue.

#### Syntax

```
SampleBuffer buf = dac.dequeue();

for (size_t i=0; i<buf.size(); i++) {
    buf[i] =  SAMPLES_BUFFER[i];
}

dac1.write(buf);
```

### `SampleBuffer.data()`

Returns a pointer to the buffer's internal memory. The buffer's internal memory is contiguous and can be used with memcpy, for example.

```
buf.data()
```

### `SampleBuffer.size()`

Returns the buffer size in samples (i.e., the number of samples in the buffer).

```
buf.size()
```

#### Returns

- The buffer size in samples.

### `SampleBuffer.bytes()`

Returns the buffer size in bytes (i.e., the `number of samples * number of channels * sample size`).

```
buf.bytes()
```

#### Returns

- The buffer size in bytes.

### `SampleBuffer.flush()`

Flushes any cached data for the buffer.

```
buf.flush()
```

### `SampleBuffer.invalidate()`

Invalidats any cached data for the buffer.

```
buf.invalidate()
```

### `SampleBuffer.timestamp()`

Returns the timestamp of the buffer.

```
buf.timestamp()
```

#### Returns

- Timestamp as `int`.

### `SampleBuffer.channels()`

Returns the number of channels in the buffer.

```
buf.channels()
```

#### Returns

- Timestamp as `int`.

### `SampleBuffer.setflags()`

This function is used internally by the library to set the buffer's flags. The flags can be one or both of two options: `DMA_BUFFER_DISCONT` set if the capture was stopped and then restarted, and `DMA_BUFFER_INTRLVD` which indicates that the data in the buffer is interleaved.

```
buf.setflags(uint32_t mask)
```

### `SampleBuffer.getflags()`

Returns true if any of the buffer flags specified in the mask argument is set. The flags can be one or both of two options: `DMA_BUFFER_DISCONT` set if the capture was stopped and then restarted, and `DMA_BUFFER_INTRLVD` which indicates that the data in the buffer is interleaved.

```
buf.getflags(uint32_t mask)
```

#### Returns

Returns true if any of the buffer flags specified in the mask argument is set, otherwise returns false.

### `SampleBuffer.clrflags()`

Clears the buffer flags specified in the mask argument.

```
buf.clrflags(uint32_t mask)
```

#### Returns

`void`.

### `SampleBuffer.release()`

Releases the buffer back to its memory pool.

```
buf.release()
```
