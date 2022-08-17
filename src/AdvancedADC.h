#include <vector>
#include "DMABuffer.h"
#include "pinDefinitions.h"

enum {
    DAC_RESOLUTION_8  = 0U,
    DAC_RESOLUTION_12 = 1U,
};

enum {
    ADC_RESOLUTION_8  = 0U,
    ADC_RESOLUTION_10 = 1U,
    ADC_RESOLUTION_12 = 2U,
    ADC_RESOLUTION_14 = 3U,
    ADC_RESOLUTION_16 = 4U,
};

typedef uint16_t                sample_t;
typedef DMABuffer<sample_t>     &DMABuf;
typedef mbed::Callback<void()>  adc_callback_t;

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
        DMABuf read();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, adc_callback_t cb=nullptr);
        int stop();
};

struct dac_descr_t;
class AdvancedDAC {
    private:
        dac_descr_t *descr;
        std::vector<PinName> dac_pins;

    public:
        template <typename ... T> AdvancedDAC(pin_size_t p0, T ... args): descr(nullptr) {
            for (auto p : {p0, args...}) {
                dac_pins.push_back(analogPinToPinName(p));
            }
        }
        ~AdvancedDAC();

        bool available();
        DMABuf dequeue();
        DMABuf dequeue();
        void write(DMABuffer<sample_t> *dmabuf);
        int begin(uint32_t resolution, uint32_t frequency, size_t n_samples, size_t n_buffers);
        int stop();
};
