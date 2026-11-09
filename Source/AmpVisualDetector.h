#pragma once
// Pure C++ recognition rules: intentionally independent of JUCE/audio threads.
// The result is a *visual suggestion*, NEVER proof of hardware provenance.
#include <array>
#include <cctype>
#include <string>
#include <vector>
#include <utility>

namespace libertyAmp
{
struct Family
{
    const char* name;
    const char* shortName;
    const char* description;
};

inline constexpr std::array<Family, 19> families {{
    {"Inconnu", "NAM", "Apparence neutre, origine inconnue"},
    {"British Plexi", "PLEXI", "Tete britannique vintage a facade doree"},
    {"British JCM", "JCM", "Tete rock britannique noire et doree"},
    {"Modern 5150", "5150", "Tete high-gain americaine industrielle"},
    {"Modern Rectifier", "RECTIFIER", "Tete metal moderne a facade chromee"},
    {"American Blackface", "BLACKFACE", "Combo clair americain avec facade noire"},
    {"American Tweed", "TWEED", "Combo americain vintage en tissu tweed"},
    {"British Chime", "AC", "Combo britannique a grille diamant"},
    {"Boutique SLO", "SLO", "Tete boutique high-gain bleu profond"},
    {"Boutique BE", "BE", "Tete boutique noire, rock et modifiee"},
    {"Orange British", "ORANGE", "Tete orange a facade claire"},
    {"Hiwatt Classic", "HIWATT", "Tete anglaise a panneau clair"},
    {"Boutique ODS", "ODS", "Tete boutique vintage a facade beige"},
    {"Bass SVT", "SVT", "Amplificateur basse imposant a facade aluminium"},
    {"Diezel Modern", "VH4", "Tete moderne aux commandes metalliques"},
    {"ENGL Modern", "ENGL", "Tete high-gain allemande a facade sombre"},
    {"Jazz Chorus", "JC", "Combo a transistors et grille noire"},
    {"Trainwreck Boutique", "EXPRESS", "Tete boutique a bois apparent"},
    {"Bogner Modern", "UBER", "Tete high-gain moderne a grille metallique"}
}};

inline constexpr int familyCount = static_cast<int>(families.size());

struct Result
{
    int family = 0;
    int confidence = 0; // 0 unknown, 1 low, 2 medium, 3 high
    std::string source = "unknown"; // metadata, filename, directory, ambiguous
    std::string clue;
};

inline std::string normalize(const std::string& value)
{
    std::string out = " ";
    bool lastSpace = true;
    for (unsigned char c : value)
    {
        const bool isAsciiLetter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        if (isAsciiLetter || (c >= '0' && c <= '9'))
        {
            out += static_cast<char>(std::tolower(c));
            lastSpace = false;
        }
        else if (!lastSpace)
        {
            out += ' ';
            lastSpace = true;
        }
    }
    if (!lastSpace) out += ' ';
    return out;
}

struct Rule { int family; const char* term; int strength; };
inline const std::vector<Rule>& rules()
{
    static const std::vector<Rule> list = {
        {1,"plexi",25},{1,"super lead",26},{1,"1959 slp",30},{1,"1959hw",28},{1,"1987x",30},
        {2,"jcm800",36},{2,"jcm 800",36},{2,"2203",32},{2,"2204",32},{2,"jcm900",32},{2,"jcm 900",32},
        {3,"5150",35},{3,"5150iii",36},{3,"5150 iii",36},{3,"6505",35},{3,"evh iii",26},{3,"evh 3",26},{3,"block letter",23},
        {4,"rectifier",36},{4,"dual rect",32},{4,"triple rect",32},{4,"recto",25},{4,"mesa rect",32},
        {5,"blackface",28},{5,"twin reverb",32},{5,"deluxe reverb",32},{5,"princeton reverb",32},{5,"super reverb",30},
        {6,"tweed",32},{6,"5e3",33},{6,"5f6a",33},{6,"bassman 59",30},
        {7,"ac30",34},{7,"ac 30",34},{7,"ac15",34},{7,"ac 15",34},{7,"vox top boost",32},
        {8,"soldano",32},{8,"slo100",35},{8,"slo 100",35},{8,"slo30",33},
        {9,"friedman",32},{9,"be100",35},{9,"be 100",35},{9,"hbe",25},{9,"small box",24},
        {10,"orange",29},{10,"rockerverb",34},{10,"or15",30},{10,"ad30",29},{10,"thunderverb",33},
        {11,"hiwatt",35},{11,"dr103",36},{11,"dr 103",36},{11,"dr504",35},
        {12,"dumble",35},{12,"ods100",34},{12,"ods 100",34},{12,"overdrive special",35},
        {13,"ampeg",34},{13,"svt",35},{13,"portaflex",32},{13,"b15n",33},{13,"b 15",30},
        {14,"diezel",35},{14,"vh4",35},{14,"herbert",33},{14,"hagen",31},
        {15,"engl",35},{15,"powerball",34},{15,"fireball",33},{15,"savage 120",34},{15,"invader 100",33},
        {16,"jc120",35},{16,"jc 120",35},{16,"jazz chorus",35},
        {17,"trainwreck",35},{17,"train wreck",34},{17,"express amp",27},{17,"liverpool amp",25},
        {18,"bogner",34},{18,"uberschall",35},{18,"ecstasy 101",32}
    };
    return list;
}

inline Result detect(const std::string& filename,
                     const std::string& metadata = {},
                     const std::string& parentDirectory = {})
{
    const std::array<std::pair<std::string, const char*>, 3> sources {{
        {normalize(metadata), "metadata"},
        {normalize(filename), "filename"},
        {normalize(parentDirectory), "directory"}
    }};
    const int sourceScores[3] = {150, 110, 20};
    std::array<int, familyCount> totals {};
    std::array<int, familyCount> bestOrigin {}; // 0 metadata, 1 filename, 2 directory
    std::array<std::string, familyCount> clues {};

    for (int s = 0; s < 3; ++s)
    {
        int localScores[familyCount] {};
        for (const auto& rule : rules())
        {
            if (sources[(size_t)s].first.find(" " + std::string(rule.term) + " ") == std::string::npos)
                continue;
            const int score = sourceScores[s] + rule.strength;
            if (score > localScores[rule.family])
            {
                localScores[rule.family] = score;
                // Only retain the stronger original evidence (metadata > filename > directory).
                if (totals[(size_t)rule.family] == 0 || s < bestOrigin[(size_t)rule.family])
                {
                    bestOrigin[(size_t)rule.family] = s;
                    clues[(size_t)rule.family] = rule.term;
                }
            }
        }
        for (int f = 1; f < familyCount; ++f)
            if (localScores[f] > 0)
                totals[(size_t)f] += localScores[f];
    }

    int best = 0, bestScore = 0, secondScore = 0;
    for (int f = 1; f < familyCount; ++f)
    {
        if (totals[(size_t)f] > bestScore)
        {
            secondScore = bestScore;
            bestScore = totals[(size_t)f];
            best = f;
        }
        else if (totals[(size_t)f] > secondScore)
            secondScore = totals[(size_t)f];
    }
    if (best == 0)
        return {};
    // Two similarly strong brand/model clues are an ambiguity, not a reliable match.
    if (secondScore > 0 && (bestScore - secondScore) < 22)
        return {0, 0, "ambiguous", "conflicting identifiers"};

    const int origin = bestOrigin[(size_t)best];
    const int confidence = origin == 2 ? 1 : origin == 1 ? 2 : 3;
    return {best, confidence, sources[(size_t)origin].second, clues[(size_t)best]};
}
} // namespace libertyAmp
