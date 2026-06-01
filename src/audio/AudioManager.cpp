#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include "AudioManager.h"
#include <iostream>

// глобальные обьекты аудио движка тут именно для звуков
static ma_engine engine;
static bool isInitialized = false;

// а тут именно для музыки
static ma_sound bgmSound;
static bool isMusicLoaded = false;

bool AudioManager::init() {
	// инициализация аудиодвижка с настройками по умолчанию
	ma_result result = ma_engine_init(NULL, &engine);

    if (result != MA_SUCCESS) {
        std::cerr << "ERROR::AUDIO: Failed to initialize audio engine." << std::endl;
        return false;
    }
    isInitialized = true;
    std::cout << "Audio Engine initialized successfully!" << std::endl;
    return true;
}

// выгрузка из памяти музыки
void AudioManager::cleanup() {
    if (isMusicLoaded) {
        ma_sound_uninit(&bgmSound);
    }

    if (isInitialized) {
        ma_engine_uninit(&engine);
        isInitialized = false;
    }
}

void AudioManager::playSound(const std::string& filepath) {
    if (isInitialized) {
        ma_engine_play_sound(&engine, filepath.c_str(), NULL);
    }
}

// фоновая музыка
void AudioManager::playMusic(const std::string& filepath) {
    if (!isInitialized) return;

    // если музыка уже играет
    if (isMusicLoaded) {
        // выгружаем чтобы новую включить
        ma_sound_uninit(&bgmSound);
        isMusicLoaded = false;
    }

    // инициализация звука из файла
    ma_result result = ma_sound_init_from_file(&engine, filepath.c_str(), 0, NULL, NULL, &bgmSound);

    if (result == MA_SUCCESS) {
        isMusicLoaded = true;

        ma_sound_set_looping(&bgmSound, MA_TRUE); // включаем бесконечный повтор
        ma_sound_set_volume(&bgmSound, 0.2f); // делаем громкость 20%

        // воспроизведение музыки
        ma_sound_start(&bgmSound);
    }
    else { // ошибкоэээ
        std::cerr << "ERROR::AUDIO: Failed to load background music: " << filepath << std::endl;
    }
}

void AudioManager::stopMusic() {
    if (isMusicLoaded) {
        ma_sound_stop(&bgmSound);
    }
}