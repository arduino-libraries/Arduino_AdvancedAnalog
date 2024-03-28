# Arduino Advanced Analog Library

## AdvancedADC

### `AdvancedADC`

Creates an object associated to a specific pin.

#### Syntax

```
AdvancedADC adc(analogPin);
```

#### Parameters

- Pin `A0` through `A11` can be associated.

#### Returns

Nothing.

### `begin()`


Initializes the ADC with the specified parameters. If reconfiguration is needed during program execution, `stop()` should be called first.

#### Syntax

```
adc0.begin(resolution, sample_rate, n_samples, n_buffers)
```

#### Parameters

- `enum` - **resolution** the ADC resolution (can be 8, 10, 12, 14 or 16 bit).
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
  - `AN_RESOLUTION_14`
  - `AN_RESOLUTION_16`
- `int` - **sample_rate** - the sample rate / frequency in Hertz, e.g. `16000`.
- `int` - **n_samples** - number of samples per buffer, e.g. `32`. When reading the ADC, we store these samples into a specific buffer (see [SampleBuffer](#samplebuffer)), and read them via `buffer[x]`, where `x` is the sample you want to retrieve.
- `int` - **n_buffers** - the number of buffers in the queue.
- `bool` - **start** - if true, the ADC will start sampling immediately (the default is true).
- `enum` - **sample_time** - the ADC sampling time in cycles (the default is 8.5 cycles).
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

### `available()`

Bool to check if there's any available data on the ADC channel.

#### Syntax

```
if(adc0.available()){}
```

#### Parameters

None.

#### Returns

1 on success, 0 on failure.

### `read()`

Reads the first available byte in the buffer.

### `stop()`

Stops the ADC and releases all of its resources.

#### Syntax

```
adc.stop()
```

#### Returns

- `1`

## AdvancedADCDual

### `AdvancedADCDual`

The AdvancedADCDual class enables the configuration of two ADCs in Dual ADC mode. In this mode, the two ADCs are synchronized, with one ADC acting as the master ADC and controlling the other. Note: This mode is only supported on ADC1 and ADC2, and they must be passed in that order.

#### Syntax

```
AdvancedADCDual adc_dual(adc1, adc2);
```

#### Parameters

- `AdvancedADC` - **adc1** - the first ADC. Note this must be ADC1.
- `AdvancedADC` - **adc2** - the second ADC. Note this must be ADC2.

#### Returns

Nothing.

### `begin()`

Initialize and start the two ADCs with the specified parameters.

#### Syntax

```
adc_dual.begin(resolution, sample_rate, n_samples, n_buffers)
```

#### Parameters

- `enum` - **resolution** the ADC resolution (can be 8, 10, 12, 14 or 16 bit).
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
  - `AN_RESOLUTION_14`
  - `AN_RESOLUTION_16`
- `int` - **sample_rate** - the sample rate / frequency in Hertz, e.g. `16000`.
- `int` - **n_samples** - number of samples per buffer, e.g. `32`. When reading the ADC, we store these samples into a specific buffer (see [SampleBuffer](#samplebuffer)), and read them via `buffer[x]`, where `x` is the sample you want to retrieve.
- `int` - **n_buffers** - the number of buffers in the queue.
- `bool` - **start** - if true, the ADC will start sampling immediately (the default is true).
- `enum` - **sample_time** - the ADC sampling time in cycles (the default is 8.5 cycles).
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

### `stop()`

Stops the two ADCs and releases all of their resources.

## AdvancedDAC

### `AdvancedDAC`


Creates a DAC object on a specific pin.

#### Syntax

```
AdvancedDAC dac0(A12);
AdvancedDAC dac1(A13);
```

#### Parameters

- `A12` or `A13` (DAC0 or DAC1 channels).

#### Returns

Nothing.

### `begin()`

Initializes the DAC with the specified parameters. If reconfiguration is needed during program execution, `stop()` should be called first.

#### Syntax

```
dac0.begin(resolution, frequency, n_samples, n_buffers)
```

#### Parameters

- `enum` - resolution (choose from 8, 10, 12 bit)
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
- `int` - **frequency** - the frequency in Hertz, e.g. `8000`.
- `int` - **n_samples** - number of samples per buffer, e.g. `32`. When writing to the DAC, we first write the samples into a buffer (see [SampleBuffer](#samplebuffer)), and write it to the DAC using `dac_out.write(buf)`.
- `int` - **n_buffers** - the number of buffers in the queue.


#### Returns

1 on success, 0 on failure.

### `available()`

Checks if the DAC channel is available to write to.

#### Syntax

```
if(dac0.available()){}
```

#### Parameters

None.

#### Returns

1 on success, 0 on failure.


### `dequeue()`


Creates a buffer object and waits until a buffer to become available.

#### Syntax

```
SampleBuffer buf = dac.dequeue();

for (size_t i=0; i<buf.size(); i++) {
     buf[i] =  SAMPLES_BUFFER[i];
}

dac1.write(buf);
```

### `write()`


Writes the buffer to the DAC channel specified. Before writing, the cached data is flushed.

#### Syntax

```
dac1.write(buf);
```

#### Parameters

- A buffer containing the samples (see [SampleBuffer](#samplebuffer)).

### `stop()`

Stops the DAC and releases all of its resources.

#### Syntax

```
dac.stop()
```

#### Returns

- `1`

### `frequency()`

Sets the frequency for the DAC. This can only be used after `begin()`, where an initial frequency is set.

#### Syntax

```
int frequency = 20000;

dac.frequency(frequency);
```

#### Parameters

- `int` - frequency in Hertz (Hz).

## AdvancedI2S

### `AdvancedI2S`

Creates an I2S object using the provided pins.

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

Nothing.

### `begin()`

Initialize and start the I2S device.

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
- `int` - **n_samples** - The number of samples per buffer (i.e, buffer width).
- `int` - **n_buffers** - The number of buffers in the sample queue (i.e, queue depth).

#### Returns

1 on success, 0 on failure.

### `available()`

Checks if the I2S is readable, writable or both (in full-duplex mode).

#### Syntax

```
if(i2s.available()){}
```

#### Parameters

None.

#### Returns

True if I2S is readable, writable or both (in full-duplex mode), false if not.

### `read()`

Returns a sample buffer from the queue for reading. This function can be called in half-duplex input or full-duplex modes. When the code is done with the buffer, it should be returned to I2S by calling `release()` (see [SampleBuffer](#release))

#### Syntax

```
SampleBuffer buf = i2s.read();

for (size_t i=0; i<buf.size(); i++) {
     Serial.println(buf[i]);
}

i2s.release(buf);
```

### `dequeue()`

Returns a sample buffer from the queue for writing. This function can be called in half-duplex output or full-duplex modes. When the code is done with the buffer, it should be written out to I2S by calling `write()` (see [I2S](#write))

#### Syntax

```
SampleBuffer buf = dac.dequeue();

for (size_t i=0; i<buf.size(); i++) {
     buf[i] =  SAMPLES_BUFFER[i];
}

dac1.write(buf);
```

### `write()`

Writes the buffer to I2S.

#### Syntax

```
i2s.write(buf);
```

#### Parameters

- A buffer containing the samples (see [SampleBuffer](#samplebuffer)).

### `stop()`

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

### `begin()`

Initializes the WAV reader, opens the file and reads the WAV file header.

#### Syntax

```
wav.begin(path, n_samples, n_buffers, loop)
```

#### Parameters

- `string` - **path** - the path to the WAV file.
- `int` - **n_samples** - the number of samples per buffer.
- `int` - **n_buffers** - the number of buffers in the queue.
- `bool` - **loop* - if true, the WAV reader will loop back to the start of the file, when the end file is reached.

### `stop()`

Stops the WAV file and releases all of its resources (include the WAV file handle).

### `available()`

Returns true if the WAV file has more data to be read.

### `read()`

Returns a sample buffer from the queue for reading.

### `rewind()`

If `loop` is false, this functions restarts the file read position.

## SampleBuffer

### Sample

`Sample` is the data type used for the [`SampleBuffer`](#samplebuffer-1), and is identical to an unsigned short variable (`uint16_t`).

A single `Sample` can store 2^16 or 65,535 distinct values.

### `SampleBuffer`

`SampleBuffer` is used to store samples (see [Sample](#sample)) that are then written to the DAC. Before writing, first use `dequeue()`, then write data to the buffer.

#### Syntax

```
SampleBuffer buf = dac.dequeue();

for (size_t i=0; i<buf.size(); i++) {
     buf[i] =  SAMPLES_BUFFER[i];
}

dac1.write(buf);
```

### `data()`

Returns a pointer to the buffer's memory.

```
buf.data()
```

### `size()`

Returns the size of the buffer.

```
buf.size()
```

### `bytes()`

Returns the total number of bytes used to store the audio data, by using the formula `n_samples * n_channels * sample`.

```
buf.bytes()
```

#### Returns

- Total number of bytes.

### `flush()`

Clears the data cache.

```
buf.flush()
```

### `invalidate()`

Invalidats the cache by discarding the data.

```
buf.invalidate()
```

### `timestamp()`

Returns the timestamp of the buffer.

```
buf.timestamp()
```

#### Returns

- Timestamp as `int`.

### `channels()`

Returns the number of channels used in the buffer.

```
buf.channels()
```

#### Returns

- Timestamp as `int`.


### `release()`

Releases the buffer back into a free pool.

```
buf.release()
```

#### Returns

- Nothing (void).

### `setflags()`

Sets flag(s) for the buffer.

```
buf.setflags(int arg)
```

### `getflags()`

Get flag(s) of the buffer.

```
buf.getflags(int arg)
```

#### Returns

- `int` - flags.

### `clrflags()`

Clear flag(s) from the buffer.

```
buf.getflags(int arg)
```

### `writable()`

Returns the amount of writable elements in the buffer.

```
buf.writable()
```

#### Returns

- Amount of readable elements in the buffer.

### `readable()`

Returns the amount of readable elements in the buffer.

```
buf.readable()
```

### `allocate()`

Used to obtain a buffer from the free queue.

```
buf.allocate()
```

### `release()`

Releases the buffer back to the free queue.

```
buf.release()
```

### `enqueue()`

Add a DMA buffer to the ready queue.

```
buf.enqueue()
```

### `dequeue()`

Return a DMA buffer from the ready queue.

```
buf.dequeue()
```
