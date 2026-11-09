#include "AmpVisualDetector.h"
#include <iostream>
#include <array>

int main()
{
    struct Test { const char* filename; const char* metadata; const char* directory; int expected; int minConfidence; };
    constexpr std::array<Test, 21> tests {{
        {"Plexi_100W_A2.nam", "", "", 1, 2},
        {"JCM800_2203_A2.nam", "", "", 2, 2},
        {"EVH_5150III_Blue.nam", "", "", 3, 2},
        {"Mesa_Dual_Rectifier.nam", "", "", 4, 2},
        {"Twin_Reverb_clean.nam", "", "", 5, 2},
        {"5E3_tweed.nam", "", "", 6, 2},
        {"Vox_AC30.nam", "", "", 7, 2},
        {"SLO_100.nam", "", "", 8, 2},
        {"Friedman_BE100.nam", "", "", 9, 2},
        {"Rockerverb.nam", "", "", 10, 2},
        {"Hiwatt_DR103.nam", "", "", 11, 2},
        {"Dumble_ODS100.nam", "", "", 12, 2},
        {"Ampeg_SVT.nam", "", "", 13, 2},
        {"Diezel_VH4.nam", "", "", 14, 2},
        {"ENGL_Powerball.nam", "", "", 15, 2},
        {"JC120_Studio.nam", "", "", 16, 2},
        {"Trainwreck_Express_A1.nam", "", "", 17, 2},
        {"Bogner_Uberschall.nam", "", "", 18, 2},
        {"mystery123.nam", "", "", 0, 0},
        {"untitled.nam", "gear: Marshall JCM800 2203", "", 2, 3},
        {"unknown.nam", "", "Orange Rockerverb captures", 10, 1}
    }};
    for (const auto& test : tests)
    {
        const auto result = libertyAmp::detect(test.filename, test.metadata, test.directory);
        if (result.family != test.expected || result.confidence < test.minConfidence)
        {
            std::cerr << "Detection failed: " << test.filename << " -> " << result.family
                      << " expected " << test.expected << "\n";
            return 1;
        }
    }
    if (libertyAmp::detect("Plexi_JCM800.nam").family != 0)
        return 2;
    std::cout << "22 amp visual detection checks passed.\n";
}
