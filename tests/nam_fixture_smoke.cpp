#include <get_dsp.h>
#include <dsp.h>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    try
    {
        auto model = nam::get_dsp(std::filesystem::path(argv[1]));
        if (!model || model->NumInputChannels() != 1 || model->NumOutputChannels() != 1)
            return 3;
        const double expected = model->GetExpectedSampleRate();
        model->Reset(expected > 0 ? expected : 48000.0, 64);
        std::array<float,64> input {}, output {};
        input[0] = 0.001f;
        float* in[] = { input.data() };
        float* out[] = { output.data() };
        model->process(in, out, (int)input.size());
        for (float sample : output) if (!std::isfinite(sample)) return 4;
        std::cout << "Real NAM example loaded and processed: " << argv[1] << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "NAM fixture error: " << e.what() << "\n";
        return 5;
    }
    return 0;
}
