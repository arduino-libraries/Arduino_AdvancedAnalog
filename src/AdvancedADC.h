#include <array>
#include "DMABuffer.h"
#include "AdvancedAnalog.h"

struct adc_descr_t;

class AdvancedADC {
    private:
        adc_descr_t *descr;
        std::array<PinName, AN_MAX_ADC_CHANNELS> adc_pins;

    public:
        template <typename ... T>
        AdvancedADC(pin_size_t p0, T ... args): descr(nullptr) {
            static_assert(sizeof ...(args) < AN_MAX_ADC_CHANNELS,
                    "A maximum of 5 channels can be sampled successively.");

            size_t i=0;
            for (auto p : {p0, args...}) {
                adc_pins[i++] = analogPinToPinName(p);
            }
        }
        ~AdvancedADC();
        bool available();
        SampleBuffer read();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, user_callback_t callback=nullptr);
        int stop();
};
