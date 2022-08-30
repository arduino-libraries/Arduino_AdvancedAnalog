#include <array>
#include "DMABuffer.h"
#include "AdvancedAnalog.h"

struct adc_descr_t;

class AdvancedADC {
    private:
        size_t n_channels;
        adc_descr_t *descr;
        PinName adc_pins[AN_MAX_ADC_CHANNELS];

    public:
        template <typename ... T>
        AdvancedADC(pin_size_t p0, T ... args): n_channels(0), descr(nullptr) {
            static_assert(sizeof ...(args) < AN_MAX_ADC_CHANNELS,
                    "A maximum of 5 channels can be sampled successively.");

            for (auto p : {p0, args...}) {
                adc_pins[n_channels++] = analogPinToPinName(p);
            }
        }
        ~AdvancedADC();
        bool available();
        SampleBuffer read();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers);
        int stop();
};
