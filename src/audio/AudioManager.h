#pragma once
#include <string>

class AudioManager {
public:
    static bool init(); // инициализация
    static void cleanup(); // выгрузка из памяти музыки

    // функция проигрывания звуков передается путь к файлу
    static void playSound(const std::string& filepath);

    // для фоновой музыки
    static void playMusic(const std::string& filepath);
    static void stopMusic();
};