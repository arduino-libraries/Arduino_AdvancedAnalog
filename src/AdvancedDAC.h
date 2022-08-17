#include <vector>
#include "DMABuffer.h"
#include "AdvancedAnalog.h"
#include "pinDefinitions.h"

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
        SampleBuffer dequeue();
        void write(SampleBuffer dmabuf);
        int begin(uint32_t resolution, uint32_t frequency, size_t n_samples, size_t n_buffers, user_callback_t cb=nullptr);
        int stop();
};
