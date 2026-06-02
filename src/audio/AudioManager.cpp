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

// буффер звуков или пул еще назіваеют
const int MAX_SOUNDS = 32; // одновременно может звучать до 32 звуков
static ma_sound sfxPool[MAX_SOUNDS]; // массив звуковых объектов
static bool sfxAllocated[MAX_SOUNDS]; // флаг занят ли слот
static int currentSfxIndex = 0; // текущий слод для нового звука

bool AudioManager::init() {
	// инициализация аудиодвижка с настройками по умолчанию
	ma_result result = ma_engine_init(NULL, &engine);

    if (result != MA_SUCCESS) {
        std::cerr << "ERROR::AUDIO: Failed to initialize audio engine." << std::endl;
        return false;
    }

    // обнуляем все слоты в буфере
    for (int i = 0; i < MAX_SOUNDS; i++) {
        sfxAllocated[i] = false;
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

    // очищаем буффер
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (sfxAllocated[i]) {
            ma_sound_uninit(&sfxPool[i]);
        }
    }

    if (isInitialized) {
        ma_engine_uninit(&engine);
        isInitialized = false;
    }
}

void AudioManager::playSound(const std::string& filepath,  float volume) {
    if (!isInitialized) return;

    // если в слоте есть уже звук то выгружаем его из памяти
    if (sfxAllocated[currentSfxIndex]) {
        ma_sound_uninit(&sfxPool[currentSfxIndex]);
        sfxAllocated[currentSfxIndex] = false;
    }

    // загружаем новый звук в текущий слот буфера
    ma_result result = ma_sound_init_from_file(&engine, filepath.c_str(), 0, NULL, NULL, &sfxPool[currentSfxIndex]);

    if (result == MA_SUCCESS) {
        sfxAllocated[currentSfxIndex] = true;

        // устанавливаем громкость 0.0f єто тишина ноль просто, а 1.0f максимум 
        ma_sound_set_volume(&sfxPool[currentSfxIndex], volume);

        // запускаем звук
        ma_sound_start(&sfxPool[currentSfxIndex]);
    }
    else {
        std::cerr << "ERROR::AUDIO: Failed to load sound: " << filepath << std::endl;
    }
    // пускаем индекс по кругу
    currentSfxIndex = (currentSfxIndex + 1) % MAX_SOUNDS;
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
        ma_sound_set_volume(&bgmSound, 0.1f); // делаем громкость 20%

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