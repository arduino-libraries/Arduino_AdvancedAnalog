#include <vector>
#include "DMABuffer.h"
#include "pinDefinitions.h"

enum {
    ADC_RESOLUTION_8  = 8,
    ADC_RESOLUTION_12 = 12,
    ADC_RESOLUTION_16 = 16,
};

enum {
    ADC_SAMPLE_RATE_48K = 48000,
};

struct adc_descr_t;
typedef DMABuffer<uint16_t>     ADCBuffer;
typedef DMABufferPool<uint16_t> ADCBufferPool;
typedef mbed::Callback<void()>  adc_callback_t;

class AdvancedADC {
    private:
        adc_descr_t *adc_descr;
        std::vector<PinName> adc_pins;

    public:
        template <typename ... T> AdvancedADC(pin_size_t p0, T ... args): adc_descr(nullptr) {
            for (auto p : {p0, args...}) {
                adc_pins.push_back(analogPinToPinName(p));
            }
        }
        bool available();
        ADCBuffer dequeue();
        int begin(uint32_t bits, uint32_t fs, size_t n_samples, size_t n_buffers, adc_callback_t cb=nullptr);
        int stop();
};
