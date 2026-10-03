#include "LocalizationManager.h"
#include "SettingsManager.h"
#include <unordered_map>

static GameLanguage s_currentLanguage = GameLanguage::Russian;

// Словари для каждого языка
static const std::unordered_map<std::string, std::string> s_dictRu = {
    { "BTN_START_GAME", "В БОЙ!" },
    { "BTN_LOAD_GAME", "ЗАГРУЗИТЬ ИГРУ" },
    { "BTN_LEVEL_SELECT", "ВЫБОР УРОВНЯ" },
    { "BTN_MAP_EDITOR", "РЕДАКТОР КАРТ" },
    { "BTN_SETTINGS", "НАСТРОЙКИ" },
    { "BTN_EXIT", "ВЫХОД" },

    { "SETTINGS_TITLE", "НАСТРОЙКИ" },
    { "SETTINGS_VOLUME", "Громкость звука:" },
    { "SETTINGS_LANGUAGE", "Язык:" },
    { "SETTINGS_UI_SCALE", "Масштаб интерфейса:" },
    { "SETTINGS_CLOSE", "Закрыть" },

    { "PAUSE_TITLE", "ПАУЗА" },
    { "PAUSE_RESUME", "Продолжить" },
    { "PAUSE_SAVE", "Сохранить игру" },
    { "PAUSE_EXIT", "Выйти в меню" },

    { "LEVEL_TITLE", "ВЫБОР УРОВНЯ" },
    { "LEVEL_TAB_ALL", "ВСЕ" },
    { "LEVEL_TAB_CAMPAIGN", "КАМПАНИЯ" },
    { "LEVEL_TAB_TEST", "ТЕСТОВЫЕ" },
    { "LEVEL_SEARCH_HINT", "Поиск по имени или тегу..." },
    { "LEVEL_BADGE_CAMPAIGN", "КАМПАНИЯ" },
    { "LEVEL_BADGE_TEST", "ТЕСТОВЫЙ" },
    { "LEVEL_BTN_RENAME", "Имя" },
    { "LEVEL_BTN_TAGS", "Теги" },
    { "LEVEL_BTN_PLAY", "ИГРАТЬ" },
    { "LEVEL_BTN_CREATE", "+ СОЗДАТЬ КАРТУ" },
    { "LEVEL_BTN_BACK", "< НАЗАД" },
    { "LEVEL_BTN_PREV", "< НАЗАД" },
    { "LEVEL_BTN_NEXT", "ВПЕРЕД >" },
    { "LEVEL_PAGE", "Стр." },
    { "LEVEL_TOGGLE_TYPE", "Тип" },

    { "TAGS_TITLE", "ТЕГИ УРОВНЯ" },
    { "TAGS_ADD_HINT", "Новый тег..." },
    { "TAGS_ADD_BTN", "+ Добавить" },
    { "TAGS_CLOSE", "Готово" },
    { "TAGS_POPULAR", "Быстрые теги:" },

    { "RENAME_TITLE", "ПЕРЕИМЕНОВАТЬ КАРТУ" },
    { "RENAME_HINT", "Введите название карты..." },
    { "RENAME_SAVE", "Сохранить" },
    { "RENAME_CANCEL", "Отмена" },

    { "EDITOR_TITLE", "РЕДАКТОР КАРТ" },
    { "EDITOR_GROUND", "Земля" },
    { "EDITOR_WALL", "Стена" },
    { "EDITOR_PLATFORM", "Платформа" },
    { "EDITOR_PATH", "Дорога" },
    { "EDITOR_SPAWNER", "Спавнер" },
    { "EDITOR_BASE", "База" },
    { "EDITOR_ERASER", "Ластик" },
    { "EDITOR_WAVES", "Волны (W)" },
    { "EDITOR_SAVE", "Сохранить (S)" },
    { "EDITOR_TEST", "Тест (T)" },
    { "EDITOR_CLEAR", "Очистить (C)" },
    { "EDITOR_EXIT", "Выход (ESC)" },
    { "EDITOR_NEW_MAP", "+ Новый" },
    { "EDITOR_MAPS_LIST", "Карты" },
    { "EDITOR_TYPE_CAMPAIGN", "Кампания" },
    { "EDITOR_TYPE_TEST", "Тестовый" },
    { "EDITOR_EXIT_TITLE", "ВЫХОД ИЗ РЕДАКТОРА" },
    { "EDITOR_EXIT_QUESTION", "Вы точно хотите выйти?" },
    { "EDITOR_EXIT_SUB", "Несохранённые изменения будут потеряны." },
    { "EDITOR_EXIT_SAVE_AND_EXIT", "Сохранить и выйти" },
    { "EDITOR_EXIT_DISCARD", "Выйти без сохранения" },
    { "EDITOR_EXIT_CANCEL", "Отмена" }
};

static const std::unordered_map<std::string, std::string> s_dictUa = {
    { "BTN_START_GAME", "У БІЙ!" },
    { "BTN_LOAD_GAME", "ЗАВАНТАЖИТИ ГРУ" },
    { "BTN_LEVEL_SELECT", "ВИБІР РІВНЯ" },
    { "BTN_MAP_EDITOR", "РЕДАКТОР КАРТ" },
    { "BTN_SETTINGS", "НАЛАШТУВАННЯ" },
    { "BTN_EXIT", "ВИХІД" },

    { "SETTINGS_TITLE", "НАЛАШТУВАННЯ" },
    { "SETTINGS_VOLUME", "Гучність звуку:" },
    { "SETTINGS_LANGUAGE", "Мова:" },
    { "SETTINGS_UI_SCALE", "Масштаб інтерфейсу:" },
    { "SETTINGS_CLOSE", "Закрити" },

    { "PAUSE_TITLE", "ПАУЗА" },
    { "PAUSE_RESUME", "Продовжити" },
    { "PAUSE_SAVE", "Зберегти гру" },
    { "PAUSE_EXIT", "Вийти в меню" },

    { "LEVEL_TITLE", "ВИБІР РІВНЯ" },
    { "LEVEL_TAB_ALL", "ВСІ" },
    { "LEVEL_TAB_CAMPAIGN", "КАМПАНІЯ" },
    { "LEVEL_TAB_TEST", "ТЕСТОВІ" },
    { "LEVEL_SEARCH_HINT", "Пошук за назвою чи тегом..." },
    { "LEVEL_BADGE_CAMPAIGN", "КАМПАНІЯ" },
    { "LEVEL_BADGE_TEST", "ТЕСТОВИЙ" },
    { "LEVEL_BTN_RENAME", "Ім'я" },
    { "LEVEL_BTN_TAGS", "Теги" },
    { "LEVEL_BTN_PLAY", "ГРАТИ" },
    { "LEVEL_BTN_CREATE", "+ СТВОРИТИ КАРТУ" },
    { "LEVEL_BTN_BACK", "< НАЗАД" },
    { "LEVEL_BTN_PREV", "< НАЗАД" },
    { "LEVEL_BTN_NEXT", "ВПЕРЕД >" },
    { "LEVEL_PAGE", "Стор." },
    { "LEVEL_TOGGLE_TYPE", "Тип" },

    { "TAGS_TITLE", "ТЕГИ РІВНЯ" },
    { "TAGS_ADD_HINT", "Новий тег..." },
    { "TAGS_ADD_BTN", "+ Додати" },
    { "TAGS_CLOSE", "Готово" },
    { "TAGS_POPULAR", "Швидкі теги:" },

    { "RENAME_TITLE", "ПЕРЕЙМЕНУВАТИ КАРТУ" },
    { "RENAME_HINT", "Введіть назву карти..." },
    { "RENAME_SAVE", "Зберегти" },
    { "RENAME_CANCEL", "Скасувати" },

    { "EDITOR_TITLE", "РЕДАКТОР КАРТ" },
    { "EDITOR_GROUND", "Земля" },
    { "EDITOR_WALL", "Стіна" },
    { "EDITOR_PLATFORM", "Платформа" },
    { "EDITOR_PATH", "Дорога" },
    { "EDITOR_SPAWNER", "Спавнер" },
    { "EDITOR_BASE", "База" },
    { "EDITOR_ERASER", "Гумка" },
    { "EDITOR_WAVES", "Хвилі (W)" },
    { "EDITOR_SAVE", "Зберегти (S)" },
    { "EDITOR_TEST", "Тест (T)" },
    { "EDITOR_CLEAR", "Очистити (C)" },
    { "EDITOR_EXIT", "Вихід (ESC)" },
    { "EDITOR_NEW_MAP", "+ Новий" },
    { "EDITOR_MAPS_LIST", "Карти" },
    { "EDITOR_TYPE_CAMPAIGN", "Кампанія" },
    { "EDITOR_TYPE_TEST", "Тестовий" },
    { "EDITOR_EXIT_TITLE", "ВИХІД З РЕДАКТОРА" },
    { "EDITOR_EXIT_QUESTION", "Ви точно хочете вийти?" },
    { "EDITOR_EXIT_SUB", "Незбережені зміни буде втрачено." },
    { "EDITOR_EXIT_SAVE_AND_EXIT", "Зберегти і вийти" },
    { "EDITOR_EXIT_DISCARD", "Вийти без збереження" },
    { "EDITOR_EXIT_CANCEL", "Скасувати" }
};

static const std::unordered_map<std::string, std::string> s_dictEn = {
    { "BTN_START_GAME", "START GAME" },
    { "BTN_LOAD_GAME", "LOAD GAME" },
    { "BTN_LEVEL_SELECT", "LEVEL SELECT" },
    { "BTN_MAP_EDITOR", "MAP EDITOR" },
    { "BTN_SETTINGS", "SETTINGS" },
    { "BTN_EXIT", "EXIT" },

    { "SETTINGS_TITLE", "SETTINGS" },
    { "SETTINGS_VOLUME", "Sound volume:" },
    { "SETTINGS_LANGUAGE", "Language:" },
    { "SETTINGS_UI_SCALE", "UI Scale:" },
    { "SETTINGS_CLOSE", "Close" },

    { "PAUSE_TITLE", "PAUSED" },
    { "PAUSE_RESUME", "Resume" },
    { "PAUSE_SAVE", "Save Game" },
    { "PAUSE_EXIT", "Main Menu" },

    { "LEVEL_TITLE", "SELECT MISSION" },
    { "LEVEL_TAB_ALL", "ALL" },
    { "LEVEL_TAB_CAMPAIGN", "CAMPAIGN" },
    { "LEVEL_TAB_TEST", "CUSTOM / TEST" },
    { "LEVEL_SEARCH_HINT", "Search by name or tag..." },
    { "LEVEL_BADGE_CAMPAIGN", "CAMPAIGN" },
    { "LEVEL_BADGE_TEST", "CUSTOM / TEST" },
    { "LEVEL_BTN_RENAME", "Rename" },
    { "LEVEL_BTN_TAGS", "Tags" },
    { "LEVEL_BTN_PLAY", "PLAY" },
    { "LEVEL_BTN_CREATE", "+ CREATE MAP" },
    { "LEVEL_BTN_BACK", "< BACK" },
    { "LEVEL_BTN_PREV", "< PREV" },
    { "LEVEL_BTN_NEXT", "NEXT >" },
    { "LEVEL_PAGE", "Page" },
    { "LEVEL_TOGGLE_TYPE", "Type" },

    { "TAGS_TITLE", "LEVEL TAGS" },
    { "TAGS_ADD_HINT", "New tag..." },
    { "TAGS_ADD_BTN", "+ Add" },
    { "TAGS_CLOSE", "Done" },
    { "TAGS_POPULAR", "Quick tags:" },

    { "RENAME_TITLE", "RENAME LEVEL" },
    { "RENAME_HINT", "Enter level name..." },
    { "RENAME_SAVE", "Save" },
    { "RENAME_CANCEL", "Cancel" },

    { "EDITOR_TITLE", "MAP EDITOR" },
    { "EDITOR_GROUND", "Ground" },
    { "EDITOR_WALL", "Wall" },
    { "EDITOR_PLATFORM", "Platform" },
    { "EDITOR_PATH", "Path" },
    { "EDITOR_SPAWNER", "Spawner" },
    { "EDITOR_BASE", "Base" },
    { "EDITOR_ERASER", "Eraser" },
    { "EDITOR_WAVES", "Waves (W)" },
    { "EDITOR_SAVE", "Save (S)" },
    { "EDITOR_TEST", "Test (T)" },
    { "EDITOR_CLEAR", "Clear (C)" },
    { "EDITOR_EXIT", "Exit (ESC)" },
    { "EDITOR_NEW_MAP", "+ New" },
    { "EDITOR_MAPS_LIST", "Maps" },
    { "EDITOR_TYPE_CAMPAIGN", "Campaign" },
    { "EDITOR_TYPE_TEST", "Test" },
    { "EDITOR_EXIT_TITLE", "EXIT EDITOR" },
    { "EDITOR_EXIT_QUESTION", "Are you sure you want to exit?" },
    { "EDITOR_EXIT_SUB", "Unsaved changes will be lost." },
    { "EDITOR_EXIT_SAVE_AND_EXIT", "Save & Exit" },
    { "EDITOR_EXIT_DISCARD", "Exit without saving" },
    { "EDITOR_EXIT_CANCEL", "Cancel" }
};

void LocalizationManager::init() {
    std::string savedLang = SettingsManager::getLanguage();
    setLanguageByCode(savedLang);
}

void LocalizationManager::setLanguage(GameLanguage lang) {
    s_currentLanguage = lang;
    SettingsManager::setLanguage(getLanguageCode());
}

void LocalizationManager::setLanguageByCode(const std::string& code) {
    if (code == "ua") {
        s_currentLanguage = GameLanguage::Ukrainian;
    } else if (code == "en") {
        s_currentLanguage = GameLanguage::English;
    } else {
        s_currentLanguage = GameLanguage::Russian;
    }
}

GameLanguage LocalizationManager::getLanguage() {
    return s_currentLanguage;
}

std::string LocalizationManager::getLanguageCode() {
    switch (s_currentLanguage) {
        case GameLanguage::Ukrainian: return "ua";
        case GameLanguage::English: return "en";
        default: return "ru";
    }
}

std::string LocalizationManager::get(const std::string& key) {
    const std::unordered_map<std::string, std::string>* activeDict = &s_dictRu;
    if (s_currentLanguage == GameLanguage::Ukrainian) {
        activeDict = &s_dictUa;
    } else if (s_currentLanguage == GameLanguage::English) {
        activeDict = &s_dictEn;
    }

    auto it = activeDict->find(key);
    if (it != activeDict->end()) {
        return it->second;
    }

    // Фоллбэк на русский
    auto itRu = s_dictRu.find(key);
    if (itRu != s_dictRu.end()) {
        return itRu->second;
    }

    // Фоллбэк на ключ
    return key;
}

