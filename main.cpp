#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Nokia Drive: a small retro top-down 2D driving game made with raylib.
// Build in the MSYS2 UCRT64 terminal (keep the MP3 and PNG beside the EXE):
// windres NokiaDrive.rc -O coff -o NokiaDrive_icon.o
// g++ main.cpp NokiaDrive_icon.o -o NokiaDrive.exe -std=c++17 -O2 -lraylib -lopengl32 -lgdi32 -lwinmm

namespace {
constexpr int WINDOW_W = 960;
constexpr int WINDOW_H = 600;
constexpr int VIEW_W = 320;
constexpr int VIEW_H = 200;
constexpr float ROAD_LEFT = 56.0f;
constexpr float ROAD_RIGHT = 264.0f;
constexpr float PLAYER_Y = 151.0f;
constexpr float CAR_W = 13.0f;
constexpr float CAR_H = 21.0f;

const Color LCD       = {190, 211, 145, 255};
const Color LCD_LIGHT = {218, 231, 183, 255};
const Color LCD_MID   = {130, 157, 101, 255};
const Color LCD_DARK  = {77, 105, 64, 255};
const Color INK       = {30, 51, 31, 255};

struct TrafficCar {
    float x = 0.0f;
    float y = 0.0f;
    int lane = 0;
    int style = 0;
};

enum class Screen { MENU, CONTROLS, SETTINGS, DRIVING, GAME_OVER };

Rectangle menuButton(int index) {
    return {94.0f, 76.0f + index * 23.0f, 132.0f, 18.0f};
}

Rectangle backButton() {
    return {110.0f, 166.0f, 100.0f, 18.0f};
}

float laneCenter(int lane) {
    return ROAD_LEFT + (ROAD_RIGHT - ROAD_LEFT) * (lane + 0.5f) / 3.0f;
}

Rectangle playerRect(float x) {
    return {x - CAR_W * 0.5f, PLAYER_Y - CAR_H * 0.5f, CAR_W, CAR_H};
}

Rectangle trafficRect(const TrafficCar& car) {
    return {car.x - 6.5f, car.y - 10.0f, 13.0f, 20.0f};
}

void drawButton(Rectangle r, const char* label, bool selected) {
    DrawRectangleRec(r, selected ? INK : LCD_LIGHT);
    DrawRectangleLinesEx(r, 1.0f, INK);
    const int textWidth = MeasureText(label, 9);
    DrawText(label,
             static_cast<int>(r.x + (r.width - textWidth) / 2.0f),
             static_cast<int>(r.y + 4.0f), 9,
             selected ? LCD_LIGHT : INK);
}

void drawCar(float centerX, float centerY, bool player, int style = 0) {
    const int x = static_cast<int>(std::round(centerX));
    const int y = static_cast<int>(std::round(centerY));
    const Color body = player ? INK : (style == 1 ? LCD_DARK : LCD_LIGHT);
    const Color glass = player ? LCD_LIGHT : INK;
    const Color detail = player ? LCD_MID : LCD_DARK;

    // Wheels
    DrawRectangle(x - 7, y - 7, 2, 5, INK);
    DrawRectangle(x + 5, y - 7, 2, 5, INK);
    DrawRectangle(x - 7, y + 3, 2, 5, INK);
    DrawRectangle(x + 5, y + 3, 2, 5, INK);

    // Body, windows and lights
    DrawRectangle(x - 5, y - 10, 10, 20, INK);
    DrawRectangle(x - 4, y - 9, 8, 18, body);
    DrawRectangle(x - 3, y - 6, 6, 4, glass);
    DrawRectangle(x - 3, y + 2, 6, 3, detail);
    DrawRectangle(x - 3, y - 10, 6, 1, player ? LCD_LIGHT : INK);
    DrawRectangle(x - 3, y + 9, 6, 1, player ? LCD_LIGHT : INK);
    if (player) DrawRectangle(x - 1, y - 8, 2, 1, INK);
}

void drawRoad(float scrollOffset, float playerX,
              const std::vector<TrafficCar>& traffic,
              float speed, float distanceKm, int maxSpeed) {
    ClearBackground(LCD);

    DrawRectangle(0, 22, VIEW_W, VIEW_H - 22, LCD);
    DrawRectangle(static_cast<int>(ROAD_LEFT - 4), 22,
                  static_cast<int>(ROAD_RIGHT - ROAD_LEFT + 8), VIEW_H - 22, LCD_DARK);
    DrawRectangle(static_cast<int>(ROAD_LEFT), 22,
                  static_cast<int>(ROAD_RIGHT - ROAD_LEFT), VIEW_H - 22, LCD_MID);

    DrawLine(static_cast<int>(ROAD_LEFT), 22, static_cast<int>(ROAD_LEFT), VIEW_H, INK);
    DrawLine(static_cast<int>(ROAD_RIGHT), 22, static_cast<int>(ROAD_RIGHT), VIEW_H, INK);

    const int dashSpacing = 28;
    const int dashHeight = 13;
    const int offset = static_cast<int>(scrollOffset) % dashSpacing;
    for (int laneLine = 1; laneLine <= 2; ++laneLine) {
        const int x = static_cast<int>(ROAD_LEFT +
            (ROAD_RIGHT - ROAD_LEFT) * laneLine / 3.0f);
        for (int y = 24 - offset; y < VIEW_H; y += dashSpacing) {
            DrawRectangle(x - 1, y, 2, dashHeight, LCD_LIGHT);
        }
    }

    const int sideOffset = (offset * 2) % 42;
    for (int y = 29 - sideOffset; y < VIEW_H; y += 42) {
        DrawRectangle(35, y, 5, 2, LCD_DARK);
        DrawRectangle(280, y + 12, 5, 2, LCD_DARK);
    }

    // Dashboard
    DrawRectangle(0, 0, VIEW_W, 22, LCD_LIGHT);
    DrawLine(0, 21, VIEW_W, 21, INK);
    DrawText(TextFormat("SPEED %03d", static_cast<int>(speed)), 7, 7, 8, INK);
    DrawText("KM/H", 91, 7, 8, INK);
    DrawText(TextFormat("DIST %.2f KM", distanceKm), 161, 7, 8, INK);

    DrawRectangle(270, 6, 42, 10, LCD);
    DrawRectangleLines(270, 6, 42, 10, INK);
    const float fill = maxSpeed > 0
        ? std::clamp(speed / static_cast<float>(maxSpeed), 0.0f, 1.0f)
        : 0.0f;
    DrawRectangle(272, 8, static_cast<int>(38.0f * fill), 6, INK);

    for (const TrafficCar& car : traffic) drawCar(car.x, car.y, false, car.style);
    drawCar(playerX, PLAYER_Y, true);
}

void drawMenu(int selected, bool musicEnabled) {
    ClearBackground(LCD);
    DrawRectangleLinesEx({8, 8, 304, 184}, 2.0f, INK);
    DrawText("NOKIA DRIVE", 76, 28, 20, INK);
    DrawLine(55, 54, 265, 54, INK);
    DrawText("2D MOTOR SIMULATOR", 88, 60, 8, LCD_DARK);
    const char* labels[] = {"START DRIVE", "CONTROLS", "SETTINGS", "EXIT"};
    for (int i = 0; i < 4; ++i) drawButton(menuButton(i), labels[i], i == selected);
    DrawText(musicEnabled ? "MUSIC: ON (M TO TOGGLE)" : "MUSIC: OFF (M TO TOGGLE)",
             75, 177, 8, INK);
}

void drawControls() {
    ClearBackground(LCD);
    DrawRectangleLinesEx({8, 8, 304, 184}, 2.0f, INK);
    DrawText("HOW TO DRIVE", 89, 21, 15, INK);
    DrawLine(26, 43, 294, 43, INK);
    DrawText("UP / W       ACCELERATE", 30, 56, 9, INK);
    DrawText("DOWN / S     BRAKE", 30, 76, 9, INK);
    DrawText("LEFT / A     STEER LEFT", 30, 96, 9, INK);
    DrawText("RIGHT / D    STEER RIGHT", 30, 116, 9, INK);
    DrawText("ESC          PAUSE / RESUME", 30, 136, 8, INK);
    DrawText("R            RESTART DRIVE", 30, 149, 8, INK);
    DrawText("M            MUSIC ON/OFF", 30, 160, 8, INK);
    drawButton(backButton(), "BACK", true);
}

void drawSettings(int selected, int difficulty, int density) {
    ClearBackground(LCD);
    DrawRectangleLinesEx({8, 8, 304, 184}, 2.0f, INK);
    DrawText("SETTINGS", 115, 22, 16, INK);
    DrawLine(26, 44, 294, 44, INK);

    const char* difficultyNames[] = {"EASY", "NORMAL", "HARD"};
    const char* densityNames[] = {"LOW", "NORMAL", "BUSY"};
    const Rectangle diffRow = {25, 58, 270, 27};
    const Rectangle trafficRow = {25, 94, 270, 27};

    DrawRectangleRec(diffRow, selected == 0 ? INK : LCD_LIGHT);
    DrawRectangleLinesEx(diffRow, 1.0f, INK);
    const Color diffText = selected == 0 ? LCD_LIGHT : INK;
    DrawText("DIFFICULTY", 34, 67, 9, diffText);
    DrawText("<", 190, 67, 10, diffText);
    DrawText(difficultyNames[difficulty], 205, 67, 9, diffText);
    DrawText(">", 278, 67, 10, diffText);

    DrawRectangleRec(trafficRow, selected == 1 ? INK : LCD_LIGHT);
    DrawRectangleLinesEx(trafficRow, 1.0f, INK);
    const Color trafficText = selected == 1 ? LCD_LIGHT : INK;
    DrawText("TRAFFIC", 34, 103, 9, trafficText);
    DrawText("<", 190, 103, 10, trafficText);
    DrawText(densityNames[density], 205, 103, 9, trafficText);
    DrawText(">", 278, 103, 10, trafficText);

    drawButton({94, 139, 132, 18}, "BACK", selected == 2);
    DrawText("USE LEFT / RIGHT TO CHANGE", 76, 170, 8, INK);
}

void resetDrive(float& playerX, float& speed, float& distanceKm,
                float& roadOffset, float& spawnTimer,
                std::vector<TrafficCar>& traffic) {
    playerX = (ROAD_LEFT + ROAD_RIGHT) * 0.5f;
    speed = 0.0f;
    distanceKm = 0.0f;
    roadOffset = 0.0f;
    spawnTimer = 0.0f;
    traffic.clear();
}

void drawPauseOverlay(int selected) {
    DrawRectangle(62, 52, 196, 100, LCD_LIGHT);
    DrawRectangleLinesEx({62, 52, 196, 100}, 2.0f, INK);
    DrawText("PAUSED", 126, 61, 15, INK);
    drawButton({91, 83, 138, 17}, "RESUME", selected == 0);
    drawButton({91, 104, 138, 17}, "RESTART", selected == 1);
    drawButton({91, 125, 138, 17}, "MAIN MENU", selected == 2);
}

void drawGameOver(float distanceKm, int selected) {
    ClearBackground(LCD);
    DrawRectangleLinesEx({8, 8, 304, 184}, 2.0f, INK);
    DrawText("TRIP ENDED", 91, 32, 18, INK);
    DrawText("YOU HIT ANOTHER CAR", 83, 59, 9, INK);
    DrawText(TextFormat("DISTANCE: %.2f KM", distanceKm), 83, 77, 9, INK);
    drawButton({94, 104, 132, 18}, "DRIVE AGAIN", selected == 0);
    drawButton({94, 127, 132, 18}, "MAIN MENU", selected == 1);
    DrawText("PRESS ENTER TO SELECT", 89, 165, 8, INK);
}
} // namespace

int main() {
    InitWindow(WINDOW_W, WINDOW_H, "Nokia Drive - 2D Car Simulator");

    // Set the game window icon. Keep nokia_drive_icon.png beside the EXE.
    if (FileExists("nokia_drive_icon.png")) {
        Image gameIcon = LoadImage("nokia_drive_icon.png");
        if (gameIcon.data != nullptr) {
            SetWindowIcon(gameIcon);
            UnloadImage(gameIcon);
        }
    }
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    // Play Grand Project as looping background music. The MP3 must be beside the EXE.
    InitAudioDevice();
    Music backgroundMusic = {0};
    bool musicLoaded = false;
    bool musicEnabled = true;

    if (IsAudioDeviceReady() && FileExists("grand_project-wonders-of-the-earth-550792.mp3")) {
        backgroundMusic = LoadMusicStream("grand_project-wonders-of-the-earth-550792.mp3");
       musicLoaded = IsMusicValid(backgroundMusic);
        if (musicLoaded) {
            SetMusicVolume(backgroundMusic, 0.25f);
            PlayMusicStream(backgroundMusic);
        }
    }

    RenderTexture2D lowRes = LoadRenderTexture(VIEW_W, VIEW_H);
    SetTextureFilter(lowRes.texture, TEXTURE_FILTER_POINT);

    Screen screen = Screen::MENU;
    int menuSelection = 0;
    int settingsSelection = 0;
    int pauseSelection = 0;
    int gameOverSelection = 0;
    int difficulty = 1;     // 0 Easy, 1 Normal, 2 Hard
    int trafficDensity = 1; // 0 Low, 1 Normal, 2 Busy
    bool paused = false;
    bool running = true;

    float playerX = (ROAD_LEFT + ROAD_RIGHT) * 0.5f;
    float speed = 0.0f;
    float distanceKm = 0.0f;
    float roadOffset = 0.0f;
    float spawnTimer = 0.0f;
    std::vector<TrafficCar> traffic;

    auto startDriving = [&]() {
        resetDrive(playerX, speed, distanceKm, roadOffset, spawnTimer, traffic);
        paused = false;
        screen = Screen::DRIVING;
    };

    auto activateMenu = [&]() {
        switch (menuSelection) {
            case 0: startDriving(); break;
            case 1: screen = Screen::CONTROLS; break;
            case 2: settingsSelection = 0; screen = Screen::SETTINGS; break;
            case 3: running = false; break;
        }
    };

    while (running && !WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.05f);
        const Vector2 mouse = GetMousePosition();
        const Vector2 virtualMouse = {mouse.x * VIEW_W / WINDOW_W,
                                      mouse.y * VIEW_H / WINDOW_H};
        const bool mouseClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        // Press M anywhere to pause or resume the soundtrack.
        if (musicLoaded && IsKeyPressed(KEY_M)) {
            musicEnabled = !musicEnabled;
            if (musicEnabled) ResumeMusicStream(backgroundMusic);
            else PauseMusicStream(backgroundMusic);
        }
        if (musicLoaded && musicEnabled) UpdateMusicStream(backgroundMusic);

        if (screen == Screen::MENU) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) menuSelection = (menuSelection + 3) % 4;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuSelection = (menuSelection + 1) % 4;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) activateMenu();
            for (int i = 0; i < 4; ++i) {
                if (CheckCollisionPointRec(virtualMouse, menuButton(i))) {
                    menuSelection = i;
                    if (mouseClick) activateMenu();
                }
            }
        } else if (screen == Screen::CONTROLS) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_BACKSPACE)) {
                screen = Screen::MENU;
            }
            if (mouseClick && CheckCollisionPointRec(virtualMouse, backButton())) screen = Screen::MENU;
        } else if (screen == Screen::SETTINGS) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) settingsSelection = (settingsSelection + 2) % 3;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) settingsSelection = (settingsSelection + 1) % 3;
            if (settingsSelection < 2) {
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                    if (settingsSelection == 0) difficulty = (difficulty + 2) % 3;
                    else trafficDensity = (trafficDensity + 2) % 3;
                }
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                    if (settingsSelection == 0) difficulty = (difficulty + 1) % 3;
                    else trafficDensity = (trafficDensity + 1) % 3;
                }
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    if (settingsSelection == 0) difficulty = (difficulty + 1) % 3;
                    else trafficDensity = (trafficDensity + 1) % 3;
                }
            } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                screen = Screen::MENU;
            }

            if (mouseClick) {
                if (CheckCollisionPointRec(virtualMouse, {25, 58, 270, 27})) {
                    settingsSelection = 0;
                    difficulty = (difficulty + (virtualMouse.x < 200 ? 2 : 1)) % 3;
                } else if (CheckCollisionPointRec(virtualMouse, {25, 94, 270, 27})) {
                    settingsSelection = 1;
                    trafficDensity = (trafficDensity + (virtualMouse.x < 200 ? 2 : 1)) % 3;
                } else if (CheckCollisionPointRec(virtualMouse, {94, 139, 132, 18})) {
                    screen = Screen::MENU;
                }
            }
        } else if (screen == Screen::DRIVING) {
            if (paused) {
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) pauseSelection = (pauseSelection + 2) % 3;
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) pauseSelection = (pauseSelection + 1) % 3;
                if (IsKeyPressed(KEY_ESCAPE)) paused = false;
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    if (pauseSelection == 0) paused = false;
                    else if (pauseSelection == 1) startDriving();
                    else { screen = Screen::MENU; paused = false; }
                }
                if (mouseClick) {
                    const Rectangle buttons[] = {{91, 83, 138, 17}, {91, 104, 138, 17}, {91, 125, 138, 17}};
                    for (int i = 0; i < 3; ++i) {
                        if (CheckCollisionPointRec(virtualMouse, buttons[i])) {
                            pauseSelection = i;
                            if (i == 0) paused = false;
                            else if (i == 1) startDriving();
                            else { screen = Screen::MENU; paused = false; }
                        }
                    }
                }
            } else {
                if (IsKeyPressed(KEY_ESCAPE)) { paused = true; pauseSelection = 0; }
                if (IsKeyPressed(KEY_R)) startDriving();

                if (!paused && screen == Screen::DRIVING) {
                    const float maxSpeed = difficulty == 0 ? 90.0f : (difficulty == 1 ? 120.0f : 150.0f);
                    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) speed = std::min(maxSpeed, speed + 42.0f * dt);
                    else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) speed = std::max(0.0f, speed - 75.0f * dt);
                    else speed = std::max(0.0f, speed - 7.0f * dt);

                    float steering = 0.0f;
                    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) steering -= 1.0f;
                    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) steering += 1.0f;
                    playerX += steering * (70.0f + speed * 0.15f) * dt;
                    playerX = std::clamp(playerX, ROAD_LEFT + 10.0f, ROAD_RIGHT - 10.0f);
                    roadOffset += speed * 0.55f * dt;
                    distanceKm += speed * dt / 3600.0f;

                    spawnTimer += dt;
                    const float baseInterval = trafficDensity == 0 ? 1.65f : (trafficDensity == 1 ? 1.12f : 0.72f);
                    const float spawnInterval = baseInterval * (difficulty == 2 ? 0.88f : 1.0f);
                    if (spawnTimer >= spawnInterval) {
                        spawnTimer = 0.0f;
                        const int lane = GetRandomValue(0, 2);
                        bool laneClear = true;
                        for (const TrafficCar& car : traffic) {
                            if (car.lane == lane && car.y < 47.0f) { laneClear = false; break; }
                        }
                        if (laneClear) traffic.push_back({laneCenter(lane), 36.0f, lane, GetRandomValue(0, 1)});
                    }

                    const float trafficSpeed = 13.0f + speed * 0.58f + difficulty * 3.0f;
                    for (TrafficCar& car : traffic) car.y += trafficSpeed * dt;
                    traffic.erase(std::remove_if(traffic.begin(), traffic.end(),
                        [](const TrafficCar& car) { return car.y > VIEW_H + 18.0f; }), traffic.end());

                    const Rectangle player = playerRect(playerX);
                    for (const TrafficCar& car : traffic) {
                        if (CheckCollisionRecs(player, trafficRect(car))) {
                            screen = Screen::GAME_OVER;
                            gameOverSelection = 0;
                            break;
                        }
                    }
                }
            }
        } else if (screen == Screen::GAME_OVER) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                gameOverSelection = (gameOverSelection + 1) % 2;
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (gameOverSelection == 0) startDriving();
                else screen = Screen::MENU;
            }
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::MENU;
            if (mouseClick) {
                if (CheckCollisionPointRec(virtualMouse, {94, 104, 132, 18})) startDriving();
                else if (CheckCollisionPointRec(virtualMouse, {94, 127, 132, 18})) screen = Screen::MENU;
            }
        }

        BeginTextureMode(lowRes);
        if (screen == Screen::MENU) drawMenu(menuSelection, musicEnabled);
        else if (screen == Screen::CONTROLS) drawControls();
        else if (screen == Screen::SETTINGS) drawSettings(settingsSelection, difficulty, trafficDensity);
        else if (screen == Screen::DRIVING) {
            const int maxSpeed = difficulty == 0 ? 90 : (difficulty == 1 ? 120 : 150);
            drawRoad(roadOffset, playerX, traffic, speed, distanceKm, maxSpeed);
            if (paused) drawPauseOverlay(pauseSelection);
        } else if (screen == Screen::GAME_OVER) drawGameOver(distanceKm, gameOverSelection);
        EndTextureMode();

        BeginDrawing();
        ClearBackground(INK);
        DrawTexturePro(lowRes.texture,
                       {0.0f, 0.0f, static_cast<float>(VIEW_W), -static_cast<float>(VIEW_H)},
                       {0.0f, 0.0f, static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)},
                       {0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    if (musicLoaded) {
        StopMusicStream(backgroundMusic);
        UnloadMusicStream(backgroundMusic);
    }
    if (IsAudioDeviceReady()) CloseAudioDevice();

    UnloadRenderTexture(lowRes);
    CloseWindow();
    return 0;
}
