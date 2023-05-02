#include <iostream>
#include <string>
#include <unordered_map>
#include <cstdlib> // for rand and srand
#include <ctime> // for time
#include <string> // for string
#include <sstream> // for stringstream

// A singleton class that provides utility functions for phoneme mapping
class PhonemeUtility {
private:
    // A map that stores the phoneme values
    std::unordered_map<std::string, int> phoneme_map;

    // A private constructor to prevent creating multiple instances
    PhonemeUtility();

    // A private copy constructor to prevent copying the instance
    PhonemeUtility(const PhonemeUtility&);

    // A private assignment operator to prevent assigning the instance
    PhonemeUtility& operator=(const PhonemeUtility&);

    // A static pointer to store the single instance
    static PhonemeUtility* instance;

public:
    // A public destructor to delete the instance
    ~PhonemeUtility();

    // A public static function to get the single instance
    static PhonemeUtility* get_instance();

    // A function that takes a string and returns a string that maps that string to phonemes with "-" in between
    std::string string_to_phonemes(const std::string& s);

    std::string generate_random_phonemes(const int num);
};