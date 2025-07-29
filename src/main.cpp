#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef BROWN
#define BROWN (Color){139, 69, 19, 255}
#endif

constexpr int SCREEN_WIDTH = 1200;
constexpr int SCREEN_HEIGHT = 800;
constexpr int PIXEL_SIZE = 4;
constexpr int UI_HEIGHT = 40;
constexpr int SIM_HEIGHT_IN_PIXELS = SCREEN_HEIGHT - UI_HEIGHT;
constexpr int GRID_WIDTH = SCREEN_WIDTH / PIXEL_SIZE;
constexpr int GRID_HEIGHT = SIM_HEIGHT_IN_PIXELS / PIXEL_SIZE;

enum Element {
    EMPTY,
    SAND,
    WATER,
    FIRE,
    WOOD,
    SMOKE,
    ASH
};

struct Particle {
    Element type = EMPTY;
    Color color = BLACK;
    float life = 0.0f;
};

std::vector<std::vector<Particle>> grid(GRID_HEIGHT, std::vector<Particle>(GRID_WIDTH));
std::vector<std::vector<Particle>> buffer(GRID_HEIGHT, std::vector<Particle>(GRID_WIDTH));

bool isDrawingPlank = false;
Vector2 plankStartPos = { 0, 0 };

int Clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float getSimpleNoise(int x, int y, float time_factor, float scale) {
    unsigned int hash_val = (unsigned int)((x * 1619 + y * 31337 + static_cast<int>(time_factor * 1000) * 69420) & 0x7FFFFFFF);
    return static_cast<float>(hash_val % 1000) / 1000.0f * scale;
}

bool InBounds(int x, int y) {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

void MoveParticle(int from_x, int from_y, int to_x, int to_y) {
    if (!InBounds(from_x, from_y) || !InBounds(to_x, to_y)) return;

    buffer[to_y][to_x] = grid[from_y][from_x];
    buffer[from_y][from_x] = { EMPTY, BLACK, 0.0f };
}

bool IsBufferTargetEmptyOrReplaceable(int x, int y, Element moving_type) {
    if (!InBounds(x, y)) return false;
    Element target_type = buffer[y][x].type;

    if (target_type == EMPTY) return true;

    switch (moving_type) {
    case SAND:
        return target_type == WATER;
    case WATER:
        return false;
    case FIRE:
        return target_type == SMOKE || target_type == WATER;
    case SMOKE:
        return false;
    default:
        return false;
    }
}

void SetCell(int x, int y, Element type) {
    if (!InBounds(x, y)) return;

    if ((grid[y][x].type == WOOD || grid[y][x].type == ASH) && type != FIRE) return;
    if (grid[y][x].type == type && type != FIRE && type != WATER) return;

    grid[y][x].type = type;
    grid[y][x].life = 0.0f;

    switch (type) {
    case SAND:
        grid[y][x].color = Color{
            static_cast<unsigned char>(Clamp(200 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 60)), 200, 255)),
            static_cast<unsigned char>(Clamp(170 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 40)), 170, 220)),
            static_cast<unsigned char>(Clamp(100 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 30)), 100, 140)),
            255
        };

        break;
    case WATER:
        grid[y][x].color = Color{ static_cast<unsigned char>(Clamp(20 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 30)), 0, 50)),
                                   static_cast<unsigned char>(Clamp(100 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 70)), 80, 170)),
                                   static_cast<unsigned char>(Clamp(180 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 70)), 150, 255)),
                                   200 };
        break;
    case FIRE:
        grid[y][x].color = RED;
        grid[y][x].life = 1.5f + static_cast<float>(GetRandomValue(-5, 5)) / 10.0f;
        break;
    case WOOD:
        grid[y][x].color = Color{ static_cast<unsigned char>(Clamp(BROWN.r + static_cast<int>(getSimpleNoise(x, y, 0, 30)), 100, 180)),
                                   static_cast<unsigned char>(Clamp(BROWN.g + static_cast<int>(getSimpleNoise(x, y, 0, 20)), 50, 100)),
                                   static_cast<unsigned char>(Clamp(BROWN.b + static_cast<int>(getSimpleNoise(x, y, 0, 10)), 0, 30)),
                                   255 };
        grid[y][x].life = 5.0f;
        break;
    case SMOKE:
        grid[y][x].color = Color{ static_cast<unsigned char>(Clamp(100 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 50)), 50, 150)),
                                   static_cast<unsigned char>(Clamp(100 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 50)), 50, 150)),
                                   static_cast<unsigned char>(Clamp(100 + static_cast<int>(getSimpleNoise(x, y, GetTime(), 50)), 50, 150)),
                                   150 };
        grid[y][x].life = 1.0f + static_cast<float>(GetRandomValue(-3, 3)) / 10.0f;
        break;
    case ASH:
        grid[y][x].color = Color{ static_cast<unsigned char>(Clamp(30 + static_cast<int>(getSimpleNoise(x, y, 0, 20)), 20, 60)),
                                   static_cast<unsigned char>(Clamp(30 + static_cast<int>(getSimpleNoise(x, y, 0, 20)), 20, 60)),
                                   static_cast<unsigned char>(Clamp(30 + static_cast<int>(getSimpleNoise(x, y, 0, 20)), 20, 60)),
                                   255 };
        break;
    default:
        grid[y][x].color = BLACK;
        break;
    }
}

void DrawWoodPlank(Vector2 start, Vector2 end) {
    int x1 = Clamp(static_cast<int>(start.x) / PIXEL_SIZE, 0, GRID_WIDTH - 1);
    int y1 = Clamp(static_cast<int>(start.y) / PIXEL_SIZE, 0, GRID_HEIGHT - 1);
    int x2 = Clamp(static_cast<int>(end.x) / PIXEL_SIZE, 0, GRID_WIDTH - 1);
    int y2 = Clamp(static_cast<int>(end.y) / PIXEL_SIZE, 0, GRID_HEIGHT - 1);

    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            SetCell(x, y, WOOD);
        }
    }
}

void UpdateSand(int x, int y) {
    if (InBounds(x, y + 1) && IsBufferTargetEmptyOrReplaceable(x, y + 1, SAND)) {
        MoveParticle(x, y, x, y + 1);
        return;
    }

    int dir = (GetRandomValue(0, 1) == 0) ? 1 : -1;
    if (InBounds(x + dir, y + 1) && IsBufferTargetEmptyOrReplaceable(x + dir, y + 1, SAND)) {
        MoveParticle(x, y, x + dir, y + 1);
        return;
    }
    else if (InBounds(x - dir, y + 1) && IsBufferTargetEmptyOrReplaceable(x - dir, y + 1, SAND)) {
        MoveParticle(x, y, x - dir, y + 1);
        return;
    }

    buffer[y][x] = grid[y][x];
}

void UpdateWater(int x, int y) {
    if (InBounds(x, y + 1) && (buffer[y + 1][x].type == EMPTY || buffer[y + 1][x].type == SAND)) {
        MoveParticle(x, y, x, y + 1);
        return;
    }

    int dir = (GetRandomValue(0, 1) == 0) ? 1 : -1;
    if (InBounds(x + dir, y) && buffer[y][x + dir].type == EMPTY) {
        MoveParticle(x, y, x + dir, y);
        return;
    }
    else if (InBounds(x - dir, y) && buffer[y][x - dir].type == EMPTY) {
        MoveParticle(x, y, x - dir, y);
        return;
    }

    if (InBounds(x + dir, y + 1) && (buffer[y + 1][x + dir].type == EMPTY || buffer[y + 1][x + dir].type == SAND)) {
        MoveParticle(x, y, x + dir, y + 1);
        return;
    }
    else if (InBounds(x - dir, y + 1) && (buffer[y + 1][x - dir].type == EMPTY || buffer[y + 1][x - dir].type == SAND)) {
        MoveParticle(x, y, x - dir, y + 1);
        return;
    }

    buffer[y][x] = grid[y][x];
}

void UpdateFire(int x, int y) {
    Particle& c = grid[y][x];
    c.life -= GetFrameTime() * 0.8f;

    if (c.life <= 0.0f) {
        SetCell(x, y, SMOKE);
        grid[y][x].life = 0.5f + static_cast<float>(GetRandomValue(0, 5)) / 10.0f;
        return;
    }

    unsigned char alpha = static_cast<unsigned char>(Clamp(static_cast<int>(c.life / 1.5f * 255), 100, 255));
    unsigned char red_val = static_cast<unsigned char>(Clamp(255 - static_cast<int>(c.life * 50) + GetRandomValue(-30, 30), 150, 255));
    unsigned char green_val = static_cast<unsigned char>(Clamp(100 + static_cast<int>(c.life * 50) + GetRandomValue(-20, 20), 50, 200));
    unsigned char blue_val = static_cast<unsigned char>(Clamp(GetRandomValue(0, 30), 0, 50));

    if (GetRandomValue(0, 100) < 5) {
        c.color = Color{ 255, 220, 100, alpha };
    }
    else {
        c.color = Color{ red_val, green_val, blue_val, alpha };
    }

    int move_x = GetRandomValue(-1, 1);
    int move_y = -1;

    if (InBounds(x + move_x, y + move_y) && (buffer[y + move_y][x + move_x].type == EMPTY || buffer[y + move_y][x + move_x].type == SMOKE)) {
        MoveParticle(x, y, x + move_x, y + move_y);
        return;
    }
    else if (InBounds(x, y - 1) && (buffer[y - 1][x].type == EMPTY || buffer[y - 1][x].type == SMOKE)) {
        MoveParticle(x, y, x, y - 1);
        return;
    }

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;

            if (InBounds(nx, ny)) {
                if (grid[ny][nx].type == WOOD) {
                    grid[ny][nx].life -= GetFrameTime() * 2.0f;
                    if (grid[ny][nx].life <= 0.0f) {
                        SetCell(nx, ny, SMOKE);
                    }
                    else {
                        SetCell(nx, ny, FIRE);
                    }
                    if (GetRandomValue(0, 100) < 50) SetCell(nx, ny, FIRE);
                }
                else if (grid[ny][nx].type == SAND) {
                    if (GetRandomValue(0, 100) < 5) {
                        SetCell(nx, ny, ASH);
                        SetCell(x, y, SMOKE);
                    }
                }
                else if (grid[ny][nx].type == WATER) {
                    if (GetRandomValue(0, 100) < 10) {
                        SetCell(ny, nx, EMPTY);
                        SetCell(x, y, SMOKE);
                    }
                }
            }
        }
    }

    buffer[y][x] = c;
}

void UpdateSmoke(int x, int y) {
    Particle& c = grid[y][x];
    c.life -= GetFrameTime() * 0.5f;
    c.color.a = static_cast<unsigned char>(Clamp(static_cast<int>(c.life / (1.0f + static_cast<float>(GetRandomValue(-3, 3)) / 10.0f) * 150), 0, 150));

    if (c.life <= 0.0f || c.color.a == 0) {
        return;
    }

    int move_x = GetRandomValue(-1, 1);
    int move_y = -1;

    if (InBounds(x + move_x, y + move_y) && buffer[y + move_y][x + move_x].type == EMPTY) {
        MoveParticle(x, y, x + move_x, y + move_y);
        return;
    }
    else if (InBounds(x, y - 1) && buffer[y - 1][x].type == EMPTY) {
        MoveParticle(x, y, x, y - 1);
        return;
    }

    buffer[y][x] = c;
}

void UpdateSimulation() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            buffer[y][x] = { EMPTY, BLACK, 0.0f };
        }
    }

    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        int start_x = (y % 2 == 0) ? 0 : GRID_WIDTH - 1;
        int end_x = (y % 2 == 0) ? GRID_WIDTH : -1;
        int step_x = (y % 2 == 0) ? 1 : -1;

        for (int x = start_x; x != end_x; x += step_x) {
            if (buffer[y][x].type != EMPTY) {
                continue;
            }

            switch (grid[y][x].type) {
            case SAND:  UpdateSand(x, y); break;
            case WATER: UpdateWater(x, y); break;
            case FIRE:  UpdateFire(x, y); break;
            case SMOKE: UpdateSmoke(x, y); break;
            case WOOD:  buffer[y][x] = grid[y][x]; break;
            case ASH:   buffer[y][x] = grid[y][x]; break;
            default: break;
            }
        }
    }
    grid = buffer;
}

void DrawGrid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x].type != EMPTY) {
                Color drawColor = grid[y][x].color;

                if (grid[y][x].type == SAND || grid[y][x].type == WATER || grid[y][x].type == WOOD || grid[y][x].type == ASH) {
                    int flicker = GetRandomValue(-8, 8);
                    drawColor.r = static_cast<unsigned char>(Clamp(static_cast<int>(drawColor.r) + flicker, 0, 255));
                    drawColor.g = static_cast<unsigned char>(Clamp(static_cast<int>(drawColor.g) + flicker, 0, 255));
                    drawColor.b = static_cast<unsigned char>(Clamp(static_cast<int>(drawColor.b) + flicker, 0, 255));
                }
                DrawRectangle(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, drawColor);
            }
        }
    }
}

void DrawGlowAndEmbers() {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x].type == FIRE) {
                int px = x * PIXEL_SIZE;
                int py = y * PIXEL_SIZE;
                float flicker_intensity = 0.5f + 0.5f * sinf(GetTime() * 12.0f + x * 0.5f + y * 0.3f);
                unsigned char glow_alpha = static_cast<unsigned char>(Clamp(static_cast<int>(grid[y][x].life * 80 * flicker_intensity), 0, 150));
                Color glowColor = Color{ 255, 150, 50, glow_alpha };

                DrawCircle(px + PIXEL_SIZE / 2, py + PIXEL_SIZE / 2, PIXEL_SIZE * 2.5f * flicker_intensity, glowColor);

                if (GetRandomValue(0, 100) < 30) {
                    DrawPixel(px + GetRandomValue(0, PIXEL_SIZE - 1), py + GetRandomValue(0, PIXEL_SIZE - 1), Color{ 255, 255, static_cast<unsigned char>(GetRandomValue(180, 255)), 255 });
                }
            }
        }
    }
    EndBlendMode();
}

void DrawUI(Element current, int fps) {
    DrawRectangle(0, SIM_HEIGHT_IN_PIXELS, SCREEN_WIDTH, UI_HEIGHT, DARKGRAY);
    DrawText("[1] Sand  [2] Water  [3] Fire  [4] Wood (Drag to draw planks) [5] Smoke [6] Ash", 10, SIM_HEIGHT_IN_PIXELS + 8, 16, RAYWHITE);
    const char* selected = "";
    switch (current) {
    case SAND:  selected = "Selected: Sand"; break;
    case WATER: selected = "Selected: Water"; break;
    case FIRE:  selected = "Selected: Fire"; break;
    case WOOD:  selected = "Selected: Wood (Plank Mode)"; break;
    case SMOKE: selected = "Selected: Smoke"; break;
    case ASH:   selected = "Selected: Ash"; break;
    default: break;
    }
    DrawText(selected, 600, SIM_HEIGHT_IN_PIXELS + 8, 16, YELLOW);

    DrawText(TextFormat("FPS: %d", fps), SCREEN_WIDTH - 100, SIM_HEIGHT_IN_PIXELS + 8, 16, LIME);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pixel RPG Particle Sim (v1)");
    SetTargetFPS(60);
    srand(static_cast<unsigned>(time(NULL)));

    Element current = SAND;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE)) current = SAND;
        if (IsKeyPressed(KEY_TWO)) current = WATER;
        if (IsKeyPressed(KEY_THREE)) current = FIRE;
        if (IsKeyPressed(KEY_FOUR)) current = WOOD;
        if (IsKeyPressed(KEY_FIVE)) current = SMOKE;
        if (IsKeyPressed(KEY_SIX)) current = ASH;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (current == WOOD) {
                isDrawingPlank = true;
                plankStartPos = GetMousePosition();
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (current == WOOD && isDrawingPlank) {
                DrawWoodPlank(plankStartPos, GetMousePosition());
                isDrawingPlank = false;
            }
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            if (mp.y >= SIM_HEIGHT_IN_PIXELS) {
                isDrawingPlank = false;
            }
            else {
                int cx = static_cast<int>(mp.x) / PIXEL_SIZE;
                int cy = static_cast<int>(mp.y) / PIXEL_SIZE;

                int brushSize = 3;
                if (current != WOOD) {
                    for (int dy = -brushSize; dy <= brushSize; ++dy) {
                        for (int dx = -brushSize; dx <= brushSize; ++dx) {
                            if (sqrt(static_cast<float>(dx * dx + dy * dy)) <= brushSize && GetRandomValue(0, 10) < 8) {
                                SetCell(cx + dx, cy + dy, current);
                            }
                        }
                    }
                }
            }
        }
        if (IsKeyPressed(KEY_C)) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    grid[y][x] = { EMPTY, BLACK, 0.0f };
                }
            }
        }

        UpdateSimulation();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawGrid();
        DrawGlowAndEmbers();

        if (isDrawingPlank && current == WOOD) {
            Vector2 currentMousePos = GetMousePosition();
            DrawRectangleLines(static_cast<int>(plankStartPos.x), static_cast<int>(plankStartPos.y),
                static_cast<int>(currentMousePos.x - plankStartPos.x), static_cast<int>(currentMousePos.y - plankStartPos.y),
                WHITE);
        }

        DrawUI(current, GetFPS());
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
