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


Initializes the ADC with the specific parameters. The `begin()` method is firstly used to initialize the library.

If reconfigured during program execution, use `stop()` first.

#### Syntax

```
adc0.begin(resolution, sample_rate, n_samples, n_buffers)
```

#### Parameters

- `enum` - resolution (choose from 8, 10, 12, 14, 16 bit)
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
  - `AN_RESOLUTION_14`
  - `AN_RESOLUTION_16`
- `int` - frequency
- `int` - n_samples
- `int` - n_buffers

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

Stops the ADC and buffer transfer, and releases any memory allocated for the buffer array.

#### Syntax

```
adc.stop()
```

#### Returns

- `1`

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


Initializes the DAC with the specific parameters. The `begin()` method is firstly used to initialize the library.

If reconfigured during program execution, use `stop()` first.

#### Syntax

```
dac0.begin(resolution, frequency, n_samples, n_buffers)
```

#### Parameters

- `enum` - resolution (choose from 8, 10, 12 bit)
  - `AN_RESOLUTION_8`
  - `AN_RESOLUTION_10`
  - `AN_RESOLUTION_12`
- `int` - frequency 
- `int` - n_samples 
- `int` - n_buffers

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

Stops the DAC timer and buffer transfer, and releases any memory allocated for the buffer array.

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