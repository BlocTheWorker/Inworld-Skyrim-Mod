// PhonemeUtility.cpp
#include "PhonemeUtility.h"

PhonemeUtility* PhonemeUtility::instance = nullptr;

PhonemeUtility::PhonemeUtility() {
    phoneme_map = {{"Aah", 0},   {"BigAah", 1}, {"BMP", 2}, {"ChJSh", 3}, {"DST", 4}, {"Eee", 5},
                   {"Eh", 6},    {"FV", 7},     {"I", 8},   {"K", 9},     {"N", 10},  {"Oh", 11},
                   {"OohQ", 12}, {"R", 13},     {"Th", 14}, {"W", 15}};
}

PhonemeUtility::PhonemeUtility(const PhonemeUtility&) {}
PhonemeUtility& PhonemeUtility::operator=(const PhonemeUtility&) {
    return *this;
}
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
std::string PhonemeUtility::string_to_phonemes(const std::string& s) {
    std::string result;                             // The result string
    std::string word;                               // A temporary word to store each phoneme
    for (char c : s) {                              // Loop through each character in the string
        if (c == ' ' || c == '\n') {                // If the character is a space or a newline
            if (!word.empty()) {                    // If the word is not empty
                if (phoneme_map.count(word) > 0) {  // If the word is a valid phoneme
                    if (!result.empty()) {          // If the result is not empty
                        result += "-";              // Add a "-" to the result
                    }
                    result += std::to_string(phoneme_map[word]);  // Add the phoneme value to the result string
                } else {                                          // If the word is not a valid phoneme
                    std::cout << "Invalid phoneme: " << word << std::endl;  // Print an error message
                }
                word.clear();  // Clear the word
            }
        } else {        // If the character is not a space or a newline
            word += c;  // Append the character to the word
        }
    }
    if (!word.empty()) {                    // If the word is not empty after the loop
        if (phoneme_map.count(word) > 0) {  // If the word is a valid phoneme
            if (!result.empty()) {          // If the result is not empty
                result += "-";              // Add a "-" to the result
            }
            result += std::to_string(phoneme_map[word]);            // Add the phoneme value to the result string
        } else {                                                    // If the word is not a valid phoneme
            std::cout << "Invalid phoneme: " << word << std::endl;  // Print an error message
        }
        word.clear();  // Clear the word
    }
    return result;  // Return the result string
}