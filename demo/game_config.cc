// Demonstrates loading a game configuration from JSON into nested structs
// with enums, optionals, vectors, and maps — all via reflection.

#include <iostream>
#include "../simple_json.hpp"

enum class Difficulty { Easy, Normal, Hard, Nightmare };
enum class AudioBackend { OpenAL, FMOD, PulseAudio };

struct Resolution {
    int width;
    int height;
};

struct GraphicsSettings {
    Resolution resolution;
    bool fullscreen;
    bool vsync;
    int fps_cap;
};

struct AudioSettings {
    AudioBackend backend;
    double master_volume;
    double music_volume;
    double sfx_volume;
    bool mute;
};

struct Keybind {
    std::string action;
    std::string key;
};

struct GameConfig {
    std::string player_name;
    Difficulty difficulty;
    GraphicsSettings graphics;
    AudioSettings audio;
    std::vector<Keybind> keybinds;
    std::map<std::string, int> highscores;
    std::optional<std::string> last_save_file;
};

static constexpr auto config_json = R"({
    "player_name": "DragonSlayer42",
    "difficulty": "Hard",
    "graphics": {
        "resolution": { "width": 1920, "height": 1080 },
        "fullscreen": true,
        "vsync": true,
        "fps_cap": 144
    },
    "audio": {
        "backend": "PulseAudio",
        "master_volume": 0.8,
        "music_volume": 0.6,
        "sfx_volume": 1.0,
        "mute": false
    },
    "keybinds": [
        { "action": "jump",   "key": "Space" },
        { "action": "attack", "key": "LeftClick" },
        { "action": "dodge",  "key": "Shift" }
    ],
    "highscores": {
        "level_1": 15200,
        "level_2": 8700,
        "boss_rush": 42000
    }
})";

int main() {
    // Parse JSON
    auto parsed = sj::parse(config_json);
    if (!parsed) {
        std::cerr << "Parse error: " << parsed.error().message << "\n";
        return 1;
    }

    // Deserialize into GameConfig — all fields resolved via reflection
    auto result = sj::from_json<GameConfig>(*parsed);
    if (!result) {
        std::cerr << "Deserialization error: " << result.error().message
                  << " at path: " << result.error().path << "\n";
        return 1;
    }

    GameConfig const& cfg = *result;

    std::cout << "=== Game Configuration ===\n\n";
    std::cout << "Player:     " << cfg.player_name << "\n";
    std::cout << "Difficulty: " << sj::to_json(cfg.difficulty).as_string() << "\n";
    std::cout << "Resolution: " << cfg.graphics.resolution.width
              << "x" << cfg.graphics.resolution.height << "\n";
    std::cout << "Fullscreen: " << std::boolalpha << cfg.graphics.fullscreen << "\n";
    std::cout << "VSync:      " << cfg.graphics.vsync << "\n";
    std::cout << "FPS cap:    " << cfg.graphics.fps_cap << "\n\n";

    std::cout << "Audio backend: " << sj::to_json(cfg.audio.backend).as_string() << "\n";
    std::cout << "Master vol:    " << cfg.audio.master_volume << "\n";
    std::cout << "Music vol:     " << cfg.audio.music_volume << "\n";
    std::cout << "SFX vol:       " << cfg.audio.sfx_volume << "\n\n";

    std::cout << "Keybinds:\n";
    for (auto const& kb : cfg.keybinds)
        std::cout << "  " << kb.action << " -> " << kb.key << "\n";

    std::cout << "\nHighscores:\n";
    for (auto const& [level, score] : cfg.highscores)
        std::cout << "  " << level << ": " << score << "\n";

    if (cfg.last_save_file)
        std::cout << "\nLast save: " << *cfg.last_save_file << "\n";
    else
        std::cout << "\nNo previous save file.\n";

    // Round-trip: serialize back to JSON
    sj::json roundtrip = sj::to_json(cfg);
    std::cout << "\n=== Round-trip JSON ===\n" << roundtrip.dump(2) << "\n";

    return 0;
}
