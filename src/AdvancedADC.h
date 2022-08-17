#include <vector>
#include "DMABuffer.h"
#include "AdvancedAnalog.h"
#include "pinDefinitions.h"

struct adc_descr_t;

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
        SampleBuffer read();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, user_callback_t callback=nullptr);
        int stop();
};
