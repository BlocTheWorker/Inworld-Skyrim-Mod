// PhonemeUtility.cpp
#include "PhonemeUtility.h"

PhonemeUtility* PhonemeUtility::instance = nullptr;

PhonemeUtility::PhonemeUtility() {
    phoneme_map = {
        {"a", 0}, {"ə", 0},  {"ɚ", 0},  
        {"aɪ", 1},  {"aʊ", 1},  {"æ", 1}, 
        {"m", 2}, 
        {"t", 3}, {"ʃ", 3},  {"z", 3},  {"j", 3},
        {"d", 4},  {"ð", 4},  
        {"i", 5},  
        {"ɛ", 6},  {"ɐ", 6},  {"ʌ",6}, 
        {"f", 7},  
        {"ɪ", 8},  {"l", 8},  
        {"k", 9},  
        {"n", 10}, 
        {"o", 11}, {"ɔ", 11}, 
        {"u", 12}, {"ʊ", 12}, 
        {"ɹ", 13}, 
        {"θ", 14}, {"ʒ", 14}, 
        {"w", 15}
    };
}

PhonemeUtility::PhonemeUtility(const PhonemeUtility&) {}
PhonemeUtility& PhonemeUtility::operator=(const PhonemeUtility&) { return *this; }
PhonemeUtility::~PhonemeUtility() {}

PhonemeUtility* PhonemeUtility::get_instance() {
    if (instance == nullptr) {            // If the instance is not created yet
        instance = new PhonemeUtility();  // Create a new instance and assign it to the pointer
    }
    return instance;  // Return the pointer to the instance
}

std::string PhonemeUtility::generate_random_phonemes(const int num) {
    srand(time(NULL));
    std::stringstream ss;
    for (int i = 0; i < num; i++) {
        int r = rand() % 10;
        ss << r << "-";
    }
    std::string str = ss.str();
    str.pop_back();
    return str;
}

// A function that takes a string and returns a string that maps that string to phonemes with "-" in between
std::string PhonemeUtility::string_to_phonemes(const std::string& phonemes) {
    std::string result;
    auto splitted = phonemes.substr(0, phonemes.size() / 5);

    for (size_t i = 0; i < splitted.size(); i++) {
        if (splitted[i] == '-') {
            continue;  // skip the dash
        }
        std::string phoneme(1, splitted[i]);  // make a string from one character
        auto it = phoneme_map.find(phoneme);
        if (it != phoneme_map.end()) {
            if (!result.empty()) {
                result += '-';
            }
            result += std::to_string(it->second);
        }
    }

    return result;
}