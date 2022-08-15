#include <vector>
#include "DMABuffer.h"
#include "pinDefinitions.h"

enum {
    ADC_RESOLUTION_8  = 0U,
    ADC_RESOLUTION_10 = 1U,
    ADC_RESOLUTION_12 = 2U,
    ADC_RESOLUTION_14 = 3U,
    ADC_RESOLUTION_16 = 4U,
};

struct adc_descr_t;
typedef uint16_t ADCSample;
typedef DMABuffer<ADCSample> &ADCBuffer;
typedef mbed::Callback<void()>  adc_callback_t;

class AdvancedADC {
    private:
        adc_descr_t *descr;
        std::vector<PinName> adc_pins;

    public:
        template <typename ... T> AdvancedADC(pin_size_t p0, T ... args): descr(nullptr) {
            for (auto p : {p0, args...}) {
                adc_pins.push_back(analogPinToPinName(p));
            }
        }
        ~AdvancedADC();
        bool available();
        ADCBuffer dequeue();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, adc_callback_t cb=nullptr);
        int stop();
};
