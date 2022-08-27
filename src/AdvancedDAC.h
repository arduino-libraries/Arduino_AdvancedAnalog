#include <vector>
#include "DMABuffer.h"
#include "AdvancedAnalog.h"

struct dac_descr_t;

class AdvancedDAC {
    private:
        size_t n_channels;
        dac_descr_t *descr;
        PinName dac_pins[AN_MAX_DAC_CHANNELS];

    public:
        template <typename ... T>
        AdvancedDAC(pin_size_t p0, T ... args): n_channels(0), descr(nullptr) {
            static_assert(sizeof ...(args) < AN_MAX_DAC_CHANNELS,
                    "A maximum of 1 channel is currently supported.");

            for (auto p : {p0, args...}) {
                dac_pins[n_channels++] = analogPinToPinName(p);
            }
        }
        ~AdvancedDAC();

        bool available();
        SampleBuffer dequeue();
        void write(SampleBuffer dmabuf);
        int begin(uint32_t resolution, uint32_t frequency, size_t n_samples, size_t n_buffers, user_callback_t cb=nullptr);
        int stop();
};
