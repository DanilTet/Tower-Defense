#pragma once
#include <string>

class SettingsManager {
public:
    static void init();
    static void load();
    static void save();

    static float getVolume();
    static void setVolume(float volume);

    static bool isMuted();
    static void setMuted(bool muted);

    static std::string getLanguage();
    static void setLanguage(const std::string& lang);

private:
    static float s_masterVolume;
    static bool s_isMuted;
    static std::string s_language; // "ru", "ua", "en"
    static bool s_loaded;
};

