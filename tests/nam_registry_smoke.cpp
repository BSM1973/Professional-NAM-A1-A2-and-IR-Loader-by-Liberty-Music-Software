#include <model_config.h>
#include <iostream>
int main()
{
    const auto& registry = nam::ConfigParserRegistry::instance();
    bool ok = true;
    for (const char* name : { "WaveNet", "LSTM", "ConvNet", "Linear" })
    {
        if (!registry.has(name))
        {
            std::cerr << "NAM parser not linked: " << name << "\n";
            ok = false;
        }
    }
    if (ok) std::cout << "NAM architecture registry passed\n";
    return ok ? 0 : 1;
}
