// ============================================================================
// TUMBLEPOP - MASTERCLASS PROFESSIONAL EDITION v8.0
// Programming Fundamentals Project - Fall 2025
// PORTFOLIO-READY BUILD WITH ENHANCED ANIMATIONS & UI
// ============================================================================

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <map>
#include <tuple>

using namespace sf;
using namespace std;

// ============================================================================
// CONSTANTS
// ============================================================================
namespace
{
    const int SCREEN_WIDTH = 1136;
    const int SCREEN_HEIGHT = 896;
    const int CELL_SIZE = 64;
    const int LEVEL_HEIGHT = 14;
    const int LEVEL_WIDTH = 18;

    const float GRAVITY = 0.8f;
    const float TERMINAL_VELOCITY = 15.0f;
    const float JUMP_STRENGTH = -18.0f;
    const float GROUND_SNAP_PX = 12.0f;

    const int PLAYER_FRAME_WIDTH = 96;
    const int PLAYER_FRAME_HEIGHT = 96;
    const float PLAYER_SCALE = 1.95f; // ENHANCED: Larger, more prominent player

    const int ENEMY_FRAME_WIDTH = 64;
    const int ENEMY_FRAME_HEIGHT = 64;
    const float ENEMY_SCALE = 1.75f; // ENHANCED: Slightly larger enemies

    const float PROJECTILE_SPEED = 12.0f;
    const float ROLL_SPEED = 0.9f;
    const float PROJECTILE_LIFETIME = 10.0f;

    const float PROJECTILE_SCALE = 1.4f;         // ENHANCED: More visible projectiles
    const float EFFECT_SCALE = 1.55f;            // ENHANCED: More prominent effects
    const float VACUUM_EFFECT_THICKNESS = 1.65f; // ENHANCED: Thicker vacuum beam

    // MASTERCLASS: Character Selection Enhanced Constants
    const float CHAR_SELECT_SCALE = 4.2f;          // Large character previews
    const float CHAR_SELECT_BOX_W = 420.0f;        // Wide selection boxes
    const float CHAR_SELECT_BOX_H = 540.0f;        // Tall selection boxes
    const float CHAR_SELECT_BOUNCE = 18.0f;        // Noticeable bounce animation
    const float CHAR_SELECT_GLOW_PULSE = 6.0f;     // Glow pulse speed
    const float CHAR_SELECT_BREATHE_SCALE = 0.05f; // Breathing animation scale
}

static inline int platformTopY(int row)
{
    int y = row * CELL_SIZE;
    if (row == 11)
        y -= 22;
    return y;
}

static inline int enemyPlatformYOffset(int row)
{
    return (row == 11) ? -22 : 0;
}

namespace
{
    int slopeOffset[LEVEL_HEIGHT][LEVEL_WIDTH];
}

static inline int platformTopYAt(int row, int col)
{
    int base = platformTopY(row);
    base += enemyPlatformYOffset(row);
    if (row >= 0 && row < LEVEL_HEIGHT && col >= 0 && col < LEVEL_WIDTH)
        base += slopeOffset[row][col];
    return base;
}

const int screen_x = SCREEN_WIDTH;
const int screen_y = SCREEN_HEIGHT;

void display_level(RenderWindow &window,
                   char **lvl,
                   Texture &bgTex,
                   Sprite &bgSprite,
                   Texture &blockTexture,
                   Sprite &blockSprite,
                   const int height,
                   const int width,
                   const int cell_size)
{
    bgSprite.setTexture(bgTex);
    bgSprite.setScale((float)SCREEN_WIDTH / bgTex.getSize().x,
                      (float)SCREEN_HEIGHT / bgTex.getSize().y);
    window.draw(bgSprite);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (lvl[i][j] == '#')
            {
                if (blockTexture.getSize().x > 0)
                {
                    blockSprite.setTexture(blockTexture);
                    blockSprite.setTextureRect(IntRect(0, 0, (int)blockTexture.getSize().x, (int)blockTexture.getSize().y));
                    blockSprite.setPosition(j * cell_size, platformTopYAt(i, j));
                    window.draw(blockSprite);
                }
                else
                {
                    // Texture missing: skip to remain within allowed API
                }
            }
        }
    }
}

void player_gravity(char **lvl,
                    float &offset_y,
                    float &velocityY,
                    bool &onGround,
                    const float &gravity,
                    float &terminal_Velocity,
                    float &player_x,
                    float &player_y,
                    const int cell_size,
                    int &Pheight,
                    int &Pwidth)
{
    offset_y = player_y;
    offset_y += velocityY;

    int by = (int)((offset_y + Pheight) / cell_size);
    int bl = (int)(player_x / cell_size);
    int br = (int)((player_x + Pwidth) / cell_size);
    int bm = (int)((player_x + Pwidth / 2.0f) / cell_size);

    char bottom_left_down = (by >= 0 && by < LEVEL_HEIGHT && bl >= 0 && bl < LEVEL_WIDTH) ? lvl[by][bl] : ' ';
    char bottom_right_down = (by >= 0 && by < LEVEL_HEIGHT && br >= 0 && br < LEVEL_WIDTH) ? lvl[by][br] : ' ';
    char bottom_mid_down = (by >= 0 && by < LEVEL_HEIGHT && bm >= 0 && bm < LEVEL_WIDTH) ? lvl[by][bm] : ' ';

    if (bottom_left_down == '#' || bottom_mid_down == '#' || bottom_right_down == '#')
    {
        onGround = true;
    }
    else
    {
        player_y = offset_y;
        onGround = false;
    }

    if (!onGround)
    {
        velocityY += gravity;
        if (velocityY >= terminal_Velocity)
            velocityY = terminal_Velocity;
    }
    else
    {
        velocityY = 0;
    }
}

enum GameState
{
    CHARACTER_SELECT,
    LEVEL_1,
    LEVEL_2,
    GAME_OVER,
    VICTORY
};

enum PowerupType
{
    POWERUP_SPEED,
    POWERUP_RANGE,
    POWERUP_POWER,
    POWERUP_LIFE
};

enum PlayerAnimState
{
    P_IDLE,
    P_RUN,
    P_SLIDE,
    P_JUMP_START,
    P_JUMP_MID,
    P_JUMP_PEAK,
    P_FALL,
    P_ALERT,
    P_LAND,
    P_CROUCH,
    P_HURT,
    P_KNOCKDOWN,
    P_VICTORY,
    P_DEATH_FADE,
    P_POWER_RUN,
    P_JETPACK,
    P_SHOOT,
    P_SHOOT_FORWARD,
    P_SHOOT_SIDE
};

// ============================================================================
// PARTICLE SYSTEM
// ============================================================================
class Particle
{
public:
    Vector2f position;
    Vector2f velocity;
    Color color;
    float lifetime;
    float maxLifetime;
    float size;
    bool active;

    Particle() : active(false) {}

    Particle(Vector2f pos, Vector2f vel, Color col, float life, float sz = 3.0f)
    {
        position = pos;
        velocity = vel;
        color = col;
        lifetime = life;
        maxLifetime = life;
        size = sz;
        active = true;
    }

    // Updates projectile animation and movement each frame
    void update(float dt)
    {
        if (!active)
            return;

        position += velocity * dt * 60.0f;
        velocity.y += GRAVITY * 0.3f;
        lifetime -= dt;

        if (lifetime <= 0)
        {
            active = false;
        }
    }

    void draw(RenderWindow &window)
    {
        if (!active)
            return;

        float alpha = (lifetime / maxLifetime) * 255;
        CircleShape particle(size);
        particle.setPosition(position);
        particle.setFillColor(Color(color.r, color.g, color.b, (Uint8)alpha));
        window.draw(particle);
    }
};

class ParticleSystem
{
public:
    vector<Particle> particles;

    void addParticle(Particle p)
    {
        particles.push_back(p);
    }

    void emit(Vector2f position, Color color, int count = 10)
    {
        for (int i = 0; i < count; i++)
        {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float speed = 2.0f + (rand() % 100) / 50.0f;
            Vector2f vel(cos(angle) * speed, sin(angle) * speed);
            addParticle(Particle(position, vel, color, 0.5f + (rand() % 50) / 100.0f));
        }
    }

    void update(float dt)
    {
        for (size_t i = 0; i < particles.size(); i++)
        {
            particles[i].update(dt);
        }

        // Remove inactive particles
        particles.erase(
            remove_if(particles.begin(), particles.end(),
                      [](const Particle &p)
                      { return !p.active; }),
            particles.end());
    }

    void draw(RenderWindow &window)
    {
        for (size_t i = 0; i < particles.size(); i++)
        {
            particles[i].draw(window);
        }
    }
};

// ============================================================================
// SCORE MANAGER
// ============================================================================
class ScoreManager
{
public:
    int score;
    int combo;
    float comboTimer;
    bool noDamage;
    float levelTime;
    float scoreAnimTimer; // MASTERCLASS: For animated score pop effect

    ScoreManager() { reset(); }

    void reset()
    {
        score = 0;
        combo = 0;
        comboTimer = 0;
        noDamage = true;
        levelTime = 0;
        scoreAnimTimer = 0;
    }

    void resetLevel()
    {
        combo = 0;
        comboTimer = 0;
        noDamage = true;
        levelTime = 0;
        scoreAnimTimer = 0;
    }

    void update(float dt)
    {
        levelTime += dt;
        if (comboTimer > 0)
        {
            comboTimer -= dt;
            if (comboTimer <= 0)
            {
                combo = 0;
            }
        }
        // MASTERCLASS: Update score animation timer
        if (scoreAnimTimer > 0)
            scoreAnimTimer -= dt;
    }

    void addCapturePoints(int points, int currentCapturedCount)
    {
        score += points;
        scoreAnimTimer = 0.3f; // MASTERCLASS: Trigger score pop
        // CONSOLE OUTPUT (Requirement 3)
        // "Captured 1" (No max capacity)
        cout << "Captured " << currentCapturedCount << endl;
    }

    void addDefeatPoints(int basePoints)
    {
        int points = basePoints * 2;
        if (combo >= 5)
        {
            points = (int)(points * 2.0f);
        }
        else if (combo >= 3)
        {
            points = (int)(points * 1.5f);
        }
        score += points;
        combo++;
        comboTimer = 3.0f;
        scoreAnimTimer = 0.4f; // MASTERCLASS: Trigger score pop
    }

    void addMultiKillBonus(int count)
    {
        int bonus = 0;
        if (count >= 3)
            bonus = 500;
        else if (count == 2)
            bonus = 200;
        if (bonus > 0)
            score += bonus;
    }

    void addVacuumBurstBonus(int count)
    {
        if (count >= 3)
            score += 300;
    }

    void addAerialBonus()
    {
        score += 150;
    }

    void addCharacterBonus(const std::string &label)
    {
        score += 500;
        cout << "[CHARACTER BONUS] " << label << " +500 (Total: " << score << ")" << endl;
    }

    void playerHit()
    {
        score -= 50;
        if (score < 0)
            score = 0;
        noDamage = false;
        combo = 0;
        comboTimer = 0;
    }

    void playerDeath()
    {
        score -= 200;
        if (score < 0)
            score = 0;
    }

    void levelComplete(int level)
    {
        if (level == 1)
        {
            score += 1000;
            if (noDamage)
                score += 1500;
            if (levelTime < 30)
                score += 2000;
            else if (levelTime < 45)
                score += 1000;
            else if (levelTime < 60)
                score += 500;
        }
        else
        {
            score += 2000;
            if (noDamage)
                score += 2500;
            if (levelTime < 60)
                score += 3000;
            else if (levelTime < 90)
                score += 1500;
            else if (levelTime < 120)
                score += 750;
        }
    }
};

// ============================================================================
// POWERUP CLASS
// ============================================================================
class Powerup
{
public:
    float x, y;
    PowerupType type;
    bool active;
    Sprite sprite;
    float bobTimer;
    int animFrames;
    float animTimer;
    float animFPS;
    int frameIndex;
    std::vector<IntRect> rects;
    bool useMetaRects;
    int frameW;
    int texH;

    Powerup(PowerupType t, float px, float py)
    {
        type = t;
        x = px;
        y = py;
        active = true;
        bobTimer = 0;
        animFrames = 1;
        animTimer = 0;
        animFPS = 8.0f;
        frameIndex = 0;
        useMetaRects = false;
    }

    void setupSprite(Texture &tex)
    {
        sprite.setTexture(tex);
        sprite.setScale(1.5f, 1.5f);

        if (type == POWERUP_SPEED)
            animFrames = 4;
        else if (type == POWERUP_POWER)
            animFrames = 5;
        else
            animFrames = max(1u, tex.getSize().x / (unsigned)48);

        frameIndex = 0;
        animTimer = 0;

        frameW = (int)(tex.getSize().x / std::max(1, animFrames));
        texH = (int)tex.getSize().y;
        sprite.setTextureRect(IntRect(0, 0, frameW, texH));
    }

    void applyMetaRects(const std::vector<IntRect> &r)
    {
        rects = r;
        if (!rects.empty())
        {
            animFrames = (int)rects.size();
            frameIndex = 0;
            animTimer = 0;
            sprite.setTextureRect(rects[0]);
            frameW = rects[0].width;
            texH = rects[0].height;
            useMetaRects = true;
        }
    }

    void update(float dt)
    {
        if (!active)
            return;

        bobTimer += dt;
        float bobOffset = sin(bobTimer * 3.0f) * 5.0f;
        sprite.setPosition(x - (frameW * 1.5f) / 2.0f, y + bobOffset - (texH * 1.5f) / 2.0f);

        animTimer += dt;
        if (animFrames > 1 && animTimer > (1.0f / animFPS))
        {
            animTimer = 0;
            frameIndex = (frameIndex + 1) % animFrames;
            if (useMetaRects && !rects.empty())
            {
                sprite.setTextureRect(rects[frameIndex]);
            }
            else
            {
                sprite.setTextureRect(IntRect(frameIndex * frameW, 0, frameW, texH));
            }
        }
    }

    bool collidesWith(float px, float py, int pw, int ph)
    {
        return (x < px + pw && x + 48 > px && y < py + ph && y + 48 > py);
    }
};

// ============================================================================
// PLAYER CLASS - ENHANCED WITH PROPER ANIMATIONS
// ============================================================================
class Player
{
public:
    float x, y;
    float velocityX, velocityY;
    int health;
    bool isYellow;
    float speed;
    float baseSpeed;
    bool onGround;
    bool facingRight;

    int vacuumDirection;
    bool vacuumActive;
    int capturedEnemies[5];
    int capturedCount;
    int maxCapacity;
    float vacuumRange;
    float baseVacuumRange;
    float vacuumAngle;
    float baseVacuumAngle;
    float vacuumPower;

    // Vacuum Animation State
    int vacuumAnimState; // 0=OFF, 1=GROW, 2=SUSTAIN, 3=SHRINK
    float vacuumAnimFrame;
    float vacuumAnimSpeed;
    float jumpBoostTimer;

    float speedBoostTimer;
    float rangeBoostTimer;
    float powerBoostTimer;

    int animFrame;
    float animTimer;
    PlayerAnimState animState;
    PlayerAnimState prevAnimState;
    float animStateTimer;
    int animFrames;
    int animRow;
    float animFPS;
    int frameW;
    int frameH;
    int row1TotalFrames;
    int row2TotalFrames;
    std::vector<IntRect> row1Rects;
    std::vector<IntRect> row2Rects;
    std::map<std::string, std::pair<int, int>> labelsRow1;
    std::map<std::string, std::pair<int, int>> labelsRow2;

    Texture *sheetRow1;
    Texture *sheetRow2;
    int rowsRow1;
    int rowsRow2;

    Texture *seqSheet;
    int seqStart;
    bool shootHold;

    Sprite sprite;

    // Enhanced animation frame data
    struct AnimSequence
    {
        int startFrame;
        int frameCount;
        float fps;
        Texture *texture;
    };

    map<PlayerAnimState, AnimSequence> animations;

    Player()
    {
        x = 100;
        y = 500;
        velocityX = 0;
        velocityY = 0;
        health = 3;
        isYellow = true;
        baseSpeed = 8.0f;
        speed = baseSpeed;
        onGround = false;
        facingRight = true;

        vacuumDirection = 0;
        vacuumActive = false;
        capturedCount = 0;
        maxCapacity = 3;
        baseVacuumRange = 150.0f;
        vacuumRange = baseVacuumRange;
        baseVacuumAngle = 45.0f;
        vacuumAngle = baseVacuumAngle;
        vacuumPower = 5.0f;

        vacuumAnimState = 0;
        vacuumAnimFrame = 0.0f;
        vacuumAnimSpeed = 24.0f; // Smoother/Faster animation
        jumpBoostTimer = 0.0f;

        speedBoostTimer = 0;
        rangeBoostTimer = 0;
        powerBoostTimer = 0;

        animFrame = 0;
        animTimer = 0;
        animState = P_IDLE;
        prevAnimState = P_IDLE;
        animStateTimer = 0;
        animFrames = 4;
        animRow = 0;
        animFPS = 2.0f;
        frameW = PLAYER_FRAME_WIDTH;
        frameH = PLAYER_FRAME_HEIGHT;
        row1TotalFrames = 34; // based on analyzed sheet layout
        row2TotalFrames = 24; // allows dash frames in row_2

        sheetRow1 = nullptr;
        sheetRow2 = nullptr;
        rowsRow1 = 0;
        rowsRow2 = 0;

        seqSheet = nullptr;
        seqStart = 0;
        shootHold = false;

        for (int i = 0; i < 5; i++)
            capturedEnemies[i] = -1;
    }

    void setCharacter(bool yellow)
    {
        isYellow = yellow;
        if (yellow)
        {
            baseSpeed = 8.0f;
            baseVacuumRange = 180.0f;
            baseVacuumAngle = 54.0f;
        }
        else
        {
            baseSpeed = 12.0f;
            baseVacuumRange = 150.0f;
            baseVacuumAngle = 45.0f;
        }
        speed = baseSpeed;
        vacuumRange = baseVacuumRange;
        vacuumAngle = baseVacuumAngle;
    }

    void setupSprite(Texture &tex)
    {
        sprite.setTexture(tex);
        frameH = (int)tex.getSize().y;
        int texW = (int)tex.getSize().x;
        int inferred = row1TotalFrames > 0 ? row1TotalFrames : 34;
        frameW = max(1, texW / inferred);
        sprite.setTextureRect(IntRect(0, 0, frameW, frameH));
        // strict compliance: no origin; adjust position when placing
        sprite.setScale(PLAYER_SCALE, PLAYER_SCALE);
    }

    void bindSheets(Texture *row1, Texture *row2)
    {
        sheetRow1 = row1;
        sheetRow2 = row2;

        rowsRow1 = (sheetRow1) ? 1 : 0;
        rowsRow2 = (sheetRow2) ? 1 : 0;

        if (sheetRow1)
            sprite.setTexture(*sheetRow1);

        // Setup animation sequences based on sprite sheet analysis
        setupAnimations();

        // Infer dynamic frame size from imp sheets
        if (sheetRow1)
        {
            frameH = (int)sheetRow1->getSize().y;
            int texW = (int)sheetRow1->getSize().x;
            int fwCandidate = (row1TotalFrames > 0) ? texW / row1TotalFrames : PLAYER_FRAME_WIDTH;
            if (fwCandidate > 0 && fwCandidate * row1TotalFrames == texW)
            {
                frameW = fwCandidate;
            }
            else
            {
                // Fallback: assume square-ish frames or use previous value
                frameW = max(1, min(PLAYER_FRAME_WIDTH, texW));
            }
            sprite.setTextureRect(IntRect(0, 0, frameW, frameH));
            // strict compliance: no origin; adjust position when placing
        }

        // Build per-frame rectangles from transparent gaps (works with auto-cropped sheets)
        buildFrameRects();
        if (!row1Rects.empty())
            row1TotalFrames = (int)row1Rects.size();
        if (!row2Rects.empty())
            row2TotalFrames = (int)row2Rects.size();
    }

    bool loadMetaLabels(const std::string &color)
    {
        labelsRow1.clear();
        labelsRow2.clear();
        labelsRow1["jump_start"] = std::make_pair(12, 13);
        labelsRow1["jump_mid"] = std::make_pair(14, 15);
        labelsRow1["jump_peak"] = std::make_pair(16, 16);
        labelsRow1["fall"] = std::make_pair(17, 17);
        labelsRow1["crouch"] = std::make_pair(18, 18);
        labelsRow1["victory"] = std::make_pair(21, 23);
        labelsRow1["jetpack"] = std::make_pair(28, 30);
        labelsRow2["knockdown"] = std::make_pair(8, 14);
        labelsRow2["land"] = std::make_pair(19, 19);
        return true;
    }

    void setupAnimations()
    {
        int r1 = (int)row1TotalFrames;
        auto clamp = [&](int start, int count)
        { return std::make_pair(start, std::max(0, std::min(count, std::max(0, r1 - start)))); };
        auto pick = [&](std::map<std::string, std::pair<int, int>> &labels, const std::string &name, int defStart, int defCount)
        { auto it = labels.find(name); if (it!=labels.end()) { int s=it->second.first, e=it->second.second; int c = std::max(1, e - s + 1); return std::make_pair(s,c); } return clamp(defStart, defCount); };
        auto add = [&](PlayerAnimState st, std::pair<int, int> p, float fps, Texture *sheet)
        { animations[st] = {p.first, p.second, fps, sheet}; };

        // MASTERCLASS: Enhanced animation FPS for smoother movement
        add(P_IDLE, std::make_pair(4, 1), 4.0f, sheetRow1);
        add(P_RUN, std::make_pair(4, 4), 10.0f, sheetRow1); // Faster run animation
        add(P_SLIDE, std::make_pair(18, 2), 10.0f, sheetRow1);
        add(P_JUMP_START, pick(labelsRow1, "jump_start", 12, 2), 14.0f, sheetRow1);
        add(P_JUMP_MID, pick(labelsRow1, "jump_mid", 14, 2), 12.0f, sheetRow1);
        add(P_JUMP_PEAK, pick(labelsRow1, "jump_peak", 16, 1), 10.0f, sheetRow1);
        add(P_FALL, pick(labelsRow1, "fall", 17, 1), 10.0f, sheetRow1);
        add(P_ALERT, clamp(16, 1), 10.0f, sheetRow1);
        add(P_CROUCH, pick(labelsRow1, "crouch", 18, 1), 8.0f, sheetRow1);
        add(P_HURT, std::make_pair(19, 4), 12.0f, sheetRow1);
        add(P_VICTORY, pick(labelsRow1, "victory", 21, 3), 10.0f, sheetRow1);
        add(P_POWER_RUN, std::make_pair(4, 4), 12.0f, sheetRow1); // Faster power run
        add(P_JETPACK, pick(labelsRow1, "jetpack", 28, 3), 12.0f, sheetRow1);
        add(P_SHOOT, clamp(12, 2), 14.0f, sheetRow1);

        int r2 = (int)row2TotalFrames;
        auto clamp2 = [&](int start, int count)
        { return std::make_pair(start, std::max(0, std::min(count, std::max(0, r2 - start)))); };
        auto pick2 = [&](std::map<std::string, std::pair<int, int>> &labels, const std::string &name, int defStart, int defCount)
        { auto it = labels.find(name); if (it!=labels.end()) { int s=it->second.first, e=it->second.second; int c = std::max(1, e - s + 1); return std::make_pair(s,c); } return clamp2(defStart, defCount); };
        add(P_SHOOT_FORWARD, clamp2(0, 4), 14.0f, sheetRow2);
        add(P_SHOOT_SIDE, clamp2(4, 4), 14.0f, sheetRow2);
        add(P_KNOCKDOWN, pick2(labelsRow2, "knockdown", 8, 7), 16.0f, sheetRow2);
        add(P_DEATH_FADE, std::make_pair(1, 6), 8.0f, sheetRow2);
        add(P_LAND, pick2(labelsRow2, "land", 19, 1), 12.0f, sheetRow2);
    }

    void buildFrameRects()
    {
        row1Rects.clear();
        row2Rects.clear();
        auto uniform = [&](Texture *tex, std::vector<IntRect> &out)
        {
            if (!tex || tex->getSize().x == 0 || tex->getSize().y == 0)
                return;
            int w = (int)tex->getSize().x;
            int h = (int)tex->getSize().y;
            int fw = PLAYER_FRAME_WIDTH;
            int frames = std::max(1, w / fw);
            out.reserve(frames);
            for (int i = 0; i < frames; ++i)
                out.push_back(IntRect(i * fw, 0, fw, h));
        };
        uniform(sheetRow1, row1Rects);
        uniform(sheetRow2, row2Rects);
    }

    IntRect getRectForFrame(Texture *sheet, int index)
    {
        if (sheet == sheetRow1 && !row1Rects.empty())
        {
            index = std::max(0, std::min(index, (int)row1Rects.size() - 1));
            return row1Rects[index];
        }
        if (sheet == sheetRow2 && !row2Rects.empty())
        {
            index = std::max(0, std::min(index, (int)row2Rects.size() - 1));
            return row2Rects[index];
        }
        return IntRect(index * frameW, 0, frameW, frameH);
    }

    void applyPowerup(PowerupType type)
    {
        switch (type)
        {
        case POWERUP_SPEED:
            speedBoostTimer = 10.0f;
            speed = baseSpeed * 2.0f;
            break;
        case POWERUP_RANGE:
            rangeBoostTimer = 10.0f;
            vacuumAngle = baseVacuumAngle * 1.5f;
            break;
        case POWERUP_POWER:
            powerBoostTimer = 10.0f;
            vacuumRange = baseVacuumRange * 1.5f;
            break;
        case POWERUP_LIFE:
            health++;
            if (health > 5)
                health = 5;
            break;
        }
    }

    void updatePowerups(float dt)
    {
        if (speedBoostTimer > 0)
        {
            speedBoostTimer -= dt;
            if (speedBoostTimer <= 0)
                speed = baseSpeed;
        }
        if (rangeBoostTimer > 0)
        {
            rangeBoostTimer -= dt;
            if (rangeBoostTimer <= 0)
                vacuumAngle = baseVacuumAngle;
        }
        if (powerBoostTimer > 0)
        {
            powerBoostTimer -= dt;
            if (powerBoostTimer <= 0)
                vacuumRange = baseVacuumRange;
        }
    }

    void captureEnemy(int type)
    {
        if (capturedCount < maxCapacity)
        {
            capturedEnemies[capturedCount++] = type;
        }
    }

    int releaseOneEnemy()
    {
        if (capturedCount > 0)
        {
            int type = capturedEnemies[--capturedCount];
            capturedEnemies[capturedCount] = -1;
            return type;
        }
        return -1;
    }

    void releaseAllEnemies(int *types, int &count)
    {
        count = capturedCount;
        for (int i = 0; i < capturedCount; i++)
            types[i] = capturedEnemies[capturedCount - 1 - i];
        capturedCount = 0;
        for (int i = 0; i < 5; i++)
            capturedEnemies[i] = -1;
    }

    void update(float dt, char **map)
    {
        // Decrement powerup timers and restore base speed/range/angle when expired
        updatePowerups(dt);

        // Physics
        if (!onGround)
        {
            velocityY += GRAVITY;
            if (velocityY > TERMINAL_VELOCITY)
                velocityY = TERMINAL_VELOCITY;
        }

        // Apply horizontal movement scaled by frame time; vertical moves by velocityY
        x += velocityX * dt * 60.0f;
        y += velocityY;

        checkCollisions(map);

        // Track time spent in current animation state; treat tiny velocities as standing
        animStateTimer += dt;
        bool movingHoriz = fabs(velocityX) > 0.1f;

        // Store previous state for transition detection
        prevAnimState = animState;

        // Gate early exit when in hurt/knockdown/shoot sequences; otherwise evaluate movement
        if (!(animState == P_HURT && animStateTimer < 0.35f) &&
            !(animState == P_KNOCKDOWN && animStateTimer < 0.5f) &&
            !(animState == P_SHOOT && animStateTimer < 0.3f) &&
            !(animState == P_SHOOT_FORWARD && animStateTimer < 0.3f) &&
            !(animState == P_SHOOT_SIDE && animStateTimer < 0.3f))
        {

            if (onGround)
            {
                if (!movingHoriz)
                {
                    animState = P_IDLE;
                }
                else
                {
                    animState = (speedBoostTimer > 0) ? P_POWER_RUN : P_RUN;
                }
            }
            else
            {
                // Choose jump phase based on vertical velocity thresholds
                if (velocityY < -9.0f)
                {
                    animState = P_JUMP_START;
                }
                else if (velocityY < -2.0f)
                {
                    animState = P_JUMP_MID;
                }
                else if (fabs(velocityY) < 1.8f)
                {
                    animState = P_JUMP_PEAK;
                }
                else if (velocityY > 0)
                {
                    animState = P_FALL;
                }
            }
        }

        // Reset animation frame if state changed
        if (animState != prevAnimState)
        {
            animFrame = 0;
            animTimer = 0;
            animStateTimer = 0;
        }

        // Get current animation sequence
        AnimSequence &anim = animations[animState];
        seqSheet = anim.texture;
        seqStart = anim.startFrame;
        animFrames = anim.frameCount;
        animFPS = anim.fps;

        // Update animation frame
        animTimer += dt;
        if (animFrames > 1 && animTimer > (1.0f / animFPS))
        {
            animFrame = (animFrame + 1) % max(1, animFrames);
            animTimer = 0;
        }

        // Apply texture and frame
        if (seqSheet)
        {
            sprite.setTexture(*seqSheet);
            IntRect r = getRectForFrame(seqSheet, seqStart + animFrame);
            if (facingRight)
            {
                IntRect rflip(r.left + r.width, r.top, -r.width, r.height);
                sprite.setTextureRect(rflip);
            }
            else
            {
                sprite.setTextureRect(r);
            }
            frameW = r.width;
            frameH = r.height;
        }

        // Update sprite position and orientation
        float spriteX = x + (frameW * PLAYER_SCALE) / 2.0f;
        float spriteY = y + (frameH * PLAYER_SCALE);
        if (onGround)
            spriteY += (GROUND_SNAP_PX + 2.0f) * PLAYER_SCALE;
        sprite.setPosition(spriteX - (frameW * PLAYER_SCALE) / 2.0f,
                           spriteY - (frameH * PLAYER_SCALE));
        // strict compliance: no origin; adjust position when placing

        // Compute grid position under player's feet and at horizontal center
        int footRow = (int)((y + (frameH * PLAYER_SCALE)) / CELL_SIZE);
        int centerCol = (int)((x + (frameW * PLAYER_SCALE) / 2.0f) / CELL_SIZE);
        // Read tile type; default to empty
        char tile = ' ';
        if (footRow >= 0 && footRow < LEVEL_HEIGHT && centerCol >= 0 && centerCol < LEVEL_WIDTH)
            tile = map[footRow][centerCol];
        // Slope tiles are marked as 'S', '/' (down-right), '\\' (down-left)
        bool onSlope = (tile == 'S' || tile == '/' || tile == '\\');

        if (onSlope)
        {
            // Choose a neighbor tile to define slope direction
            int nrow = footRow;
            int ncol = centerCol + 1;
            if (ncol >= LEVEL_WIDTH)
                ncol = LEVEL_WIDTH - 1;
            if (tile == 'S')
            {
                // For generic 'S', check diagonal neighbors to resolve slope
                int r1 = std::min(LEVEL_HEIGHT - 1, footRow + 1);
                int c1 = std::min(LEVEL_WIDTH - 1, centerCol + 1);
                int r2 = std::min(LEVEL_HEIGHT - 1, footRow + 1);
                int c2 = std::max(0, centerCol - 1);
                if (map[r1][c1] == 'S')
                {
                    nrow = r1;
                    ncol = c1;
                }
                else if (map[r2][c2] == 'S')
                {
                    nrow = r2;
                    ncol = c2;
                }
            }

            // Compute slope normal and angle for sprite rotation
            float sx = centerCol * CELL_SIZE + CELL_SIZE / 2.0f;
            float sy = (float)platformTopYAt(footRow, centerCol);
            float ex = ncol * CELL_SIZE + CELL_SIZE / 2.0f;
            float ey = (float)platformTopYAt(nrow, ncol);
            float tx = ex - sx;
            float ty = ey - sy;
            float nx = -ty;
            float ny = tx;
            float slopeAngle = std::atan2(ny, nx) * 180.0f / 3.14159265f;

            // Determine downhill direction along the slope
            int downhill = 0;
            if (ey > sy)
            {
                downhill = (ex > sx) ? 1 : -1;
            }
            else if (ey < sy)
            {
                downhill = (ex > sx) ? -1 : 1;
            }

            // Read input to bias slide speed against/with the slope
            bool leftNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
            bool rightNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
            int inputDir = rightNow ? 1 : (leftNow ? -1 : 0);
            if (onGround && std::fabs(velocityX) > 0.1f)
            {
                animState = P_SLIDE;
            }
            // Align sprite to slope top and rotate to match angle
            sprite.setPosition(spriteX - (frameW * PLAYER_SCALE) / 2.0f,
                               (float)platformTopYAt(footRow, centerCol) - (frameH * PLAYER_SCALE));
            // strict compliance: no rotation, adjust Y to align visually
            // keep same placement by nudging position along slope
        }
        else
        {
            // Reset rotation on flat ground
            // rotation reset not needed under strict mode
        }

        // Flip horizontally when facing right (because frames are drawn mirrored)
        sprite.setScale(PLAYER_SCALE, PLAYER_SCALE);
    }

    void checkCollisions(char **map)
    {
        // Start as airborne until a support tile is detected beneath the feet
        onGround = false;

        int playerWidth = (int)(frameW * PLAYER_SCALE);
        int playerHeight = (int)(frameH * PLAYER_SCALE);

        int baseRow = (int)((y + playerHeight + 2) / CELL_SIZE);
        int leftX = (int)((x + 10) / CELL_SIZE);
        int rightX = (int)((x + playerWidth - 10) / CELL_SIZE);
        int centerX = (int)((x + playerWidth / 2) / CELL_SIZE);

        // Test if any of the three foot samples hit a solid or sloped tile
        auto isSolidAtRow = [&](int row)
        {
            if (row < 0 || row >= LEVEL_HEIGHT)
                return false;
            bool leftSolid = (leftX >= 0 && leftX < LEVEL_WIDTH && (map[row][leftX] == '#' || map[row][leftX] == '/' || map[row][leftX] == '\\' || map[row][leftX] == 'S'));
            bool centerSolid = (centerX >= 0 && centerX < LEVEL_WIDTH && (map[row][centerX] == '#' || map[row][centerX] == '/' || map[row][centerX] == '\\' || map[row][centerX] == 'S'));
            bool rightSolid = (rightX >= 0 && rightX < LEVEL_WIDTH && (map[row][rightX] == '#' || map[row][rightX] == '/' || map[row][rightX] == '\\' || map[row][rightX] == 'S'));
            return centerSolid || (leftSolid && rightSolid);
        };

        if (velocityY < 0)
        {
            int headRow = (int)((y - 2) / CELL_SIZE);
            auto solidAt = [&](int row, int col)
            {
                if (row >= 0 && row < LEVEL_HEIGHT && col >= 0 && col < LEVEL_WIDTH)
                    return map[row][col] == '#';
                return false;
            };
            float headOffset = facingRight ? 8.0f : -8.0f;
            int headLeftCol = (int)((x + headOffset + playerWidth * 0.25f) / CELL_SIZE);
            int headCenterCol = (int)((x + headOffset + playerWidth * 0.5f) / CELL_SIZE);
            int headRightCol = (int)((x + headOffset + playerWidth * 0.75f) / CELL_SIZE);
            int solids = 0;
            if (solidAt(headRow, headLeftCol)) solids++;
            if (solidAt(headRow, headCenterCol)) solids++;
            if (solidAt(headRow, headRightCol)) solids++;
            if (solids >= 2)
            {
                y = (headRow + 1) * CELL_SIZE;
                velocityY = 0;
            }
        }

        int hitRow = -1;
        if (isSolidAtRow(baseRow))
            hitRow = baseRow;
        else if (isSolidAtRow(baseRow + 1))
            hitRow = baseRow + 1;
        else if (isSolidAtRow(baseRow - 1))
            hitRow = baseRow - 1;

        // If a blocking tile was found, snap Y to its top and zero vertical velocity
        if (hitRow != -1)
        {
            int topY = platformTopYAt(hitRow, centerX) - playerHeight;
            if (velocityY >= 0 || y > topY)
            {
                y = topY;
                velocityY = 0;
                onGround = true;
            }
        }

        if (x < 0)
            x = 0;
        if (x > SCREEN_WIDTH - playerWidth)
            x = SCREEN_WIDTH - playerWidth;
        if (y > SCREEN_HEIGHT - playerHeight)
        {
            y = SCREEN_HEIGHT - playerHeight;
            velocityY = 0;
            onGround = true;
        }
    }

    void reset(int level)
    {
        x = 100;
        y = 700;
        velocityX = 0;
        velocityY = 0;
        health = 3;
        onGround = false;
        facingRight = true;
        capturedCount = 0;
        maxCapacity = (level == 1) ? 3 : 5;
        speed = baseSpeed;
        vacuumRange = baseVacuumRange;
        vacuumAngle = baseVacuumAngle;
        speedBoostTimer = 0;
        rangeBoostTimer = 0;
        powerBoostTimer = 0;
        jumpBoostTimer = 0.0f;
        for (int i = 0; i < 5; i++)
            capturedEnemies[i] = -1;
    }

    float getCenterX() { return x + (frameW * PLAYER_SCALE) / 2; }
    float getCenterY() { return y + (frameH * PLAYER_SCALE) / 2; }
    int getWidth() { return (int)(frameW * PLAYER_SCALE); }
    int getHeight() { return (int)(frameH * PLAYER_SCALE); }
    int getFrameWidth() const { return frameW; }
    int getFrameHeight() const { return frameH; }

    FloatRect getHitbox() const
    {
        float left = x + 28.0f * PLAYER_SCALE;
        float top = y + 20.0f * PLAYER_SCALE;
        float width = 0.42f * frameW * PLAYER_SCALE;
        float height = 0.80f * frameH * PLAYER_SCALE;
        return FloatRect(left, top, width, height);
    }
};

// ============================================================================
// ENEMY CLASS - ENHANCED ANIMATIONS
// ============================================================================
class Enemy
{
public:
    float x, y;
    float velocityX, velocityY;
    int type;
    bool active;
    bool captured;
    int capturePoints;
    float stateTimer;
    float actionTimer;
    float shootTimer;
    bool isVisible;
    bool isShooting;
    bool hasFired;
    bool onGround;
    float jumpCooldown;
    bool facingRight;

    int animFrame;
    float animTimer;
    float rowChangeTimer;
    float rowChangeInterval;
    bool seekingDrop;
    float dropTargetX;
    float pauseTimer;
    bool jumpingAcross;
    float jumpTargetX;
    int jumpTargetRow;

    bool teleporting;
    float teleportTimer;
    float teleportCooldown;
    float invisibleTimer;

    Sprite sprite;
    Texture *texPtr;
    Texture *texDimPtr;
    int texRows;
    int rowFrameCounts[4];
    int animRow;
    int animFrames;
    float animFPS;
    int seqStart;
    std::vector<std::vector<IntRect>> rowRects;
    int currW;
    int currH;
    int footPadPx;
    std::map<std::string, std::pair<int, int>> metaLabels;
    std::map<int, std::map<std::string, std::pair<int, int>>> metaLabelsPerRow;

    // Initialize enemy with type and spawn position; set default behaviors and timers
    Enemy(int t, float sx, float sy)
    {
        type = t;
        x = sx;
        y = sy;
        active = true;
        captured = false;
        velocityX = (rand() % 2 == 0) ? 2.0f : -2.0f;
        velocityY = 0;
        stateTimer = 0;
        actionTimer = 0;
        shootTimer = 0;
        isVisible = true;
        isShooting = false;
        hasFired = false;
        onGround = false;
        jumpCooldown = 0;
        facingRight = (velocityX > 0);
        animFrame = 0;
        animTimer = 0;
        rowChangeTimer = 0;
        rowChangeInterval = 4.0f + (rand() % 300) / 100.0f;
        seekingDrop = false;
        dropTargetX = x;
        pauseTimer = 0;
        jumpingAcross = false;
        jumpTargetX = x;
        jumpTargetRow = -1;
        teleporting = false;
        teleportTimer = 0;
        teleportCooldown = 0;
        invisibleTimer = 0;

        texPtr = nullptr;
        texDimPtr = nullptr;
        texRows = 1;
        rowFrameCounts[0] = 26;
        rowFrameCounts[1] = 0;
        rowFrameCounts[2] = 0;
        rowFrameCounts[3] = 0;
        animRow = 0;
        animFrames = 4;
        animFPS = 4.0f;
        seqStart = 0;
        footPadPx = 0;

        // Grant capture scores per enemy type
        switch (type)
        {
        case 0:
            capturePoints = 50;
            break;
        case 1:
            capturePoints = 75;
            break;
        case 2:
            capturePoints = 150;
            break;
        case 3:
            capturePoints = 200;
            break;
        }
    }

    bool isSolidAt(char **map, int row, int col)
    {
        return (row >= 0 && row < LEVEL_HEIGHT && col >= 0 && col < LEVEL_WIDTH && map[row][col] == '#');
    }

    void chooseReappearPosition(char **map, float playerX, float playerY, float &outX, float &outY)
    {
        int preferredCol = (int)(playerX / CELL_SIZE);
        int playerRowIdx = std::max(0, std::min(LEVEL_HEIGHT - 1, (int)(playerY / CELL_SIZE)));

        int bestRow = -1;
        int bestCol = preferredCol;
        int bestDist = LEVEL_HEIGHT + 5;
        for (int radius = 0; radius < LEVEL_WIDTH; ++radius)
        {
            int cands[2] = {preferredCol - radius, preferredCol + radius};
            for (int ci = 0; ci < 2; ++ci)
            {
                int c = cands[ci];
                if (c < 0 || c >= LEVEL_WIDTH)
                    continue;
                for (int r = 0; r < LEVEL_HEIGHT; ++r)
                {
                    if (isSolidAt(map, r, c))
                    {
                        int d = std::abs(r - playerRowIdx);
                        if (d < bestDist)
                        {
                            bestDist = d;
                            bestRow = r;
                            bestCol = c;
                        }
                        break;
                    }
                }
            }
            if (bestRow != -1)
                break;
        }

        if (bestRow == -1)
        {
            bestRow = std::max(0, std::min(LEVEL_HEIGHT - 1, playerRowIdx));
            bestCol = std::max(0, std::min(LEVEL_WIDTH - 1, preferredCol));
        }

        int leftCol = bestCol, rightCol = bestCol;
        while (leftCol > 0 && isSolidAt(map, bestRow, leftCol))
            leftCol--;
        while (rightCol < LEVEL_WIDTH - 1 && isSolidAt(map, bestRow, rightCol))
            rightCol++;
        int segLeftCol = leftCol + 1;
        int segRightCol = rightCol - 1;

        float segLeftX = segLeftCol * CELL_SIZE + 8.0f;
        float segRightX = segRightCol * CELL_SIZE + (CELL_SIZE - getWidth() - 8.0f);
        float targetX = playerX;
        if (segLeftCol >= 0 && segRightCol >= segLeftCol)
        {
            float width = segRightX - segLeftX;
            float prefer = 0.5f;
            if (playerX >= segLeftX && playerX <= segRightX && width > 0)
                prefer = (playerX - segLeftX) / width;
            float jitter = ((rand() % 41) - 20) / 100.0f;
            float mix = std::max(0.2f, std::min(0.8f, prefer + jitter));
            targetX = segLeftX + mix * std::max(0.0f, width);
        }

        outX = targetX;
        outY = bestRow * CELL_SIZE - getHeight() - 2.0f;
    }

    bool findJumpTarget(char **map, int currentRow)
    {
        int centerCol = (int)((x + getWidth() / 2) / CELL_SIZE);
        int targetRow = -1;

        for (int r = currentRow - 1; r >= 0; --r)
        {
            if (isSolidAt(map, r, centerCol))
            {
                targetRow = r;
                break;
            }
        }

        if (targetRow == -1)
        {
            for (int r = currentRow + 1; r < LEVEL_HEIGHT; ++r)
            {
                if (isSolidAt(map, r, centerCol))
                {
                    targetRow = r;
                    break;
                }
            }
        }

        if (targetRow == -1)
            return false;

        int leftCol = centerCol, rightCol = centerCol;
        while (leftCol > 0 && isSolidAt(map, targetRow, leftCol))
            leftCol--;
        while (rightCol < LEVEL_WIDTH - 1 && isSolidAt(map, targetRow, rightCol))
            rightCol++;

        int segLeftCol = leftCol + 1;
        int segRightCol = rightCol - 1;
        int targetCol = max(segLeftCol, min(centerCol, segRightCol));

        jumpTargetX = targetCol * CELL_SIZE + 8.0f;
        jumpTargetRow = targetRow;
        return true;
    }

    bool parseInt(const std::string &s, size_t &pos, int &out)
    {
        while (pos < s.size() && !(s[pos] == '-' || (s[pos] >= '0' && s[pos] <= '9')))
            pos++;
        if (pos >= s.size())
            return false;
        size_t start = pos;
        while (pos < s.size() && (s[pos] == '-' || (s[pos] >= '0' && s[pos] <= '9')))
            pos++;
        try
        {
            out = std::stoi(s.substr(start, pos - start));
        }
        catch (...)
        {
            return false;
        }
        return true;
    }

    bool loadEnemyMetaForType()
    {
        rowRects.clear();
        metaLabels.clear();
        metaLabelsPerRow.clear();
        buildEnemyFrameRects();

        if (type == 2)
        {
            int frames = (!rowRects.empty()) ? (int)rowRects[0].size() : 0;
            int idleA = 0;
            int idleB = std::max(0, std::min(frames - 1, 3));
            int runA = std::min(frames > 0 ? idleB + 1 : 0, std::max(0, frames - 5));
            int runB = std::max(0, std::min(frames - 1, runA + 10));
            int spinA = std::max(0, frames - 8);
            int spinB = std::max(0, frames - 5);
            int recA = std::max(0, frames - 4);
            int recB = std::max(0, frames - 1);
            metaLabels["idle"] = std::make_pair(idleA, idleB);
            metaLabels["run"] = std::make_pair(runA, runB);
            metaLabels["spin"] = std::make_pair(spinA, spinB);
            metaLabels["recover"] = std::make_pair(recA, recB);
        }
        else if (type == 3)
        {
            int frames = (!rowRects.empty()) ? (int)rowRects[0].size() : 0;
            int walkC = std::min(6, std::max(1, frames));
            int jumpC = std::min(3, std::max(0, frames - walkC));
            int atkC = std::min(3, std::max(0, frames - walkC - jumpC));
            int walkS = 0;
            int jumpS = walkS + walkC;
            int atkS = jumpS + jumpC;
            metaLabelsPerRow[0]["walk"] = std::make_pair(walkS, walkS + walkC - 1);
            metaLabelsPerRow[1]["jump"] = std::make_pair(jumpS, jumpS + jumpC - 1);
            metaLabelsPerRow[2]["attack_orb"] = std::make_pair(atkS, atkS + atkC - 1);
        }
        else if (type == 1)
        {
            int frames = (!rowRects.empty()) ? (int)rowRects[0].size() : 0;
            int idleA = 0;
            int idleB = std::max(0, std::min(frames - 1, 6));
            metaLabels["idle"] = std::make_pair(idleA, idleB);
            metaLabels["capture"] = std::make_pair(std::min(frames - 1, 25), std::min(frames - 1, 33));
            metaLabels["hit"] = std::make_pair(std::min(frames - 1, 19), std::min(frames - 1, 24));
        }
        else if (type == 0)
        {
            int frames = (!rowRects.empty()) ? (int)rowRects[0].size() : 0;
            int idleA = 0;
            int idleB = std::max(0, std::min(frames - 1, 5));
            int chaseA = std::min(frames - 1, idleB + 1);
            int chaseB = std::max(chaseA, std::min(frames - 1, chaseA + 10));
            int chargeA = std::max(chaseB + 1, frames / 2);
            int chargeB = std::max(chargeA, std::min(frames - 1, chargeA + 2));
            metaLabels["idle"] = std::make_pair(idleA, idleB);
            metaLabels["chase"] = std::make_pair(chaseA, chaseB);
            metaLabels["charge"] = std::make_pair(chargeA, chargeB);
        }

        return !rowRects.empty();
    }

    // Bind texture, scale, and origin for consistent collision and rotation
    void setupSprite(Texture &tex, Texture *dimTex = nullptr)
    {
        texPtr = &tex;
        texDimPtr = dimTex;
        sprite.setTexture(tex);
        bool loadedMeta = loadEnemyMetaForType();
        if (!loadedMeta)
            buildEnemyFrameRects();
        texRows = (int)rowRects.size();
        for (int r = 0; r < 4; ++r)
        {
            rowFrameCounts[r] = (r < texRows) ? (int)rowRects[r].size() : 0;
        }

        animRow = 0;
        seqStart = 0;
        animFrames = (rowFrameCounts[0] > 0) ? std::min(4, rowFrameCounts[0]) : 1;
        animFPS = 6.0f + (rand() % 30) / 10.0f; // Varied animation speed

        IntRect r;
        if (texRows > 0 && !rowRects[0].empty())
            r = rowRects[0][0];
        else
        {
            int inferredRows = std::max(1, (int)(tex.getSize().y / ENEMY_FRAME_HEIGHT));
            int inferredFrames = std::max(1, (int)(tex.getSize().x / ENEMY_FRAME_WIDTH));
            int fh = tex.getSize().y / std::max(1, inferredRows);
            int fw = tex.getSize().x / std::max(1, inferredFrames);
            r = IntRect(0, 0, fw, fh);
        }
        sprite.setTextureRect(r);
        currW = r.width;
        currH = r.height;
        sprite.setScale(ENEMY_SCALE, ENEMY_SCALE);

        if (type == 0)
            footPadPx = 12;
        else
            footPadPx = 0;
    }

    void buildEnemyFrameRects()
    {
        rowRects.clear();
        if (!texPtr || texPtr->getSize().x == 0 || texPtr->getSize().y == 0)
            return;
        int W = (int)texPtr->getSize().x;
        int H = (int)texPtr->getSize().y;

        // MASTERCLASS: Use actual sprite dimensions based on enemy type
        int fh = H;                 // Use full height for horizontal strip sprites
        int fw = ENEMY_FRAME_WIDTH; // 64 pixel frame width

        // Chelnov uses a 3x4 grid layout
        if (type == 3)
        {
            int cols = 3;
            int rows = 4;
            int tileW = std::max(1, W / cols);
            int tileH = std::max(1, H / rows);
            for (int r = 0; r < rows; ++r)
            {
                std::vector<IntRect> rects;
                int y0 = r * tileH;
                int h = (r == rows - 1) ? (H - y0) : tileH;
                for (int i = 0; i < cols; ++i)
                {
                    int x0 = i * tileW;
                    int w = (i == cols - 1) ? (W - x0) : tileW;
                    rects.push_back(IntRect(x0, y0, w, h));
                }
                rowRects.push_back(rects);
            }
            return;
        }

        // MASTERCLASS: For horizontal strip sprites (ghost, skeleton, invisible)
        // Calculate proper frame width based on sprite analysis
        int framesPerRow = std::max(1, W / fw);

        // Single row sprite sheets (ghost, skeleton, invisible are all single row)
        std::vector<IntRect> rects;
        rects.reserve(framesPerRow);
        for (int i = 0; i < framesPerRow; ++i)
        {
            rects.push_back(IntRect(i * fw, 0, fw, fh));
        }
        rowRects.push_back(rects);
    }

    IntRect getEnemyRect(int row, int index)
    {
        if (row >= 0 && row < (int)rowRects.size() && !rowRects[row].empty())
        {
            index = std::max(0, std::min(index, (int)rowRects[row].size() - 1));
            return rowRects[row][index];
        }
        if (!rowRects.empty())
        {
            int r0Count = (int)rowRects[0].size();
            index = std::max(0, std::min(index, r0Count - 1));
            return rowRects[0][index];
        }
        int fh = ENEMY_FRAME_HEIGHT;
        return IntRect(index * ENEMY_FRAME_WIDTH, 0, ENEMY_FRAME_WIDTH, fh);
    }

    void update(float dt, char **map, float playerX, float playerY)
    {
        if (!active || captured)
            return;

        stateTimer += dt;
        actionTimer += dt;
        shootTimer += dt;
        if (pauseTimer > 0)
            pauseTimer -= dt;
        if (jumpCooldown > 0)
            jumpCooldown -= dt;

        // MASTERCLASS: Smooth animation updates with better timing
        animTimer += dt;
        float effectiveFPS = std::max(1.0f, animFPS);
        if (animTimer > (1.0f / effectiveFPS))
        {
            animFrame = (animFrame + 1) % max(1, animFrames);
            if (texPtr)
            {
                IntRect r = getEnemyRect(animRow, seqStart + animFrame);
                sprite.setTextureRect(r);
                currW = r.width;
                currH = r.height;
            }
            animTimer = 0;
        }

        if (velocityX > 0)
            facingRight = true;
        else if (velocityX < 0)
            facingRight = false;

        switch (type)
        {
        case 0:
            updateGhost(dt, map, playerX, playerY);
            break;
        case 1:
            updateSkeleton(dt, map, playerX, playerY);
            break;
        case 2:
            updateInvisibleMan(dt, map, playerX, playerY);
            break;
        case 3:
            updateChelnov(dt, map, playerX, playerY);
            break;
        }

        // Platform edge behavior
        if (onGround)
        {
            int bottomY = (int)((y + getHeight() + 2) / CELL_SIZE);
            int centerCol = (int)((x + getWidth() / 2) / CELL_SIZE);

            auto isSolidBelow = [&](int col)
            {
                if (bottomY >= 0 && bottomY < LEVEL_HEIGHT && col >= 0 && col < LEVEL_WIDTH)
                    return map[bottomY][col] == '#';
                return false;
            };

            int leftBound = centerCol;
            while (leftBound > 0 && isSolidBelow(leftBound))
                leftBound--;

            int rightBound = centerCol;
            while (rightBound < LEVEL_WIDTH - 1 && isSolidBelow(rightBound))
                rightBound++;

            float segLeftX = (leftBound + 1) * CELL_SIZE + 8.0f;
            float segRightX = rightBound * CELL_SIZE + (CELL_SIZE - getWidth() - 8.0f);

            if (!jumpingAcross)
            {
                // Check for gap/edge
                bool atEdge = (x <= segLeftX || x >= segRightX);
                
                if (atEdge)
                {
                     // Try to jump first
                    if (findJumpTarget(map, bottomY))
                    {
                         jumpingAcross = true;
                         velocityY = -9.0f;
                         // Jump towards target
                         velocityX = (x < jumpTargetX) ? 2.0f : -2.0f; 
                         jumpCooldown = 5.0f;
                    }
                    else
                    {
                         // Turn around if cannot jump
                         if (x <= segLeftX) velocityX = 0.5f;
                         else if (x >= segRightX) velocityX = -0.5f;
                    }
                }

                 if (pauseTimer <= 0 && fabs(velocityX) < 0.05f && !isShooting)
                {
                    velocityX = facingRight ? 0.5f : -0.5f;
                }
            }

            if (!isShooting && pauseTimer <= 0 && jumpCooldown <= 0 && rand() % 100 < 2)
            {
                if (findJumpTarget(map, bottomY))
                {
                    jumpingAcross = true;
                    velocityY = -9.0f;
                    velocityX = (x < jumpTargetX) ? fabs(velocityX) : -fabs(velocityX);
                    jumpCooldown = 5.0f;
                }
            }
        }

        rowChangeTimer += dt;
        if (onGround && pauseTimer <= 0 && stateTimer > 1.5f && rand() % 100 < 3)
        {
            pauseTimer = 0.8f + (rand() % 40) / 100.0f;
            velocityX = 0;
            stateTimer = 0;
        }

        if (jumpingAcross)
        {
            if (x < jumpTargetX)
                velocityX = 0.5f;
            else if (x > jumpTargetX)
                velocityX = -0.5f;

            if (onGround)
            {
                int bottomY = (int)((y + getHeight() + 2) / CELL_SIZE);
                if (bottomY == jumpTargetRow)
                {
                    jumpingAcross = false;
                    jumpCooldown = 4.5f;
                    pauseTimer = 0.6f;
                    velocityX = facingRight ? 0.5f : -0.5f;
                }
            }
        }

        if (facingRight)
        {
            sprite.setScale(ENEMY_SCALE, ENEMY_SCALE);
            sprite.setPosition(x, y + footPadPx * ENEMY_SCALE);
        }
        else
        {
            sprite.setScale(-ENEMY_SCALE, ENEMY_SCALE);
            sprite.setPosition(x + currW * ENEMY_SCALE, y + footPadPx * ENEMY_SCALE);
        }
    }

    void updateGhost(float dt, char **map, float playerX, float playerY)
    {
        if (!onGround)
        {
            velocityY += GRAVITY;
            if (velocityY > TERMINAL_VELOCITY)
                velocityY = TERMINAL_VELOCITY;
        }

        {
            float base = 2.0f;
            velocityX = (velocityX >= 0) ? base : -base;
        }
        if (pauseTimer <= 0)
            x += velocityX * dt * 60.0f;
        y += velocityY * dt * 60.0f;
        checkPlatformCollision(map);

        if (pauseTimer <= 0 && stateTimer > 1.0f && rand() % 100 < 6)
        {
            pauseTimer = 1.0f + (rand() % 80) / 100.0f;
            stateTimer = 0;
            if (rand() % 100 < 50)
                velocityX = -velocityX;
        }

        float distX = fabs(getCenterX() - playerX);
        float distY = fabs(getCenterY() - playerY);
        bool veryClose = (distX < 140 && distY < 110);
        bool closeToPlayer = (distX < 200 && distY < 140);

        animRow = 0;
        int framesInRow = max(1, rowFrameCounts[animRow]);
        int idleStart = 0, idleCount = min(7, framesInRow);
        int chaseStart = max(idleCount, framesInRow - 11), chaseCount = max(1, min(11, framesInRow - chaseStart));
        int chargeStart = max(idleCount, framesInRow / 2), chargeCount = max(1, min(2, framesInRow - chargeStart));
        if (!metaLabels.empty())
        {
            auto setRange = [&](const std::string &name, int &s, int &c)
            { auto it = metaLabels.find(name); if (it!=metaLabels.end()) { s = it->second.first; c = max(1, it->second.second - it->second.first + 0); } };
            setRange("idle", idleStart, idleCount);
            setRange("chase", chaseStart, chaseCount);
            setRange("charge", chargeStart, chargeCount);
        }

        if (isShooting)
        {
            actionTimer += dt;
            int atkStart = chargeStart + 1;
            int atkCount = chargeCount;
            if (!metaLabels.empty())
            {
                auto it = metaLabels.find("attack");
                if (it != metaLabels.end())
                {
                    atkStart = it->second.first;
                    atkCount = max(1, it->second.second - it->second.first + 0);
                }
            }
            seqStart = atkStart;
            animFrames = atkCount;
            animFPS = 10.0f; // MASTERCLASS: Faster attack animation
            if (actionTimer >= 0.35f)
            {
                isShooting = false;
            }
        }
        else if (veryClose && shootTimer >= 3.5f)
        {
            isShooting = true;
            shootTimer = 0;
            actionTimer = 0;
            seqStart = chargeStart;
            animFrames = max(1, min(chargeCount, 1));
            animFPS = 10.0f; // MASTERCLASS: Faster charge animation
        }
        else if (closeToPlayer)
        {
            seqStart = chaseStart;
            animFrames = chaseCount;
            animFPS = 10.0f; // MASTERCLASS: Smooth chase animation
        }
        else
        {
            seqStart = idleStart;
            animFrames = idleCount;
            animFPS = 0.6f;
        }

        if (pauseTimer <= 0 && stateTimer > 2.2f && rand() % 100 < 4)
        {
            velocityX = -velocityX;
            stateTimer = 0;
        }

        if (x < 50)
        {
            x = 50;
            velocityX = fabs(velocityX);
        }
        if (x > SCREEN_WIDTH - getWidth() - 50)
        {
            x = SCREEN_WIDTH - getWidth() - 50;
            velocityX = -fabs(velocityX);
        }
    }

    void updateSkeleton(float dt, char **map, float playerX, float playerY)
    {
        if (!onGround)
        {
            velocityY += GRAVITY;
            if (velocityY > TERMINAL_VELOCITY)
                velocityY = TERMINAL_VELOCITY;
        }

        float distX = fabs(getCenterX() - playerX);
        float distY = fabs(getCenterY() - playerY);
        bool canThrow = (distX < 260 && distY < 160);

        int framesInRow = max(1, rowFrameCounts[0]);
        int idleStart = 0, idleCount = min(7, framesInRow);
        int windupStart = max(idleCount, framesInRow / 3), windupCount = max(1, min(4, framesInRow - windupStart));
        int throwStart = windupStart + windupCount, throwCount = max(1, min(7, framesInRow - throwStart));
        int jumpStart = throwStart + throwCount, jumpCount = max(1, min(10, framesInRow - jumpStart));
        if (!metaLabels.empty())
        {
            auto setRange = [&](const std::string &name, int &s, int &c)
            { auto it = metaLabels.find(name); if (it!=metaLabels.end()) { s = it->second.first; c = max(1, it->second.second - it->second.first + 0); } };
            setRange("idle", idleStart, idleCount);
            int aS = 0, aC = 0;
            setRange("attack", aS, aC);
            if (aC > 0)
            {
                throwStart = aS;
                throwCount = aC;
                windupStart = aS;
                windupCount = min(2, aC);
            }
            int jS = 0, jC = 0;
            setRange("jump", jS, jC);
            if (jC > 0)
            {
                jumpStart = jS;
                jumpCount = jC;
            }
        }

        if (onGround && shootTimer >= 5.0f && canThrow)
        {
            isShooting = true;
            shootTimer = 0;
            actionTimer = 0;
            hasFired = false;
        }

        if (isShooting)
        {
            actionTimer += dt;
            if (actionTimer < 0.6f)
            {
                animRow = 0;
                seqStart = windupStart;
                animFrames = windupCount;
                animFPS = 8.0f; // MASTERCLASS: Smooth windup
            }
            else if (actionTimer < 1.4f)
            {
                animRow = 0;
                seqStart = throwStart;
                animFrames = throwCount;
                animFPS = 10.0f; // MASTERCLASS: Fast throw
            }
            else
            {
                isShooting = false;
            }
        }

        if (pauseTimer <= 0)
        {
            if (velocityX == 0) velocityX = 2.0f;
            x += velocityX * (dt * 60.0f);
        }
        y += velocityY * (dt * 60.0f);
        checkPlatformCollision(map);

        if (!isShooting && (!onGround || jumpingAcross))
        {
            animRow = 0;
            seqStart = jumpStart;
            animFrames = jumpCount;
            animFPS = 10.0f; // MASTERCLASS: Smooth jump animation
        }
        else if (!isShooting && onGround)
        {
            animRow = 0;
            int wS = idleStart;
            int wC = idleCount;
            auto itWalk = metaLabels.find("walk");
            if (itWalk != metaLabels.end())
            {
                wS = itWalk->second.first;
                wC = max(1, itWalk->second.second - itWalk->second.first + 0);
            }
            bool moving = (fabs(velocityX) > 0.1f && pauseTimer <= 0);
            seqStart = moving ? wS : idleStart;
            animFrames = moving ? wC : idleCount;
            animFPS = moving ? 8.0f : 0.6f;
        }

        if (x < 10)
        {
            x = 10;
            velocityX = fabs(velocityX);
        }
        if (x > SCREEN_WIDTH - getWidth() - 10)
        {
            x = SCREEN_WIDTH - getWidth() - 10;
            velocityX = -fabs(velocityX);
        }
    }

    void updateInvisibleMan(float dt, char **map, float playerX, float playerY)
    {
        // MASTERCLASS: Invisible man is ALWAYS visible (uses dim texture during teleport)
        if (!onGround)
        {
            velocityY += GRAVITY;
            if (velocityY > TERMINAL_VELOCITY)
                velocityY = TERMINAL_VELOCITY;
        }

        if (pauseTimer <= 0)
        {
            if (velocityX == 0) velocityX = 2.0f;
            x += velocityX * (dt * 60.0f);
        }
        y += velocityY * (dt * 60.0f);
        checkPlatformCollision(map);

        if (x < 50)
        {
            x = 50;
            velocityX = fabs(velocityX);
        }
        if (x > SCREEN_WIDTH - getWidth() - 50)
        {
            x = SCREEN_WIDTH - getWidth() - 50;
            velocityX = -fabs(velocityX);
        }

        // Teleport logic - but always keep visible (use dim texture)
        bool startTeleport = (actionTimer > 0.8f && rand() % 100 < 40);
        if (teleportCooldown <= 0 && startTeleport && !jumpingAcross)
        {
            teleporting = true;
            isVisible = true; // MASTERCLASS: Keep visible
            if (texDimPtr)
                sprite.setTexture(*texDimPtr); // Use dim texture for "fading" effect
            teleportTimer = 0;
            invisibleTimer = 0;
            pauseTimer = 0.05f;
            velocityX = 0;
            animRow = 0;
            int sS = 0, sC = 3;
            auto itSpin = metaLabels.find("spin");
            if (itSpin != metaLabels.end())
            {
                sS = itSpin->second.first;
                sC = max(1, itSpin->second.second - itSpin->second.first + 0);
            }
            seqStart = sS;
            animFrames = sC;
            animFPS = 12.0f; // MASTERCLASS: Fast spin animation

            chooseReappearPosition(map, playerX, playerY, jumpTargetX, y);
            jumpTargetRow = std::max(0, std::min(LEVEL_HEIGHT - 1, (int)((y + getHeight()) / CELL_SIZE)));
        }

        if (teleporting)
        {
            teleportTimer += dt;
            invisibleTimer += dt;
            if (teleportTimer >= 0.12f || invisibleTimer >= 0.25f)
            {
                float tx, ty;
                chooseReappearPosition(map, playerX, playerY, tx, ty);
                x = tx;
                y = ty;
                isVisible = true;
                if (texPtr)
                    sprite.setTexture(*texPtr); // Switch back to normal texture
                teleporting = false;
                teleportCooldown = 0.3f + (rand() % 30) / 100.0f;
                pauseTimer = 0.3f;
                animRow = 0;
                int rS = 0, rC = 3;
                auto itRec = metaLabels.find("recover");
                if (itRec != metaLabels.end())
                {
                    rS = itRec->second.first;
                    rC = max(1, itRec->second.second - itRec->second.first + 0);
                }
                seqStart = rS;
                animFrames = rC;
                animFPS = 10.0f;
                facingRight = (playerX >= x);
            }
        }

        if (teleportCooldown > 0)
            teleportCooldown -= dt;

        // Animation selection (when not teleporting)
        if (!teleporting)
        {
            animRow = 0;
            int runS = 0;
            int runC = max(1, rowFrameCounts[0] > 0 ? rowFrameCounts[0] : 1);
            auto itRun = metaLabels.find("run");
            if (itRun != metaLabels.end())
            {
                runS = itRun->second.first;
                runC = max(1, itRun->second.second - itRun->second.first + 0);
            }
            bool moving = (fabs(velocityX) > 0.1f && pauseTimer <= 0);
            seqStart = moving ? runS : (metaLabels.count("idle") ? metaLabels["idle"].first : 0);
            animFrames = moving ? runC : (metaLabels.count("idle") ? max(1, metaLabels["idle"].second - metaLabels["idle"].first + 0) : 3);
            animFPS = moving ? 10.0f : 6.0f; // MASTERCLASS: Smooth animations
        }
    }

    void updateChelnov(float dt, char **map, float playerX, float playerY)
    {
        if (shootTimer >= 4.0f)
        {
            isShooting = true;
            shootTimer = 0;
            actionTimer = 0;
            hasFired = false;
        }

        if (isShooting)
        {
            actionTimer += dt;
            if (actionTimer >= 1.0f)
            {
                isShooting = false;
            }
            animRow = 2;
            int atkS = 3, atkC = 3;
            if (!metaLabelsPerRow.empty())
            {
                auto itR = metaLabelsPerRow.find(2);
                if (itR != metaLabelsPerRow.end())
                {
                    auto it = itR->second.find("attack_orb");
                    if (it != itR->second.end())
                    {
                        atkS = it->second.first;
                        atkC = max(1, it->second.second - it->second.first + 0);
                    }
                }
            }
            seqStart = atkS;
            animFrames = atkC;
            animFPS = 6.0f;
        }

        if (!isShooting)
        {
            if (!onGround)
            {
                velocityY += GRAVITY;
                if (velocityY > TERMINAL_VELOCITY)
                    velocityY = TERMINAL_VELOCITY;
            }

            if (pauseTimer <= 0)
            {
                if (velocityX == 0) velocityX = 2.0f;
                x += velocityX * (dt * 60.0f);
            }
            y += velocityY * (dt * 60.0f);
            checkPlatformCollision(map);

            int currentRow = (int)((y + getHeight()) / CELL_SIZE);
            if (onGround && !jumpingAcross && jumpCooldown <= 0 && rand() % 100 < 4)
            {
                if (findJumpTarget(map, currentRow))
                {
                    jumpingAcross = true;
                    velocityY = -11.0f;
                    jumpCooldown = 3.5f;
                }
            }

            if (jumpingAcross)
            {
                if (x < jumpTargetX)
                    velocityX = 0.5f;
                else if (x > jumpTargetX)
                    velocityX = -0.5f;

                if (onGround)
                {
                    jumpingAcross = false;
                    pauseTimer = 0.6f;
                }
            }

            if (onGround)
            {
                animRow = 0;
                int wS = 0, wC = 5;
                if (!metaLabelsPerRow.empty())
                {
                    auto itR = metaLabelsPerRow.find(0);
                    if (itR != metaLabelsPerRow.end())
                    {
                        auto it = itR->second.find("walk");
                        if (it != itR->second.end())
                        {
                            wS = it->second.first;
                            wC = max(1, it->second.second - it->second.first + 0);
                        }
                    }
                }
                seqStart = wS;
                animFrames = wC;
                animFPS = 1.2f;
            }
            else
            {
                animRow = 1;
                int jS = 0, jC = 3;
                if (!metaLabelsPerRow.empty())
                {
                    auto itR = metaLabelsPerRow.find(1);
                    if (itR != metaLabelsPerRow.end())
                    {
                        auto it = itR->second.find("jump");
                        if (it != itR->second.end())
                        {
                            jS = it->second.first;
                            jC = max(1, it->second.second - it->second.first + 0);
                        }
                    }
                }
                seqStart = jS;
                animFrames = jC;
                animFPS = 1.5f;
            }
        }

        if (x < 50)
        {
            x = 50;
            velocityX = fabs(velocityX);
        }
        if (x > SCREEN_WIDTH - getWidth() - 50)
        {
            x = SCREEN_WIDTH - getWidth() - 50;
            velocityX = -fabs(velocityX);
        }
    }

    void checkPlatformCollision(char **map)
    {
        onGround = false;

        int baseRow = (int)((y + getHeight() + 2) / CELL_SIZE);
        int leftX = (int)((x + 5) / CELL_SIZE);
        int rightX = (int)((x + getWidth() - 5) / CELL_SIZE);
        int centerX = (int)((x + getWidth() / 2) / CELL_SIZE);

        auto isSolidAtRow = [&](int row)
        {
            if (row < 0 || row >= LEVEL_HEIGHT)
                return false;
            bool coll = false;
            if (leftX >= 0 && leftX < LEVEL_WIDTH && (map[row][leftX] == '#' || map[row][leftX] == '/' || map[row][leftX] == '\\' || map[row][leftX] == 'S'))
                coll = true;
            if (centerX >= 0 && centerX < LEVEL_WIDTH && (map[row][centerX] == '#' || map[row][centerX] == '/' || map[row][centerX] == '\\' || map[row][centerX] == 'S'))
                coll = true;
            if (rightX >= 0 && rightX < LEVEL_WIDTH && (map[row][rightX] == '#' || map[row][rightX] == '/' || map[row][rightX] == '\\' || map[row][rightX] == 'S'))
                coll = true;
            return coll;
        };

        int hitRow = -1;
        if (isSolidAtRow(baseRow))
            hitRow = baseRow;
        else if (isSolidAtRow(baseRow + 1))
            hitRow = baseRow + 1;
        else if (isSolidAtRow(baseRow - 1))
            hitRow = baseRow - 1;

        if (hitRow != -1 && velocityY >= 0)
        {
            y = platformTopYAt(hitRow, centerX) - getHeight();
            velocityY = 0;
            onGround = true;
        }

        if (y > SCREEN_HEIGHT - getHeight())
        {
            y = SCREEN_HEIGHT - getHeight();
            velocityY = 0;
            onGround = true;
        }
    }

    bool canBeCapture()
    {
        if (type == 2)
            return isVisible;
        if (type == 3)
            return !isShooting;
        return true;
    }

    bool isInVacuumRange(float px, float py, int dir, float range, float angle)
    {
        float dx = getCenterX() - px;
        float dy = getCenterY() - py;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist > range)
            return false;

        float angleToEnemy = atan2(dy, dx) * 180.0f / 3.14159f;
        float targetAngle = (dir == 0) ? 0 : (dir == 1) ? -90
                                         : (dir == 2)   ? 180
                                                        : 90;
        float angleDiff = fabs(angleToEnemy - targetAngle);
        if (angleDiff > 180)
            angleDiff = 360 - angleDiff;

        return angleDiff < angle;
    }

    void pullTowards(float px, float py, float power)
    {
        float dx = px - getCenterX();
        float dy = py - getCenterY();
        float dist = sqrt(dx * dx + dy * dy);
        if (dist > 5)
        {
            x += (dx / dist) * power;
            y += (dy / dist) * power;
        }
    }

    bool collidesWith(float px, float py, int pw, int ph)
    {
        return (x < px + pw - 10 && x + getWidth() > px + 10 &&
                y < py + ph - 10 && y + getHeight() > py + 10);
    }

    float getCenterX() { return x + (currW * ENEMY_SCALE) / 2; }
    float getCenterY() { return y + (currH * ENEMY_SCALE) / 2; }
    int getWidth() { return (int)(currW * ENEMY_SCALE); }
    int getHeight() { return (int)(currH * ENEMY_SCALE); }
};

// ============================================================================
// ENEMY PROJECTILE CLASS
// ============================================================================
class EnemyProjectile
{
public:
    float x, y;
    float velocityX, velocityY;
    bool active;
    Sprite sprite;
    int frames;
    int frameIndex;
    float animTimer;
    float animFPS;
    int frameW;
    int texH;
    bool hasTexture;

    EnemyProjectile(float sx, float sy, float tx, float ty)
    {
        x = sx;
        y = sy;
        active = true;

        float dx = tx - sx;
        float dy = ty - sy;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist > 0)
        {
            velocityX = (dx / dist) * 5.5f;
            velocityY = (dy / dist) * 5.5f;
        }
        else
        {
            velocityX = 5.5f;
            velocityY = 0;
        }

        frames = 1;
        frameIndex = 0;
        animTimer = 0;
        animFPS = 8.0f;
        frameW = 0;
        texH = 0;
        hasTexture = false;
    }

    void update(float dt)
    {
        if (!active)
            return;

        animTimer += dt;
        if (frames > 1 && animTimer > (1.0f / animFPS))
        {
            animTimer = 0;
            frameIndex = (frameIndex + 1) % frames;
            if (hasTexture && frameW > 0 && texH > 0)
            {
                sprite.setTextureRect(IntRect(frameIndex * frameW, 0, frameW, texH));
            }
        }

        x += velocityX;
        y += velocityY;

        if (x < -50 || x > SCREEN_WIDTH + 50 || y < -50 || y > SCREEN_HEIGHT + 50)
        {
            active = false;
        }

        sprite.setPosition(x, y);
    }

    bool collidesWith(float px, float py, int pw, int ph)
    {
        return (x < px + pw && x + 16 > px && y < py + ph && y + 16 > py);
    }
};

// ============================================================================
// PROJECTILE CLASS
// ============================================================================
class Projectile
{
public:
    float x, y;
    float velocityX, velocityY;
    int enemyType;
    bool active;
    bool isRolling;
    float lifetime;
    Sprite sprite;
    float rotation;
    float rotationSpeed;
    int rollDir;
    int dir;
    int desiredRow;
    bool movingToRow;
    int spawnRow;

    Projectile(float sx, float sy, int dir, int type)
    {
        x = sx;
        y = sy;
        enemyType = type;
        active = true;
        isRolling = false;
        lifetime = PROJECTILE_LIFETIME;
        rotation = 0;
        rotationSpeed = 15.0f;
        rollDir = 0;
        this->dir = dir;
        spawnRow = (int)((sy + getSize()) / CELL_SIZE);
        desiredRow = spawnRow;
        movingToRow = false;

        switch (dir)
        {
        case 0:
            velocityX = PROJECTILE_SPEED;
            velocityY = 0;
            isRolling = true;
            y = platformTopY(spawnRow) - getSize();
            velocityY = 0;
            velocityX = ROLL_SPEED;
            break;
        case 1:
            velocityX = 0;
            velocityY = -PROJECTILE_SPEED;
            desiredRow = std::max(0, spawnRow - 1);
            movingToRow = (desiredRow != spawnRow);
            if (!movingToRow)
            {
                isRolling = true;
                y = platformTopY(spawnRow) - getSize();
                velocityY = 0;
                velocityX = (rollDir != 0 ? (float)rollDir : ((rand() % 2 == 0) ? 1.0f : -1.0f)) * ROLL_SPEED;
            }
            break;
        case 2:
            velocityX = -PROJECTILE_SPEED;
            velocityY = 0;
            isRolling = true;
            y = platformTopY(spawnRow) - getSize();
            velocityY = 0;
            velocityX = -ROLL_SPEED;
            break;
        case 3:
            velocityX = 0;
            velocityY = PROJECTILE_SPEED;
            desiredRow = std::min(LEVEL_HEIGHT - 1, spawnRow + 1);
            movingToRow = (desiredRow != spawnRow);
            if (!movingToRow)
            {
                isRolling = true;
                y = platformTopY(spawnRow) - getSize();
                velocityY = 0;
                velocityX = (rollDir != 0 ? (float)rollDir : ((rand() % 2 == 0) ? 1.0f : -1.0f)) * ROLL_SPEED;
            }
            break;
        }
    }

    void setupSprite(Texture &tex)
    {
        sprite.setTexture(tex);
        sprite.setScale(PROJECTILE_SCALE, PROJECTILE_SCALE);
        sprite.setTextureRect(IntRect(0, 0, ENEMY_FRAME_WIDTH, ENEMY_FRAME_HEIGHT));
    }

    // Update movement, rolling, gravity, and platform interactions
    void update(float dt, char **map)
    {
        if (!active)
            return;

        lifetime -= dt;
        if (lifetime <= 0)
        {
            active = false;
            return;
        }

        // Move vertically to target row before switching to rolling
        if (movingToRow)
        {
            y += velocityY;
            int targetY = platformTopY(desiredRow) - getSize();
            bool reached = (dir == 1) ? (y <= targetY) : (y >= targetY);
            if (reached)
            {
                y = targetY;
                velocityY = 0;
                isRolling = true;
                movingToRow = false;
                if (velocityX == 0)
                    velocityX = (rollDir != 0 ? (float)rollDir : ((rand() % 2 == 0) ? 1.0f : -1.0f)) * ROLL_SPEED;
            }
        }
        // Aerial motion until first platform contact flips to rolling
        else if (!isRolling)
        {
            x += velocityX;
            y += velocityY;

            int gridY = (int)((y + getSize()) / CELL_SIZE);
            int gridX = (int)((x + getSize() / 2) / CELL_SIZE);

            if (gridY >= 0 && gridY < LEVEL_HEIGHT && gridX >= 0 && gridX < LEVEL_WIDTH)
            {
                if (map[gridY][gridX] == '#' || map[gridY][gridX] == '/' || map[gridY][gridX] == '\\' || map[gridY][gridX] == 'S')
                {
                    isRolling = true;
                    y = platformTopYAt(gridY, gridX) - getSize();
                    velocityY = 0;
                    if (velocityX == 0)
                        velocityX = (rollDir != 0 ? (float)rollDir : ((rand() % 2 == 0) ? 1.0f : -1.0f)) * ROLL_SPEED;
                }
            }
        }
        // Rolling along platforms with light gravity to settle into slopes
        else
        {
            x += velocityX;
            velocityY += GRAVITY * 0.5f;
            y += velocityY;

            int gridY = (int)((y + getSize()) / CELL_SIZE);
            int gridX = (int)((x + getSize() / 2) / CELL_SIZE);

            if (gridY >= 0 && gridY < LEVEL_HEIGHT && gridX >= 0 && gridX < LEVEL_WIDTH)
            {
                if (map[gridY][gridX] == '#' || map[gridY][gridX] == '/' || map[gridY][gridX] == '\\' || map[gridY][gridX] == 'S')
                {
                    y = platformTopYAt(gridY, gridX) - getSize();
                    velocityY = 0;
                }
            }

            // Bounce at screen edges to keep activity within bounds
            if (x < 10)
            {
                x = 10;
                velocityX = ROLL_SPEED;
            }
            if (x > SCREEN_WIDTH - getSize() - 10)
            {
                x = SCREEN_WIDTH - getSize() - 10;
                velocityX = -ROLL_SPEED;
            }
            if (y > SCREEN_HEIGHT - getSize())
            {
                y = SCREEN_HEIGHT - getSize();
                velocityY = 0;
            }
        }

        // Spin animations faster while rolling; place sprite at center of bounding box
        rotation += (isRolling ? fabs(velocityX) * 1.2f : rotationSpeed);
        sprite.setPosition(x, y);
        // strict compliance: no rotation, use pre-baked spin frames
    }

    bool collidesWith(Enemy &enemy)
    {
        if (!active || !enemy.active || enemy.captured)
            return false;
        return (x < enemy.x + enemy.getWidth() - 10 && x + getSize() > enemy.x + 10 &&
                y < enemy.y + enemy.getHeight() - 10 && y + getSize() > enemy.y + 10);
    }

    bool isAerial() { return !isRolling; }
    int getSize() { return (int)(ENEMY_FRAME_WIDTH * 0.7f); }
};

// ============================================================================
// GAME CLASS - MAIN ENGINE
// ============================================================================
class Game
{
public:
    RenderWindow window;
    GameState state;

    // Textures
    Texture playerGreenTex, playerYellowTex;
    Texture playerGreenJumpTex, playerYellowJumpTex;
    Texture greenRow1Tex, greenRow2Tex, yellowRow1Tex, yellowRow2Tex;
    Texture ghostTex, skeletonTex, invisibleTex, invisibleDimTex, chelnovTex;
    Texture bgTex, bg2Tex, platformTex;
    Texture vacuumBeamTex, starsTex;
    int vacuumFrameW = 0;
    int vacuumTotalFrames = 0;
    int superWaveFrameW = 0;
    int superWaveTotalFrames = 0;
    vector<Texture> vacuumRainbowFrames;
    Texture rollerSkatesTex, powTex;
    Texture vacuumEffectTex, flashTex, mysteryBoxTex, potionTex, impactTex, bombRedTex, bombBlueTex;
    Texture rainbowShotTex, debrisTex, superWaveTex;
    Texture powerupSpeedTex, powerupRangeTex, powerupPowerTex, powerupLifeTex;

    // UI helper textures for strict compliance (no tint/rotation)
    Texture uiPxBlack120Tex, uiPxTitleBlue230Tex, uiPxYellowTex, uiPxOutlineGray80Tex;
    Texture uiPxWhite28Tex, uiPxGreenOverlay180Tex, uiPxRedOverlay180Tex;
    Texture uiPxYellowBox200Tex, uiPxGreenBox200Tex;
    Texture beamRightTex, beamLeftTex, beamUpTex, beamDownTex;
    Texture uiSweep18Tex;
    Texture uiFontTex;
    bool useSpriteText = false;
    std::map<char, IntRect> uiGlyphs;

    Font gameFont;
    bool fontLoaded;

    Sprite bgSprite, platformSprite, vacuumBeamSprite;
    Music menuMusic, bgMusic;

    Player player;
    vector<Enemy> enemies;
    vector<Projectile> projectiles;
    vector<EnemyProjectile> enemyProjectiles;

    struct Effect
    {
        Sprite sprite;
        float timer;
        int frames;
        float fps;
        int index;
        bool active;
        float vx, vy;
        float gravity;
        float lifetime;
        std::vector<IntRect> customRects;
        const Texture *tex;
        int frameW;
        int texH;
    };
    vector<Effect> effects;

    vector<Powerup> powerups;
    ParticleSystem particles;
    ScoreManager scoreManager;

    std::array<std::array<char, LEVEL_WIDTH>, LEVEL_HEIGHT> levelMap;
    std::array<char *, LEVEL_HEIGHT> levelRows;
    int currentLevel;

    void drawSpriteText(RenderWindow &w, const std::string &text, float x, float y, float scale = 1.0f, Color color = Color::White);
    float spriteTextWidth(const std::string &text, float scale = 1.0f);
    void drawRectTex(Texture &tex, float x, float y, float w, float h);
    int selectedCharacter;
    float shootCooldown;
    float burstCooldown;
    float iframeTimer;
    float deathTimer;
    float titleAnimTimer;
    float vacuumAnimTimer;
    bool prevVacuumActive;
    float vacuumTrailTimer;
    float vacuumSuppressTimer;
    Clock gameClock;

    // Character selection animation
    float characterSelectTimer;
    float characterBounce;

    std::map<std::string, std::vector<IntRect>> effectMetaRects;
    std::map<std::string, std::map<std::string, std::pair<int, int>>> effectLabels;
    bool jumpGuiding;
    float jumpTargetX;
    bool seqActive;
    int seqStep;
    int seqOrder[4];
    int seqCounts[4];
    bool useWaveSpawning;
    int currentWave;
    float waveDelay;

    Game() : window(VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "TumblePop - Arcade Edition")
    {
        state = CHARACTER_SELECT;
        currentLevel = 1;
        selectedCharacter = 0;
        shootCooldown = 0;
        burstCooldown = 0;
        iframeTimer = 0;
        deathTimer = 0;
        titleAnimTimer = 0;
        vacuumAnimTimer = 0;
        prevVacuumActive = false;
        vacuumTrailTimer = 0;
        vacuumSuppressTimer = 0;
        fontLoaded = false;
        characterSelectTimer = 0;
        characterBounce = 0;
        jumpGuiding = false;
        jumpTargetX = 0;

        for (int i = 0; i < LEVEL_HEIGHT; i++)
        {
            for (int j = 0; j < LEVEL_WIDTH; j++)
                levelMap[i][j] = ' ';
            levelRows[i] = levelMap[i].data();
        }

        srand((unsigned)time(0));
        loadAssets();
        seqActive = false;
        seqStep = 0;
        seqOrder[0] = 2; // Invisible Man
        seqOrder[1] = 3; // Chelnov
        seqOrder[2] = 1; // Skeleton
        seqOrder[3] = 0; // Ghost
        seqCounts[0] = 3;
        seqCounts[1] = 4;
        seqCounts[2] = 9;
        seqCounts[3] = 4;
        useWaveSpawning = false;
        currentWave = 0;
        waveDelay = 0.0f;
    }

    ~Game() {}

    char **mapPtr()
    {
        for (int i = 0; i < LEVEL_HEIGHT; i++)
            levelRows[i] = levelMap[i].data();
        return levelRows.data();
    }

    void loadAssets()
    {
        cout << "\n========================================" << endl;
        cout << " Loading TumblePop Assets v7.0... " << endl;
        cout << "========================================\n"
             << endl;

        if (gameFont.loadFromFile("Data/Asset/arcade.ttf"))
        {
            fontLoaded = true;
            cout << "[OK] arcade.ttf" << endl;
        }
        else if (gameFont.loadFromFile("C:/Windows/Fonts/arial.ttf"))
        {
            fontLoaded = true;
            cout << "[OK] System font" << endl;
        }

        // Player sprite sheets - prioritize imp folder (png lowercase)
        if (greenRow1Tex.loadFromFile("Data/Asset/green_player_row_1.png"))
        {
            cout << "[OK] green_player_row_1.png (imp)" << endl;
        }
        else if (greenRow1Tex.loadFromFile("Data/Asset/green_player_row_1.PNG") ||
                 greenRow1Tex.loadFromFile("Data/Players/player_green_walk.png"))
        {
            cout << "[OK] player_green_walk.png" << endl;
        }

        if (greenRow2Tex.loadFromFile("Data/Asset/green_player_row_2.png"))
        {
            cout << "[OK] green_player_row_2.png (imp)" << endl;
        }

        if (yellowRow1Tex.loadFromFile("Data/Asset/yellow_player_row_1.png"))
        {
            cout << "[OK] yellow_player_row_1.png (imp)" << endl;
        }
        else if (yellowRow1Tex.loadFromFile("Data/Asset/yellow_player_row_1.PNG") ||
                 yellowRow1Tex.loadFromFile("Data/Players/player_yellow_walk.png"))
        {
            cout << "[OK] player_yellow_walk.png" << endl;
        }

        if (yellowRow2Tex.loadFromFile("Data/Asset/yellow_player_row_2.png"))
        {
            cout << "[OK] yellow_player_row_2.png (imp)" << endl;
        }

        // Enemies - prioritize imp folder
        if (ghostTex.loadFromFile("Data/Asset/ghost.png"))
        {
            cout << "[OK] ghost.png (imp)" << endl;
        }
        else if (ghostTex.loadFromFile("Data/Enemies/ghost.png"))
        {
            cout << "[OK] ghost.png" << endl;
        }

        if (skeletonTex.loadFromFile("Data/Asset/skeleton.png"))
        {
            cout << "[OK] skeleton.png (imp)" << endl;
        }
        else if (skeletonTex.loadFromFile("Data/Enemies/skeleton.png"))
        {
            cout << "[OK] skeleton.png" << endl;
        }

        if (invisibleTex.loadFromFile("Data/Asset/invisible.png"))
        {
            cout << "[OK] invisible.png (imp)" << endl;
        }
        else if (invisibleTex.loadFromFile("Data/Enemies/invisible_man.png"))
        {
            cout << "[OK] invisible_man.png" << endl;
        }
        if (invisibleDimTex.loadFromFile("Data/Asset/invisible_dim.png"))
        {
            cout << "[OK] invisible_dim.png (Asset dim)" << endl;
        }

        if (chelnovTex.loadFromFile("Data/Asset/chelnov.png"))
        {
            cout << "[OK] chelnov.png (imp)" << endl;
        }
        else if (chelnovTex.loadFromFile("Data/Enemies/chelnov.png"))
        {
            cout << "[OK] chelnov.png" << endl;
        }

        if (vacuumBeamTex.loadFromFile("Data/Asset/8.png"))
        {
            cout << "[OK] Vacuum Beam (8.png) loaded" << endl;
            // Analyze frames
            int h = vacuumBeamTex.getSize().y;
            if (h > 0)
            {
                vacuumTotalFrames = vacuumBeamTex.getSize().x / h;
                vacuumFrameW = h;
                if (vacuumTotalFrames < 1)
                    vacuumTotalFrames = 1;
                // If the texture is very wide (strip), this calculation is correct.
                // Assuming square frames.
            }
            vacuumBeamSprite.setTexture(vacuumBeamTex);
        }
        else if (vacuumBeamTex.loadFromFile("Data/Asset/vacuum_beam.png"))
        {
            cout << "[OK] vacuum_beam.png" << endl;
            vacuumBeamSprite.setTexture(vacuumBeamTex);
        }

        // Load effect textures from imp folder
        rollerSkatesTex.loadFromFile("Data/Asset/0.png");
        powTex.loadFromFile("Data/Asset/1.png");
        vacuumEffectTex.loadFromFile("Data/Asset/2.png");
        flashTex.loadFromFile("Data/Asset/3.png");
        mysteryBoxTex.loadFromFile("Data/Asset/4.png");
        potionTex.loadFromFile("Data/Asset/5.png");
        impactTex.loadFromFile("Data/Asset/6.png");
        bombRedTex.loadFromFile("Data/Asset/7.png");
        bombBlueTex.loadFromFile("Data/Asset/10.png");
        rainbowShotTex.loadFromFile("Data/Asset/8.png");
        debrisTex.loadFromFile("Data/Asset/9.png");
        superWaveTex.loadFromFile("Data/Asset/11.png");
        if (superWaveTex.getSize().y > 0)
        {
            int h = superWaveTex.getSize().y;
            superWaveTotalFrames = superWaveTex.getSize().x / h;
            superWaveFrameW = h;
            if (superWaveTotalFrames < 1)
                superWaveTotalFrames = 1;
            cout << "[OK] Super Wave (11.png) frames initialized: " << superWaveTotalFrames << endl;
        }

        auto buildRects = [&](Texture &tex)
        {
            std::vector<IntRect> out;
            if (tex.getSize().x == 0 || tex.getSize().y == 0)
                return out;
            int w = (int)tex.getSize().x;
            int h = (int)tex.getSize().y;
            int fw = ENEMY_FRAME_WIDTH;
            int frames = std::max(1, w / fw);
            out.reserve(frames);
            for (int i = 0; i < frames; ++i)
                out.push_back(IntRect(i * fw, 0, fw, h));
            return out;
        };

        effectMetaRects["0"] = buildRects(rollerSkatesTex);
        effectMetaRects["1"] = buildRects(powTex);
        effectMetaRects["2"] = buildRects(vacuumEffectTex);
        effectMetaRects["3"] = buildRects(flashTex);
        effectMetaRects["4"] = buildRects(mysteryBoxTex);
        effectMetaRects["5"] = buildRects(potionTex);
        effectMetaRects["6"] = buildRects(impactTex);
        effectMetaRects["7"] = buildRects(bombRedTex);
        effectMetaRects["8"] = buildRects(rainbowShotTex);
        effectMetaRects["9"] = buildRects(debrisTex);
        effectMetaRects["10"] = buildRects(bombBlueTex);
        effectMetaRects["11"] = buildRects(superWaveTex);

        std::vector<IntRect> r2 = effectMetaRects["2"];
        if (!r2.empty())
        {
            int n = (int)r2.size();
            int a = std::max(0, n - 4);
            int b = n - 1;
            effectLabels["2"]["disappear"] = std::make_pair(a, b);
        }

        if (starsTex.loadFromFile("Data/Asset/8.png"))
        {
            cout << "[OK] stars.png" << endl;
        }

        powerupSpeedTex.loadFromFile("Data/Asset/0.png");
        powerupRangeTex.loadFromFile("Data/Asset/2.png");
        powerupPowerTex.loadFromFile("Data/Asset/5.png");
        powerupLifeTex.loadFromFile("Data/Asset/3.png");

        // Optional UI bitmap font atlas (sprite-based text)
        if (uiFontTex.loadFromFile("Data/Asset/ui_font.png"))
        {
            useSpriteText = true;
            // Example glyph mapping: assumes monospace cells 16x24 arranged in rows
            int gw = 16, gh = 24;
            auto put = [&](char c, int col, int row)
            { uiGlyphs[c] = IntRect(col * gw, row * gh, gw, gh); };
            for (int d = 0; d <= 9; ++d)
                put('0' + d, d, 0);
            const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            for (int i = 0; upper[i]; ++i)
                put(upper[i], i % 16, 1 + i / 16);
            const char *lower = "abcdefghijklmnopqrstuvwxyz";
            for (int i = 0; lower[i]; ++i)
                put(lower[i], i % 16, 4 + i / 16);
            put(':', 10, 3);
            put('[', 11, 3);
            put(']', 12, 3);
            put('x', 13, 3);
            put('/', 14, 3);
            put(' ', 15, 3);
        }

        // UI color pixels
        uiPxBlack120Tex.loadFromFile("Data/Asset/color/ui_px_black_120.png");
        uiPxTitleBlue230Tex.loadFromFile("Data/Asset/color/ui_px_title_blue_230.png");
        uiPxYellowTex.loadFromFile("Data/Asset/color/ui_px_yellow.png");
        uiPxOutlineGray80Tex.loadFromFile("Data/Asset/color/ui_px_outline_gray_80.png");
        uiPxWhite28Tex.loadFromFile("Data/Asset/color/ui_px_white_28.png");
        uiPxGreenOverlay180Tex.loadFromFile("Data/Asset/color/ui_px_green_overlay_180.png");
        uiPxRedOverlay180Tex.loadFromFile("Data/Asset/color/ui_px_red_overlay_180.png");
        uiPxYellowBox200Tex.loadFromFile("Data/Asset/color/ui_px_yellow_box_200.png");
        uiPxGreenBox200Tex.loadFromFile("Data/Asset/color/ui_px_green_box_200.png");

        // Beam directional sheets
        beamRightTex.loadFromFile("Data/Asset/beam/beam_right.png");
        beamLeftTex.loadFromFile("Data/Asset/beam/beam_left.png");
        beamUpTex.loadFromFile("Data/Asset/beam/beam_up.png");
        beamDownTex.loadFromFile("Data/Asset/beam/beam_down.png");
        // Pre-rotated sweep
        uiSweep18Tex.loadFromFile("Data/Asset/ui_sweep_18.png");

        if (bgTex.loadFromFile("Data/Asset/bg.png"))
        {
            cout << "[OK] bg.png (imp level1)" << endl;
            bgSprite.setTexture(bgTex);
            bgSprite.setScale((float)SCREEN_WIDTH / bgTex.getSize().x,
                              (float)SCREEN_HEIGHT / bgTex.getSize().y);
        }

        bg2Tex.loadFromFile("Data/Asset/bg_level2.png");

        if (platformTex.loadFromFile("Data/Asset/block1.png"))
        {
            cout << "[OK] block1.png (imp)" << endl;
            platformSprite.setTexture(platformTex);
            platformSprite.setScale((float)CELL_SIZE / platformTex.getSize().x,
                                    (float)CELL_SIZE / platformTex.getSize().y);
        }

        // Disable smoothing for pixel art

        bool musicLoaded = false;
        if (menuMusic.openFromFile("Data/Asset/mus.ogg"))
        {
            musicLoaded = true;
        }
        else if (menuMusic.openFromFile("Data/Asset/music_level2.ogg"))
        {
            musicLoaded = true;
        }

        if (musicLoaded)
        {
            menuMusic.setVolume(50);
            menuMusic.setLoop(true);
            menuMusic.play();
            cout << "[OK] Menu music playing" << endl;
        }

        bgMusic.openFromFile("Data/Asset/mus.ogg");
        bgMusic.setVolume(40);
        bgMusic.setLoop(true);

        // Initialize player with proper sprite sheets
        player.setupSprite(yellowRow1Tex);
        player.bindSheets(&yellowRow1Tex, &yellowRow2Tex);
        player.loadMetaLabels("yellow");

        cout << "\n[DONE] Asset loading complete!\n"
             << endl;
    }

    void run()
    {
        printControls();

        while (window.isOpen())
        {
            float dt = gameClock.restart().asSeconds();
            if (dt > 0.1f)
                dt = 0.1f;

            handleInput();
            update(dt);
            render();
        }
    }

    void printControls()
    {
        cout << "\n============================================" << endl;
        cout << " TUMBLEPOP - MASTERCLASS EDITION v8.0 " << endl;
        cout << " Portfolio-Ready Professional Build " << endl;
        cout << "============================================\n"
             << endl;
        cout << "CONTROLS:" << endl;
        cout << " [1/2] Select Character" << endl;
        cout << " [ENTER] Start Game" << endl;
        cout << " [LEFT/RIGHT] Move" << endl;
        cout << " [UP] Jump" << endl;
        cout << " [W/A/S/D] Aim Vacuum" << endl;
        cout << " [SPACE] Hold to Vacuum" << endl;
        cout << " [Z] Shoot Single Enemy" << endl;
        cout << " [X] Vacuum Burst (All)" << endl;
        cout << " [R] Restart [ESC] Quit" << endl;
        cout << "\nENHANCED FEATURES:" << endl;
        cout << " - Larger character sprites with breathing animations" << endl;
        cout << " - Multi-layered pulsing vacuum beam effects" << endl;
        cout << " - Animated HUD with score pop effects" << endl;
        cout << " - Professional polish throughout" << endl;
        cout << "============================================\n"
             << endl;
    }

private:
    void handleInput()
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
        }

        if (Keyboard::isKeyPressed(Keyboard::Escape))
            window.close();
    }

    void update(float dt)
    {
        titleAnimTimer += dt;
        vacuumAnimTimer += dt;
        if (deathTimer > 0)
        {
            deathTimer -= dt;
            if (deathTimer <= 0 && player.health <= 0)
            {
                state = GAME_OVER;
                bgMusic.stop();
            }
        }
        characterSelectTimer += dt;
        characterBounce = sin(characterSelectTimer * 3.0f) * 8.0f;

        if (shootCooldown > 0)
            shootCooldown -= dt;
        if (burstCooldown > 0)
            burstCooldown -= dt;
        if (iframeTimer > 0)
            iframeTimer -= dt;

        particles.update(dt);

        switch (state)
        {
        case CHARACTER_SELECT:
            updateCharacterSelect();
            break;
        case LEVEL_1:
        case LEVEL_2:
            updateGameplay(dt);
            break;
        case GAME_OVER:
        case VICTORY:
            updateGameOver();
            break;
        }
    }

    void updateCharacterSelect()
    {
        static bool key1Pressed = false, key2Pressed = false, enterPressed = false;
        bool key1Now = Keyboard::isKeyPressed(Keyboard::Num1);
        bool key2Now = Keyboard::isKeyPressed(Keyboard::Num2);
        bool enterNow = Keyboard::isKeyPressed(Keyboard::Enter);

        if (key1Now && !key1Pressed)
        {
            selectedCharacter = 0;
            player.setCharacter(true);
            player.setupSprite(yellowRow1Tex);
            player.bindSheets(&yellowRow1Tex, &yellowRow2Tex);
            player.loadMetaLabels("yellow");
            cout << "[SELECT] YELLOW Character!" << endl;

            // Particle effect
            particles.emit(Vector2f(SCREEN_WIDTH / 2 - 200, 400), Color::Yellow, 20);
        }
        if (key2Now && !key2Pressed)
        {
            selectedCharacter = 1;
            player.setCharacter(false);
            player.setupSprite(greenRow1Tex);
            player.bindSheets(&greenRow1Tex, &greenRow2Tex);
            player.loadMetaLabels("green");
            cout << "[SELECT] GREEN Character!" << endl;

            // Particle effect
            particles.emit(Vector2f(SCREEN_WIDTH / 2 + 200, 400), Color::Green, 20);
        }
        if (enterNow && !enterPressed)
        {
            menuMusic.stop();
            startLevel(1);
        }

        key1Pressed = key1Now;
        key2Pressed = key2Now;
        enterPressed = enterNow;
    }

    void startLevel(int level)
    {
        currentLevel = level;
        state = (level == 1) ? LEVEL_1 : LEVEL_2;

        player.reset(level);
        enemies.clear();
        projectiles.clear();
        enemyProjectiles.clear();
        powerups.clear();
        scoreManager.resetLevel();

        if (selectedCharacter == 1)
            scoreManager.addCharacterBonus("Speed Demon");
        else
            scoreManager.addCharacterBonus("Max Capacity");

        buildLevel(level);
        spawnEnemies(level);
        spawnPowerups(level);

        bgMusic.stop();
        if (level == 1)
        {
            bgMusic.openFromFile("Data/Asset/mus.ogg");
        }
        else
        {
            bgMusic.openFromFile("Data/Asset/music_level2.ogg");
        }
        bgMusic.play();

        cout << "\n========== LEVEL " << level << " START! ==========\n"
             << endl;
    }

    // Builds the level layout grid and visual background for the given level
    void buildLevel(int level)
    {
        // Clear map and slope offsets before populating tiles
        for (int i = 0; i < LEVEL_HEIGHT; i++)
        {
            for (int j = 0; j < LEVEL_WIDTH; j++)
            {
                levelMap[i][j] = ' ';
                slopeOffset[i][j] = 0;
            }
        }

        if (level == 1)
        {
            // Ground (row 13)
            for (int j = 0; j < LEVEL_WIDTH; j++)
                levelMap[13][j] = '#';

            // Lower the next tiers to ease jumps and add a small center step
            // Row 11 platforms (was 10)
            for (int j = 2; j < 8; j++)
                levelMap[11][j] = '#';
            for (int j = 10; j < 16; j++)
                levelMap[11][j] = '#';

            // Row 8 platforms (was 7) with a center helper
            for (int j = 1; j < 6; j++)
                levelMap[8][j] = '#';
            for (int j = 12; j < 17; j++)
                levelMap[8][j] = '#';
            for (int j = 8; j < 11; j++)
                levelMap[8][j] = '#';

            // Row 5 platform (was 4)
            for (int j = 5; j < 13; j++)
                levelMap[5][j] = '#';

            bgSprite.setTexture(bgTex);
            bgSprite.setScale((float)SCREEN_WIDTH / bgTex.getSize().x,
                              (float)SCREEN_HEIGHT / bgTex.getSize().y);
        }
        else
        {
            // Straight platforms with clean, equally spaced layers (vertical gap >= 90px)
            for (int j = 0; j < LEVEL_WIDTH; j++)
                levelMap[13][j] = '#';

            for (int j = 0; j < 9; j++)
                levelMap[11][j] = '#';
            for (int j = 12; j < 15; j++)
                levelMap[11][j] = '#';

            for (int j = 3; j < 9; j++)
                levelMap[8][j] = '#';
            for (int j = 11; j < 18; j++)
                levelMap[8][j] = '#';

            for (int j = 5; j < 12; j++)
                levelMap[5][j] = '#';

            for (int j = 1; j < 4; j++)
                levelMap[2][j] = '#';
            for (int j = 14; j < 17; j++)
                levelMap[2][j] = '#';

            // Slanted ramps: attach only to left/right edges, allowed angles ±30/±45
            auto attachRamp = [&](int row, int edgeCol, bool fromRightEdge, int tiles, int degrees)
            {
                int step = (degrees == 45) ? CELL_SIZE : (int)std::round(CELL_SIZE * 0.577f);
                int startCol = fromRightEdge ? edgeCol + 1 : edgeCol - tiles;
                int endCol = fromRightEdge ? edgeCol + tiles : edgeCol - 1;
                bool downRight = fromRightEdge;
                for (int j = startCol; j <= endCol; ++j)
                {
                    int k = j - startCol;
                    int delta = (downRight ? +1 : -1) * (k * step);
                    levelMap[row][j] = downRight ? '/' : '\\';
                    slopeOffset[row][j] = delta;
                }
                // Trim the block adjoining the ramp so the surface remains continuous
                if (!fromRightEdge && startCol - 1 >= 0 && levelMap[row][startCol - 1] == '#')
                    levelMap[row][startCol - 1] = ' ';
                if (fromRightEdge && endCol + 1 < LEVEL_WIDTH && levelMap[row][endCol + 1] == '#')
                    levelMap[row][endCol + 1] = ' ';
            };

            auto collectSpans = [&](int r)
            {
                std::vector<std::pair<int, int>> spans;
                int c = 0;
                while (c < LEVEL_WIDTH)
                {
                    while (c < LEVEL_WIDTH && levelMap[r][c] != '#')
                        ++c;
                    int a = c;
                    while (c < LEVEL_WIDTH && levelMap[r][c] == '#')
                        ++c;
                    int b = c - 1;
                    if (a <= b)
                        spans.emplace_back(a, b);
                }
                return spans;
            };

            auto canPlaceRamp = [&](int row, int edgeCol, bool fromRightEdge, int tiles, int step)
            {
                int startCol = fromRightEdge ? edgeCol + 1 : edgeCol - tiles;
                int endCol = fromRightEdge ? edgeCol + tiles : edgeCol - 1;
                if (startCol < 0 || endCol >= LEVEL_WIDTH)
                    return false;
                for (int j = startCol; j <= endCol; ++j)
                {
                    if (levelMap[row][j] != ' ')
                        return false;
                }
                return true;
            };

            auto placeRandomRamps = [&]()
            {
                int rows[] = {2, 5, 8, 11};
                int target = 1 + (rand() % 4);
                int attempts = 0;
                int placed = 0;
                while (placed < target && attempts < 100)
                {
                    attempts++;
                    int r = rows[rand() % 4];
                    auto spans = collectSpans(r);
                    if (spans.empty())
                        continue;
                    auto span = spans[rand() % spans.size()];
                    bool fromRight = (rand() % 2) == 0;
                    int edgeCol = fromRight ? span.second : span.first;
                    int degOpt[2] = {45, 30};
                    int degrees = degOpt[rand() % 2];
                    int step = (degrees == 45) ? CELL_SIZE : (int)std::round(CELL_SIZE * 0.577f);
                    int tiles = 3 + (rand() % 3);
                    // Ensure ramp area is empty before placing slants
                    if (!canPlaceRamp(r, edgeCol, fromRight, tiles, step))
                        continue;
                    attachRamp(r, edgeCol, fromRight, tiles, degrees);
                    placed++;
                }
            };

            placeRandomRamps();

            if (bg2Tex.getSize().x > 0)
            {
                bgSprite.setTexture(bg2Tex);
                bgSprite.setScale((float)SCREEN_WIDTH / bg2Tex.getSize().x,
                                  (float)SCREEN_HEIGHT / bg2Tex.getSize().y);
            }
        }
    }

    // Spawns enemies for the level and initializes their sprites and placement
    void spawnEnemies(int level)
    {
        cout << "Spawning Level " << level << " enemies...\n"
             << endl;
        enemies.clear();

        auto addEnemiesOnSegment = [&](int row, int cStart, int cEnd, int count, const vector<int> &types)
        {
            int widthTiles = cEnd - cStart + 1;
            float baseY = row * CELL_SIZE - ENEMY_FRAME_HEIGHT * ENEMY_SCALE - 1;
            int minSpacing = 96;

            for (int i = 0; i < count; i++)
            {
                float t = (i + 1) / (float)(count + 1);
                int tileCol = cStart + (int)round(t * (widthTiles - 1));
                tileCol = max(cStart, min(tileCol, cEnd));

                int attempts = 0;
                float xPix = tileCol * CELL_SIZE + 8.0f;

                auto isTooClose = [&](float xCandidate)
                {
                    for (size_t k = 0; k < enemies.size(); k++)
                    {
                        if (fabs(enemies[k].y - baseY) < 8.0f)
                        {
                            if (fabs(enemies[k].x - xCandidate) < minSpacing)
                                return true;
                        }
                    }
                    return false;
                };

                while (attempts < widthTiles && isTooClose(xPix))
                {
                    int offset = (rand() % 3) - 1;
                    tileCol = max(cStart, min(tileCol + offset, cEnd));
                    xPix = tileCol * CELL_SIZE + 8.0f;
                    attempts++;
                }

                int type = types[i % (int)types.size()];
                enemies.push_back(Enemy(type, xPix, baseY));
            }
        };

        if (level == 1)
        {
            useWaveSpawning = false;
            addEnemiesOnSegment(5, 5, 13, 3, {0});
            addEnemiesOnSegment(8, 1, 6, 2, {0});
            addEnemiesOnSegment(8, 12, 16, 2, {0});
            addEnemiesOnSegment(11, 2, 7, 1, {0});
            addEnemiesOnSegment(13, 1, 16, 2, {1});
            addEnemiesOnSegment(11, 10, 15, 2, {1});
        }
        else
        {
            useWaveSpawning = true;
            currentWave = 0;
            waveDelay = 0.0f;
            cout << "[WAVE 1] Spawning 3 Invisible Men..." << endl;
            addEnemiesOnSegment(8, 4, 8, 2, {2});
            addEnemiesOnSegment(12, 12, 16, 1, {2});
        }

        // Setup sprites for all enemies
        for (size_t i = 0; i < enemies.size(); i++)
        {
            Texture *tex = nullptr;
            switch (enemies[i].type)
            {
            case 0:
                tex = &ghostTex;
                break;
            case 1:
                tex = &skeletonTex;
                break;
            case 2:
                tex = &invisibleTex;
                break;
            case 3:
                tex = &chelnovTex;
                break;
            }
            if (tex && tex->getSize().x > 0)
                enemies[i].setupSprite(*tex, (enemies[i].type == 2 ? &invisibleDimTex : nullptr));
            enemies[i].checkPlatformCollision(mapPtr());
            enemies[i].velocityY = 0;
            enemies[i].onGround = true;
        }

        cout << "[SPAWNED] " << enemies.size() << " enemies for Level " << level << endl;
    }

    // Spawns a wave of enemies of a specific type and count
    void spawnWave(int type, int count)
    {
        cout << "[WAVE] Spawning type " << type << " x" << count << endl;
        vector<tuple<int, int, int>> segments;
        segments.push_back({2, 1, 3});
        segments.push_back({2, 14, 16});
        segments.push_back({5, 5, 12});
        segments.push_back({8, 2, 7});
        segments.push_back({8, 10, 15});
        segments.push_back({11, 3, 8});
        segments.push_back({11, 9, 14});
        segments.push_back({13, 1, 16});

        int placed = 0;
        int segIndex = 0;
        while (placed < count && !segments.empty())
        {
            std::tuple<int, int, int> seg = segments[segIndex % segments.size()];
            segIndex++;
            int row = std::get<0>(seg);
            int cStart = std::get<1>(seg);
            int cEnd = std::get<2>(seg);
            int widthTiles = cEnd - cStart + 1;
            float baseY = row * CELL_SIZE - ENEMY_FRAME_HEIGHT * ENEMY_SCALE - 1;
            int minSpacing = 96;

            float t = (float)((rand() % (widthTiles - 1)) + 1) / (float)(widthTiles);
            int tileCol = cStart + (int)round(t * (widthTiles - 1));
            tileCol = max(cStart, min(tileCol, cEnd));
            float xPix = tileCol * CELL_SIZE + 8.0f;

            auto isTooClose = [&](float xCandidate)
            {
                for (size_t k = 0; k < enemies.size(); k++)
                {
                    if (fabs(enemies[k].y - baseY) < 8.0f)
                    {
                        if (fabs(enemies[k].x - xCandidate) < minSpacing)
                            return true;
                    }
                }
                return false;
            };

            int attempts = 0;
            while (attempts < widthTiles && isTooClose(xPix))
            {
                int offset = (rand() % 3) - 1;
                tileCol = max(cStart, min(tileCol + offset, cEnd));
                xPix = tileCol * CELL_SIZE + 8.0f;
                attempts++;
            }

            Enemy e(type, xPix, baseY);
            Texture *tex = nullptr;
            switch (type)
            {
            case 0:
                tex = &ghostTex;
                break;
            case 1:
                tex = &skeletonTex;
                break;
            case 2:
                tex = &invisibleTex;
                break;
            case 3:
                tex = &chelnovTex;
                break;
            }
            if (tex && tex->getSize().x > 0)
                e.setupSprite(*tex, (type == 2 ? &invisibleDimTex : nullptr));
            e.checkPlatformCollision(mapPtr());
            e.velocityY = 0;
            e.onGround = true;
            enemies.push_back(e);
            placed++;
        }
        cout << "[WAVE] Spawned " << placed << " enemies" << endl;
    }

    void spawnNextWave()
    {
        if (!useWaveSpawning)
            return;
        if (currentWave >= 3)
            return;
        currentWave++;
        if (currentWave == 1)
        {
            cout << "[WAVE 2] Spawning 4 Chelnov..." << endl;
            spawnWave(3, 4);
        }
        else if (currentWave == 2)
        {
            cout << "[WAVE 3] Spawning 9 Skeletons..." << endl;
            spawnWave(1, 9);
        }
        else if (currentWave == 3)
        {
            cout << "[WAVE 4] Spawning 4 Ghosts (Final Wave)..." << endl;
            spawnWave(0, 4);
        }
        waveDelay = 0.0f;
    }

    void spawnPowerups(int level)
    {
        powerups.clear();

        if (level == 1)
        {
            Powerup p1(POWERUP_SPEED, 500, 4 * CELL_SIZE - 16);
            if (rollerSkatesTex.getSize().x > 0)
                p1.setupSprite(rollerSkatesTex);
            else if (powerupSpeedTex.getSize().x > 0)
                p1.setupSprite(powerupSpeedTex);
            if (!effectMetaRects["0"].empty())
                p1.applyMetaRects(effectMetaRects["0"]);
            powerups.push_back(p1);

            Powerup p2(POWERUP_LIFE, 300, 7 * CELL_SIZE - 16);
            if (flashTex.getSize().x > 0)
                p2.setupSprite(flashTex);
            else if (powerupLifeTex.getSize().x > 0)
                p2.setupSprite(powerupLifeTex);
            if (!effectMetaRects["3"].empty())
                p2.applyMetaRects(effectMetaRects["3"]);
            powerups.push_back(p2);
        }
        else
        {
            Powerup p1(POWERUP_SPEED, 400, 5 * CELL_SIZE - 16);
            if (rollerSkatesTex.getSize().x > 0)
                p1.setupSprite(rollerSkatesTex);
            else if (powerupSpeedTex.getSize().x > 0)
                p1.setupSprite(powerupSpeedTex);
            if (!effectMetaRects["0"].empty())
                p1.applyMetaRects(effectMetaRects["0"]);
            powerups.push_back(p1);

            Powerup p2(POWERUP_RANGE, 600, 8 * CELL_SIZE - 16);
            if (vacuumEffectTex.getSize().x > 0)
                p2.setupSprite(vacuumEffectTex);
            else if (powerupRangeTex.getSize().x > 0)
                p2.setupSprite(powerupRangeTex);
            if (!effectMetaRects["2"].empty())
                p2.applyMetaRects(effectMetaRects["2"]);
            powerups.push_back(p2);

            Powerup p3(POWERUP_POWER, 350, 11 * CELL_SIZE - 16);
            if (powTex.getSize().x > 0)
                p3.setupSprite(powTex);
            else if (powerupPowerTex.getSize().x > 0)
                p3.setupSprite(powerupPowerTex);
            if (!effectMetaRects["1"].empty())
                p3.applyMetaRects(effectMetaRects["1"]);
            powerups.push_back(p3);

            Powerup p4(POWERUP_LIFE, 200, 2 * CELL_SIZE - 16);
            if (flashTex.getSize().x > 0)
                p4.setupSprite(flashTex);
            else if (powerupLifeTex.getSize().x > 0)
                p4.setupSprite(powerupLifeTex);
            if (!effectMetaRects["3"].empty())
                p4.applyMetaRects(effectMetaRects["3"]);
            powerups.push_back(p4);
        }
    }

    void updateGameplay(float dt)
    {
        scoreManager.update(dt);

        // Restart
        static bool rPressed = false;
        bool rNow = Keyboard::isKeyPressed(Keyboard::R);
        if (rNow && !rPressed)
        {
            startLevel(currentLevel);
            rPressed = rNow;
            return;
        }
        rPressed = rNow;

        // Movement
        player.velocityX = 0;
        if (Keyboard::isKeyPressed(Keyboard::Left))
        {
            player.velocityX = -player.speed;
            player.facingRight = false;
        }
        if (Keyboard::isKeyPressed(Keyboard::Right))
        {
            player.velocityX = player.speed;
            player.facingRight = true;
        }
        if (!Keyboard::isKeyPressed(Keyboard::Left) && !Keyboard::isKeyPressed(Keyboard::Right) && player.jumpBoostTimer > 0 && !player.onGround)
        {
            player.velocityX = player.facingRight ? player.speed : -player.speed;
        }

        // Jump
        static bool upPressed = false;
        bool upNow = Keyboard::isKeyPressed(Keyboard::Up);
        if (upNow && !upPressed && player.onGround)
        {
            player.velocityY = JUMP_STRENGTH;
            player.onGround = false;

            // User requested: "move him little forward in that direction"
            bool leftNow = Keyboard::isKeyPressed(Keyboard::Left);
            bool rightNow = Keyboard::isKeyPressed(Keyboard::Right);

            float forwardBoost = 18.0f;
            if (rightNow || (player.facingRight && !leftNow))
            {
                player.velocityX = player.speed;
                player.x += forwardBoost; // Nudge forward
            }
            else if (leftNow || (!player.facingRight && !rightNow))
            {
                player.velocityX = -player.speed;
                player.x -= forwardBoost; // Nudge forward
            }
            player.jumpBoostTimer = 0.35f;

            // Jump guiding removed for manual control
            jumpGuiding = false;
            if (player.x < 0)
                player.x = 0;
            if (player.x > SCREEN_WIDTH - player.getWidth())
                player.x = SCREEN_WIDTH - player.getWidth();
            particles.emit(Vector2f(player.getCenterX(), player.y + player.getHeight()),
                           Color(200, 200, 200), 8);
        }
        upPressed = upNow;
        if (player.jumpBoostTimer > 0)
        {
            player.jumpBoostTimer -= dt;
            if (player.jumpBoostTimer < 0) player.jumpBoostTimer = 0;
        }

        // Crouch
        if (Keyboard::isKeyPressed(Keyboard::Down) && player.onGround)
        {
            player.velocityX = 0;
            player.animState = P_CROUCH;
            player.animStateTimer = 0;
        }

        // Vacuum direction (WASD): 0=Right, 1=Up, 2=Left, 3=Down
        bool dirRight = Keyboard::isKeyPressed(Keyboard::D);
        bool dirUp = Keyboard::isKeyPressed(Keyboard::W);
        bool dirLeft = Keyboard::isKeyPressed(Keyboard::A);
        bool dirDown = Keyboard::isKeyPressed(Keyboard::S);
        if (dirRight)
            player.vacuumDirection = 0;
        else if (dirUp)
            player.vacuumDirection = 1;
        else if (dirLeft)
            player.vacuumDirection = 2;
        else if (dirDown)
            player.vacuumDirection = 3;

        if (!player.vacuumActive && (dirRight || dirUp || dirLeft || dirDown))
        {
            // Brief trail when aim keys were pressed without active vacuum
            vacuumTrailTimer = 0.20f;
            vacuumSuppressTimer = 0.0f;
        }

        // Vacuum activation
        player.vacuumActive = Keyboard::isKeyPressed(Keyboard::Space);
        player.shootHold = player.vacuumActive;

        // Update Vacuum Animation State Machine
        if (player.vacuumActive)
        {
            if (player.vacuumAnimState == 0 || player.vacuumAnimState == 3)
            {                               // OFF or SHRINK
                player.vacuumAnimState = 1; // GROW
            }
        }
        else
        {
            if (player.vacuumAnimState != 0 && player.vacuumAnimState != 3)
            {
                player.vacuumAnimState = 3; // SHRINK
            }
        }

        // Update Frame Counter
        if (vacuumTotalFrames > 0)
        {
            float vacMax = (float)vacuumTotalFrames;
            float growEnd = vacMax - 1;
            float sustainStart = vacMax * 0.6f;
            if (sustainStart < 0)
                sustainStart = 0;

            if (player.vacuumAnimState == 1)
            { // GROW
                player.vacuumAnimFrame += dt * player.vacuumAnimSpeed;
                if (player.vacuumAnimFrame >= growEnd)
                {
                    player.vacuumAnimState = 2; // SUSTAIN
                    player.vacuumAnimFrame = growEnd;
                }
            }
            else if (player.vacuumAnimState == 2)
            { // SUSTAIN
                player.vacuumAnimFrame += dt * player.vacuumAnimSpeed;
                // Loop between sustainStart and vacMax
                if (player.vacuumAnimFrame >= vacMax)
                {
                    player.vacuumAnimFrame = sustainStart;
                }
            }
            else if (player.vacuumAnimState == 3)
            { // SHRINK
                player.vacuumAnimFrame -= dt * player.vacuumAnimSpeed * 1.5f;
                if (player.vacuumAnimFrame <= 0)
                {
                    player.vacuumAnimFrame = 0;
                    player.vacuumAnimState = 0; // OFF
                }
            }
        }

        if (player.vacuumActive && !dirRight && !dirUp && !dirLeft && !dirDown)
        {
            player.vacuumDirection = player.facingRight ? 0 : 2;
        }
        if (player.vacuumActive && !prevVacuumActive)
            vacuumAnimTimer = 0;
        if (!player.vacuumActive && prevVacuumActive)
            vacuumTrailTimer = 0.25f;
        prevVacuumActive = player.vacuumActive;

        if (player.vacuumActive && player.onGround)
        {
            player.animState = P_SHOOT;
            player.animStateTimer = 0;
        }

        // Single shot (Z) - spawn one projectile from captured enemies
        static bool zPressed = false;
        bool zNow = Keyboard::isKeyPressed(Keyboard::Z);
        if (zNow && !zPressed && shootCooldown <= 0)
        {
            int type = player.releaseOneEnemy();
            if (type >= 0)
            {
                int dirNow = player.vacuumDirection;
                bool aimRight = Keyboard::isKeyPressed(Keyboard::D);
                bool aimUp = Keyboard::isKeyPressed(Keyboard::W);
                bool aimLeft = Keyboard::isKeyPressed(Keyboard::A);
                bool aimDown = Keyboard::isKeyPressed(Keyboard::S);
                if (!aimRight && !aimUp && !aimLeft && !aimDown)
                    // Default to facing horizontal if no aim key is held
                    dirNow = player.facingRight ? 0 : 2;

                float cx = player.getCenterX();
                float cy = player.getCenterY();
                int footRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
                if (footRow == 11)
                    cy -= 10.0f;

                // Offset projectile spawn to the player's hand position
                float handOffsetX = (player.facingRight ? +player.getFrameWidth() * PLAYER_SCALE * 0.62f
                                                        : -player.getFrameWidth() * PLAYER_SCALE * 0.62f);
                float handOffsetY = 0;
                if (dirNow == 1)
                    handOffsetY = -player.getFrameHeight() * PLAYER_SCALE * 0.10f;
                else if (dirNow == 3)
                    handOffsetY = player.getFrameHeight() * PLAYER_SCALE * 0.20f;
                else
                    handOffsetY = player.getFrameHeight() * PLAYER_SCALE * 0.26f;

                float px = cx + handOffsetX;
                float py = cy + handOffsetY;

                float ax = 0, ay = 0;
                switch (dirNow)
                {
                case 0:
                    ax = 1;
                    ay = 0;
                    break;
                case 1:
                    ax = 0;
                    ay = -1;
                    break;
                case 2:
                    ax = -1;
                    ay = 0;
                    break;
                case 3:
                    ax = 0;
                    ay = 1;
                    break;
                }
                float forwardGap = 12.0f;
                float sx = px + ax * forwardGap;
                float sy = py + ay * forwardGap;

                // Build and initialize the projectile
                Projectile p(sx - 25, sy - 25, dirNow, type);
                Texture *tex = &ghostTex;
                if (type == 1)
                    tex = &skeletonTex;
                else if (type == 2)
                    tex = &invisibleTex;
                else if (type == 3)
                    tex = &chelnovTex;
                p.setupSprite(*tex);
                if (dirNow == 0 || dirNow == 2)
                    p.rollDir = player.facingRight ? 1 : -1;
                {
                    int br = bestPlatformRow();
                    if (br != -1 && br != p.spawnRow)
                    {
                        p.desiredRow = br;
                        p.movingToRow = true;
                        p.dir = (br < p.spawnRow ? 1 : 3);
                        p.velocityY = (p.dir == 1 ? -PROJECTILE_SPEED : PROJECTILE_SPEED);
                        p.isRolling = false;
                        p.velocityX = 0;
                    }
                }
                projectiles.push_back(p);

                player.vacuumDirection = dirNow;
                vacuumTrailTimer = 0.25f;

                shootCooldown = 0.4f;
                player.shootHold = false;
                player.animState = P_SHOOT;
                player.animStateTimer = 0;

                // Shoot particle effect
                particles.emit(Vector2f(player.getCenterX(), player.getCenterY()),
                               Color::Cyan, 15);

                cout << "[SHOT] Single shot! Type: " << type << " ["
                     << player.capturedCount << "/" << player.maxCapacity << "]" << endl;
                createSuperWaveEffect();
                vacuumSuppressTimer = 0.0f;
            }
            else
            {
                cout << "[SHOT] No enemies to shoot!" << endl;
            }
        }
        zPressed = zNow;

        // Burst shot (X)
        static bool xPressed = false;
        bool xNow = Keyboard::isKeyPressed(Keyboard::X);
        if (xNow && !xPressed && burstCooldown <= 0 && player.capturedCount > 0)
        {
            int types[5];
            int count;
            player.releaseAllEnemies(types, count);

            if (count >= 3)
                scoreManager.addVacuumBurstBonus(count);

            int dirNow = player.vacuumDirection;
            bool aimRight = Keyboard::isKeyPressed(Keyboard::D);
            bool aimUp = Keyboard::isKeyPressed(Keyboard::W);
            bool aimLeft = Keyboard::isKeyPressed(Keyboard::A);
            bool aimDown = Keyboard::isKeyPressed(Keyboard::S);
            if (!aimRight && !aimUp && !aimLeft && !aimDown)
                dirNow = player.facingRight ? 0 : 2;

            float cx = player.getCenterX();
            float cy = player.getCenterY();
            int footRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
            if (footRow == 11)
                cy -= 10.0f;

            float handOffsetX = (player.facingRight ? +player.getFrameWidth() * PLAYER_SCALE * 0.62f
                                                    : -player.getFrameWidth() * PLAYER_SCALE * 0.62f);
            float handOffsetY = 0;
            if (dirNow == 1)
                handOffsetY = -player.getFrameHeight() * PLAYER_SCALE * 0.10f;
            else if (dirNow == 3)
                handOffsetY = player.getFrameHeight() * PLAYER_SCALE * 0.20f;
            else
                handOffsetY = player.getFrameHeight() * PLAYER_SCALE * 0.26f;

            float px = cx + handOffsetX;
            float py = cy + handOffsetY;
            float ax = 0, ay = 0;
            switch (dirNow)
            {
            case 0:
                ax = 1;
                ay = 0;
                break;
            case 1:
                ax = 0;
                ay = -1;
                break;
            case 2:
                ax = -1;
                ay = 0;
                break;
            case 3:
                ax = 0;
                ay = 1;
                break;
            }
            float forwardGap = 12.0f;

            for (int i = 0; i < count; i++)
            {
                float offsetX = (i - count / 2.0f) * 30.0f;
                float sx = px + ax * forwardGap + offsetX;
                float sy = py + ay * forwardGap;
                Projectile p(sx - 25,
                             sy - 25,
                             dirNow, types[i]);
                Texture *tex = &ghostTex;
                if (types[i] == 1)
                    tex = &skeletonTex;
                else if (types[i] == 2)
                    tex = &invisibleTex;
                else if (types[i] == 3)
                    tex = &chelnovTex;
                p.setupSprite(*tex);
                if (dirNow == 0 || dirNow == 2)
                    p.rollDir = player.facingRight ? 1 : -1;
                {
                    int br = bestPlatformRow();
                    if (br != -1 && br != p.spawnRow)
                    {
                        p.desiredRow = br;
                        p.movingToRow = true;
                        p.dir = (br < p.spawnRow ? 1 : 3);
                        p.velocityY = (p.dir == 1 ? -PROJECTILE_SPEED : PROJECTILE_SPEED);
                        p.isRolling = false;
                        p.velocityX = 0;
                    }
                }
                projectiles.push_back(p);
            }

            burstCooldown = 1.5f;
            player.shootHold = false;
            player.animState = P_SHOOT;
            player.animStateTimer = 0;

            // Burst particle effect
            particles.emit(Vector2f(player.getCenterX(), player.getCenterY()),
                           Color::Yellow, 30);

            cout << "[BURST] " << count << " enemies released!" << endl;
            createSuperWaveEffect();
            vacuumTrailTimer = 0.25f;
            player.vacuumDirection = dirNow;

            vacuumSuppressTimer = 0.0f;
        }
        xPressed = xNow;

        if (vacuumTrailTimer > 0)
            vacuumTrailTimer -= dt;
        if (vacuumSuppressTimer > 0)
            vacuumSuppressTimer -= dt;

        // Auto-movement removed
        if (player.onGround)
        {
            jumpGuiding = false;
        }

        // Update player
        player.update(dt, mapPtr());

        // Update enemies and handle shooting
        for (size_t i = 0; i < enemies.size(); i++)
        {
            Enemy &enemy = enemies[i];
            if (!enemy.active || enemy.captured)
                continue;

            enemy.update(dt, mapPtr(), player.getCenterX(), player.getCenterY());

            if (enemy.type == 2)
            {
                Texture &desired = enemy.isVisible ? invisibleTex : invisibleDimTex;
                enemy.sprite.setTexture(desired);
                enemy.texPtr = &desired;
            }

            // Chelnov projectile creation
            if (enemy.type == 3 && enemy.isShooting && enemy.actionTimer < 0.5f && !enemy.hasFired)
            {
                enemy.hasFired = true;
                EnemyProjectile ep(enemy.getCenterX(), enemy.getCenterY(),
                                   player.getCenterX(), player.getCenterY());
                if (currentLevel == 2 && bombBlueTex.getSize().x > 0)
                {
                    ep.sprite.setTexture(bombBlueTex);
                    ep.frames = 7;
                    int fw = bombBlueTex.getSize().x / ep.frames;
                    int fh = bombBlueTex.getSize().y;
                    ep.sprite.setTextureRect(IntRect(0, 0, fw, fh));
                    ep.sprite.setScale(PROJECTILE_SCALE, PROJECTILE_SCALE);
                    ep.sprite.setPosition(ep.x - (fw * PROJECTILE_SCALE) / 2.0f,
                                          ep.y - (fh * PROJECTILE_SCALE) / 2.0f);
                    ep.frameW = fw;
                    ep.texH = fh;
                    ep.hasTexture = true;
                }
                else if (bombRedTex.getSize().x > 0)
                {
                    ep.sprite.setTexture(bombRedTex);
                    ep.frames = 7;
                    int fw = bombRedTex.getSize().x / ep.frames;
                    int fh = bombRedTex.getSize().y;
                    ep.sprite.setTextureRect(IntRect(0, 0, fw, fh));
                    ep.sprite.setScale(PROJECTILE_SCALE, PROJECTILE_SCALE);
                    ep.sprite.setPosition(ep.x - (fw * PROJECTILE_SCALE) / 2.0f,
                                          ep.y - (fh * PROJECTILE_SCALE) / 2.0f);
                    ep.frameW = fw;
                    ep.texH = fh;
                    ep.hasTexture = true;
                }
                enemyProjectiles.push_back(ep);
                cout << "[CHELNOV] Shooting projectile!" << endl;
            }

            if (enemy.type == 0 && enemy.isShooting && enemy.actionTimer > 0.15f &&
                enemy.actionTimer < 0.35f && !enemy.hasFired)
            {
                enemy.hasFired = true;
                EnemyProjectile ep(enemy.getCenterX(), enemy.getCenterY(),
                                   player.getCenterX(), player.getCenterY());
                if (bombRedTex.getSize().x > 0)
                {
                    ep.sprite.setTexture(bombRedTex);
                    ep.frames = 7;
                    int fw = bombRedTex.getSize().x / ep.frames;
                    int fh = bombRedTex.getSize().y;
                    ep.sprite.setTextureRect(IntRect(0, 0, fw, fh));
                    ep.sprite.setScale(PROJECTILE_SCALE, PROJECTILE_SCALE);
                    ep.sprite.setPosition(ep.x - (fw * PROJECTILE_SCALE) / 2.0f,
                                          ep.y - (fh * PROJECTILE_SCALE) / 2.0f);
                    ep.frameW = fw;
                    ep.texH = fh;
                    ep.hasTexture = true;
                }
                enemyProjectiles.push_back(ep);
                cout << "[GHOST] Spit projectile!" << endl;
            }

            // Skeleton head throw projectile
            if (enemy.type == 1 && enemy.isShooting && enemy.actionTimer > 0.35f &&
                enemy.actionTimer < 0.7f && !enemy.hasFired)
            {
                enemy.hasFired = true;
                EnemyProjectile ep(enemy.getCenterX(), enemy.getCenterY(),
                                   player.getCenterX(), player.getCenterY());
                ep.frames = 1;
                if (skeletonTex.getSize().x > 0)
                {
                    ep.sprite.setTexture(skeletonTex);
                    int framesInRow = max(1, (int)(skeletonTex.getSize().x / ENEMY_FRAME_WIDTH));
                    int fw = skeletonTex.getSize().x / framesInRow;
                    int fh = skeletonTex.getSize().y;
                    int windupStart = max(7, framesInRow / 3);
                    int headIndex = min(framesInRow - 1, windupStart + 2);
                    ep.sprite.setTextureRect(IntRect(headIndex * fw, 0, fw, fh));
                    ep.sprite.setScale(PROJECTILE_SCALE, PROJECTILE_SCALE);
                    ep.sprite.setPosition(ep.x - (fw * PROJECTILE_SCALE) / 2.0f,
                                          ep.y - (fh * PROJECTILE_SCALE) / 2.0f);
                    ep.frameW = fw;
                    ep.texH = fh;
                    ep.hasTexture = true;
                }
                enemyProjectiles.push_back(ep);
                cout << "[SKELETON] Threw head projectile!" << endl;
            }

            // Vacuum suction
            if (player.vacuumActive && player.capturedCount < player.maxCapacity && enemy.canBeCapture())
            {
                if (enemy.isInVacuumRange(player.getCenterX(), player.getCenterY(),
                                          player.vacuumDirection, player.vacuumRange, player.vacuumAngle))
                {
                    enemy.pullTowards(player.getCenterX(), player.getCenterY(), player.vacuumPower);

                    // Vacuum particle effect
                    if (rand() % 3 == 0)
                    {
                        particles.emit(Vector2f(enemy.getCenterX(), enemy.getCenterY()),
                                       Color(150, 150, 255), 3);
                    }

                    // Chelnov lean-back / stun during suction
                    if (enemy.type == 3)
                    {
                        float dx = fabs(enemy.getCenterX() - player.getCenterX());
                        float dy = fabs(enemy.getCenterY() - player.getCenterY());
                        if (dx < 200 && dy < 150)
                        {
                            enemy.animRow = 1;
                            if (dx > 80)
                            {
                                enemy.seqStart = 3;
                                enemy.animFrames = 2;
                                enemy.animFPS = 6.0f;
                            }
                            else
                            {
                                enemy.seqStart = 5;
                                enemy.animFrames = 2;
                                enemy.animFPS = 6.0f;
                            }
                        }
                    }
                    if (enemy.type == 1)
                    {
                        auto itCap = enemy.metaLabels.find("capture");
                        if (itCap != enemy.metaLabels.end())
                        {
                            int s = itCap->second.first;
                            int e = itCap->second.second;
                            enemy.animRow = 0;
                            enemy.seqStart = s;
                            enemy.animFrames = std::max(1, e - s + 1);
                            enemy.animFPS = 6.0f;
                        }
                    }

                    {
                        FloatRect hb = player.getHitbox();
                        if (enemy.collidesWith(hb.left, hb.top, (int)hb.width, (int)hb.height))
                        {
                            if (!enemy.canBeCapture())
                            {
                            }
                            else
                            {
                                enemy.captured = true;
                                enemy.active = false;
                                player.captureEnemy(enemy.type);
                                scoreManager.addCapturePoints(enemy.capturePoints, player.capturedCount);

                                particles.emit(Vector2f(enemy.getCenterX(), enemy.getCenterY()),
                                               Color::Yellow, 20);
                            }
                        }
                    }
                }
            }

            // Enemy collision
            if (!player.vacuumActive && iframeTimer <= 0)
            {
                FloatRect hb = player.getHitbox();
                int pRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
                int eRow = (int)((enemy.y + enemy.getHeight()) / CELL_SIZE);
                bool canHitByRow = (enemy.type == 3) || (abs(pRow - eRow) <= 1);
                if (canHitByRow && enemy.collidesWith(hb.left, hb.top, (int)hb.width, (int)hb.height))
                {
                    if (enemy.type == 0)
                        player.health = 0;
                    else
                        player.health--;
                    scoreManager.playerHit();
                    iframeTimer = 2.0f;
                    bool fromAbove = (enemy.getCenterY() < player.getCenterY() - 10);
                    player.animState = fromAbove ? P_SHOOT_FORWARD : P_SHOOT_SIDE;
                    player.animStateTimer = 0;

                    // Damage particle effect
                    particles.emit(Vector2f(player.getCenterX(), player.getCenterY()),
                                   Color::Red, 25);

                    cout << "[DAMAGE] Player hit! Health: " << player.health << endl;

                    if (player.health <= 0)
                    {
                        scoreManager.playerDeath();
                        player.animState = P_DEATH_FADE;
                        player.animStateTimer = 0;
                        deathTimer = 1.2f;
                        cout << "[DEFEAT] Player dying animation" << endl;
                    }
                }
            }
        }

        // Enemy separation to prevent overlap on same row
        for (size_t i = 0; i < enemies.size(); i++)
        {
            Enemy &a = enemies[i];
            if (!a.active || a.captured)
                continue;
            for (size_t j = i + 1; j < enemies.size(); j++)
            {
                Enemy &b = enemies[j];
                if (!b.active || b.captured)
                    continue;
                int aRow = (int)((a.y + a.getHeight()) / CELL_SIZE);
                int bRow = (int)((b.y + b.getHeight()) / CELL_SIZE);
                if (aRow != bRow)
                    continue;
                float minGap = (a.getWidth() + b.getWidth()) * 0.5f;
                float dx = (a.x + a.getWidth() * 0.5f) - (b.x + b.getWidth() * 0.5f);
                float overlap = minGap - fabs(dx);
                if (overlap > 0)
                {
                    float push = overlap * 0.5f;
                    if (dx > 0)
                    {
                        a.x += push;
                        b.x -= push;
                        a.velocityX = fabs(a.velocityX);
                        b.velocityX = -fabs(b.velocityX);
                    }
                    else
                    {
                        a.x -= push;
                        b.x += push;
                        a.velocityX = -fabs(a.velocityX);
                        b.velocityX = fabs(b.velocityX);
                    }
                }
            }
        }

        // Update enemy projectiles
        for (size_t i = 0; i < enemyProjectiles.size(); i++)
        {
            EnemyProjectile &ep = enemyProjectiles[i];
            if (!ep.active)
                continue;

            ep.update(dt);

            if (iframeTimer <= 0)
            {
                FloatRect hb = player.getHitbox();
                if (ep.collidesWith(hb.left, hb.top, (int)hb.width, (int)hb.height))
                {
                    player.health--;
                    scoreManager.playerHit();
                    iframeTimer = 2.0f;
                    ep.active = false;
                    // Determine impact direction for animation
                    bool forwardHit = (fabs(ep.velocityY) > fabs(ep.velocityX));
                    player.animState = forwardHit ? P_SHOOT_FORWARD : P_SHOOT_SIDE;
                    player.animStateTimer = 0;

                    // Damage particle effect
                    particles.emit(Vector2f(player.getCenterX(), player.getCenterY()),
                                   Color::Red, 25);

                    cout << "[DAMAGE] Hit by projectile! Health: " << player.health << endl;

                    if (impactTex.getSize().x > 0)
                    {
                        createImpactEffect(player.getCenterX(), player.getCenterY());
                    }

                    if (player.health <= 0)
                    {
                        scoreManager.playerDeath();
                        player.animState = P_DEATH_FADE;
                        player.animStateTimer = 0;
                        deathTimer = 1.2f;
                        cout << "[DEFEAT] Player dying animation (projectile)" << endl;
                    }
                }
            }
        }

        // Update player projectiles
        int defeatedThisFrame = 0;
        for (size_t i = 0; i < projectiles.size(); i++)
        {
            Projectile &proj = projectiles[i];
            if (!proj.active)
                continue;

            proj.update(dt, mapPtr());

            if (iframeTimer <= 0 && proj.isRolling)
            {
                FloatRect hb = player.getHitbox();
                FloatRect pb(proj.x, proj.y, (float)proj.getSize(), (float)proj.getSize());
                if (hb.intersects(pb))
                {
                    player.health--;
                    scoreManager.playerHit();
                    iframeTimer = 2.0f;
                    proj.active = false;
                    player.animState = P_HURT;
                    player.animStateTimer = 0;
                    particles.emit(Vector2f(player.getCenterX(), player.getCenterY()),
                                   Color::Red, 25);
                    if (impactTex.getSize().x > 0)
                        createImpactEffect(player.getCenterX(), player.getCenterY());
                    if (player.health <= 0)
                    {
                        scoreManager.playerDeath();
                        player.animState = P_DEATH_FADE;
                        player.animStateTimer = 0;
                        deathTimer = 1.2f;
                    }
                }
            }

            for (size_t j = 0; j < enemies.size(); j++)
            {
                if (proj.collidesWith(enemies[j]))
                {
                    if (enemies[j].type == 1)
                        createSkeletonHitEffect(enemies[j].getCenterX(), enemies[j].getCenterY());
                    enemies[j].active = false;
                    proj.active = false;
                    defeatedThisFrame++;

                    // Defeat particle effect
                    particles.emit(Vector2f(enemies[j].getCenterX(), enemies[j].getCenterY()),
                                   Color::Yellow, 20);

                    if (proj.isAerial())
                    {
                        scoreManager.addAerialBonus();
                        cout << "[AERIAL BONUS] +150 points!" << endl;
                    }

                    scoreManager.addDefeatPoints(enemies[j].capturePoints);
                    cout << "[DEFEATED] Enemy type " << enemies[j].type
                         << " (" << enemies[j].capturePoints * 2 << " points)" << endl;

                    if (flashTex.getSize().x > 0)
                    {
                        createFlashEffect(enemies[j].getCenterX(), enemies[j].getCenterY());
                    }
                    if (debrisTex.getSize().x > 0)
                    {
                        createDebrisEffect(enemies[j].getCenterX(), enemies[j].getCenterY());
                    }

                    break;
                }
            }
        }

        bool dangerApproaching = false;
        for (size_t i = 0; i < projectiles.size(); i++)
        {
            Projectile &proj = projectiles[i];
            if (!proj.active || !proj.isRolling)
                continue;
            int pRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
            int bRow = (int)((proj.y + proj.getSize()) / CELL_SIZE);
            if (pRow == bRow)
            {
                float dx = player.getCenterX() - (proj.x + proj.getSize() / 2.0f);
                if (dx * proj.velocityX < 0)
                    continue;
                if (fabs(dx) < 240.0f)
                {
                    dangerApproaching = true;
                    break;
                }
            }
        }
        if (dangerApproaching && iframeTimer <= 0 && player.onGround)
        {
            player.animState = P_ALERT;
            player.animStateTimer = 0;
        }

        if (defeatedThisFrame >= 2)
        {
            scoreManager.addMultiKillBonus(defeatedThisFrame);
            cout << "[MULTI-KILL] " << defeatedThisFrame << " enemies! Bonus awarded." << endl;
        }

        // Update powerups
        for (size_t i = 0; i < powerups.size(); i++)
        {
            Powerup &pu = powerups[i];
            if (!pu.active)
                continue;

            pu.update(dt);

            {
                FloatRect hb = player.getHitbox();
                if (pu.collidesWith(hb.left, hb.top, (int)hb.width, (int)hb.height))
                {
                    player.applyPowerup(pu.type);
                    pu.active = false;

                    // Powerup particle effect
                    Color pColor = Color::Cyan;
                    switch (pu.type)
                    {
                    case POWERUP_SPEED:
                        pColor = Color::Blue;
                        break;
                    case POWERUP_RANGE:
                        pColor = Color::Green;
                        break;
                    case POWERUP_POWER:
                        pColor = Color::Magenta;
                        break;
                    case POWERUP_LIFE:
                        pColor = Color::Red;
                        break;
                    }
                    particles.emit(Vector2f(pu.x + 24, pu.y + 24), pColor, 25);

                    cout << "[POWERUP] Collected powerup type: " << pu.type << endl;

                    if (mysteryBoxTex.getSize().x > 0)
                    {
                        createMysteryBoxEffect(pu.x, pu.y);
                    }
                    if (pu.type == POWERUP_POWER && vacuumEffectTex.getSize().x > 0)
                    {
                        createVacuumPickupEffect(pu.x + 24, pu.y + 24);
                    }
                }
            }
        }

        // Update effects (dt-based)
        for (size_t i = 0; i < effects.size(); i++)
        {
            Effect &e = effects[i];
            if (!e.active)
                continue;

            e.timer += dt;
            e.sprite.move(e.vx, e.vy);
            e.vy += e.gravity;

            float limit = (float)e.frames / e.fps;
            e.index = min(e.frames - 1, (int)(e.timer * e.fps));
            if (!e.customRects.empty())
            {
                int count = (int)e.customRects.size();
                e.index = std::min(count - 1, e.index);
                e.sprite.setTextureRect(e.customRects[e.index]);
            }
            else
            {
                int fw = std::max(1, e.frameW);
                int th = std::max(1, e.texH);
                e.sprite.setTextureRect(IntRect(e.index * fw, 0, fw, th));
            }

            if (e.timer >= limit || (e.lifetime > 0 && (e.lifetime -= dt) <= 0))
                e.active = false;
        }

        // Level completion logic - only count active enemies NOT in inventory
        int activeEnemiesOnScreen = 0;
        for (size_t i = 0; i < enemies.size(); i++)
        {
            if (enemies[i].active && !enemies[i].captured)
            {
                activeEnemiesOnScreen++;
            }
        }

        int totalRemaining = activeEnemiesOnScreen;

        // Debug output
        static float debugTimer = 0;
        debugTimer += dt;
        if (debugTimer > 3.0f)
        {
            cout << "[STATUS] Active on-screen: " << activeEnemiesOnScreen
                 << " | In inventory: " << player.capturedCount
                 << " | Total remaining: " << totalRemaining << endl;
            debugTimer = 0;
        }

        // Wave spawning system for Level 2
        if (useWaveSpawning && currentLevel == 2 && currentWave < 3)
        {
            if (activeEnemiesOnScreen == 0)
            {
                if (waveDelay > 0.0f)
                    waveDelay -= dt;
                else
                    spawnNextWave();
            }
        }

        // Level completes when NO active enemies remain on screen
        if (totalRemaining == 0)
        {
            cout << "\n*** LEVEL " << currentLevel << " COMPLETED! ***" << endl;
            cout << "Final Score: " << scoreManager.score << endl;
            cout << "Level Time: " << scoreManager.levelTime << " seconds" << endl;
            cout << "No Damage Bonus: " << (scoreManager.noDamage ? "YES" : "NO") << endl;

            scoreManager.levelComplete(currentLevel);

            // Level completion particle celebration
            for (int i = 0; i < 100; i++)
            {
                float x = rand() % SCREEN_WIDTH;
                float y = rand() % SCREEN_HEIGHT;
                Color colors[] = {Color::Yellow, Color::Cyan, Color::Magenta, Color::Green};
                particles.emit(Vector2f(x, y), colors[rand() % 4], 5);
            }

            if (currentLevel == 1)
            {
                cout << "\n========== TRANSITIONING TO LEVEL 2 ==========\n"
                     << endl;
                cout << "Bonus awarded for Level 1 completion!" << endl;
                startLevel(2);
            }
            else
            {
                if (currentLevel == 2 && useWaveSpawning && currentWave < 3)
                {
                    return;
                }
                cout << "\n========== VICTORY! ALL LEVELS COMPLETED ==========\n"
                     << endl;
                cout << "Final Score with bonuses: " << scoreManager.score << endl;
                state = VICTORY;
                bgMusic.stop();
            }
        }
    }

    void updateGameOver()
    {
        static bool rPressed = false;
        bool rNow = Keyboard::isKeyPressed(Keyboard::R);
        if (rNow && !rPressed)
        {
            scoreManager.reset();
            state = CHARACTER_SELECT;
            bgSprite.setTexture(bgTex);
            bgSprite.setScale((float)SCREEN_WIDTH / bgTex.getSize().x,
                              (float)SCREEN_HEIGHT / bgTex.getSize().y);
            menuMusic.play();
        }
        rPressed = rNow;
    }

    int bestPlatformRow()
    {
        int bestRow = -1;
        int bestCount = -1;
        for (int r = 0; r < LEVEL_HEIGHT; r++)
        {
            bool hasPlatform = false;
            for (int c = 0; c < LEVEL_WIDTH; c++)
            {
                if (levelMap[r][c] == '#')
                {
                    hasPlatform = true;
                    break;
                }
            }
            if (!hasPlatform)
                continue;
            int cnt = 0;
            for (size_t i = 0; i < enemies.size(); i++)
            {
                Enemy &e = enemies[i];
                if (!e.active || e.captured)
                    continue;
                int eRow = (int)((e.y + e.getHeight()) / CELL_SIZE);
                if (eRow == r)
                    cnt++;
            }
            if (cnt > bestCount)
            {
                bestCount = cnt;
                bestRow = r;
            }
        }
        return bestRow;
    }

    // Effect creation helpers
    void createSuperWaveEffect()
    {
        Effect sw;
        sw.timer = 0;
        sw.fps = 16.0f; // MASTERCLASS: Smoother animation
        sw.index = 0;
        sw.active = true;
        sw.vx = 0;
        sw.vy = 0;
        sw.gravity = 0;
        sw.lifetime = 0.5f;

        sw.sprite.setTexture(superWaveTex);

        // MASTERCLASS: Use frame calculation like 8.png vacuum beam
        int fh = superWaveTex.getSize().y;
        int fw = fh; // Square frames based on height
        int totalFrames = std::max(1, (int)superWaveTex.getSize().x / fw);
        sw.frames = totalFrames;

        int frameIdx = 0; // Start from first frame
        sw.sprite.setTextureRect(IntRect(frameIdx * fw, 0, fw, fh));

        // Origin: Left Center (same as 8.png in drawVacuumEffect)
        sw.sprite.setOrigin(0, fh / 2.0f);

        float cx = player.getCenterX();
        float cy = player.getCenterY();
        int footRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
        if (footRow == 11)
            cy -= 10.0f;

        float playerW = player.getFrameWidth() * PLAYER_SCALE;
        float playerH = player.getFrameHeight() * PLAYER_SCALE;
        Vector2f pos(cx, cy);
        float rotation = 0;

        // MASTERCLASS: Use EXACT SAME positioning as 8.png vacuum beam (lines 5682-5703)
        int vacDir = player.vacuumDirection;
        switch (vacDir)
        {
        case 0: // Right
            rotation = 180;
            pos.x -= playerW * (-0.35f);
            pos.y += playerH * 0.22f;
            break;
        case 1: // Up
            rotation = -90;
            pos.y -= playerH * 0.4f;
            break;
        case 2: // Left
            rotation = 0;
            pos.x += playerW * (-0.35f);
            pos.y += playerH * 0.22f;
            break;
        case 3: // Down
            rotation = 90;
            pos.y += playerH * 0.1f;
            break;
        }

        sw.sprite.setPosition(pos);
        sw.sprite.setRotation(rotation);

        // MASTERCLASS: Effects (Pulse, Scale) - same as 8.png
        float pulse = 1.0f + sin(vacuumAnimTimer * 10.0f) * 0.05f;
        float rangeBonus = (player.powerBoostTimer > 0) ? 1.4f : 1.0f;
        sw.sprite.setScale(rangeBonus, pulse);

        // Store animation rects for animated effect
        sw.customRects.clear();
        for (int i = 0; i < totalFrames; ++i)
        {
            sw.customRects.push_back(IntRect(i * fw, 0, fw, fh));
        }

        effects.push_back(sw);
    }

    void createRainbowShotEffect()
    {
        Effect rs;
        rs.timer = 0;
        rs.frames = (int)effectMetaRects["8"].size() > 0 ? (int)effectMetaRects["8"].size() : 11;
        rs.fps = 18.0f;
        rs.index = 0;
        rs.active = true;
        rs.gravity = 0;
        rs.lifetime = 0.7f;

        rs.sprite.setTexture(rainbowShotTex);

        if (!effectMetaRects["8"].empty())
        {
            IntRect r = effectMetaRects["8"][0];
            rs.sprite.setTextureRect(r);
            int dirNow = player.vacuumDirection;
            bool aimRight = Keyboard::isKeyPressed(Keyboard::D);
            bool aimUp = Keyboard::isKeyPressed(Keyboard::W);
            bool aimLeft = Keyboard::isKeyPressed(Keyboard::A);
            bool aimDown = Keyboard::isKeyPressed(Keyboard::S);
            if (!aimRight && !aimUp && !aimLeft && !aimDown)
                dirNow = player.facingRight ? 0 : 2;
            // strict compliance: no origin; compute adjusted position
            rs.customRects = effectMetaRects["8"];
        }
        else
        {
            int fw = rainbowShotTex.getSize().x / rs.frames;
            int fh = rainbowShotTex.getSize().y;
            rs.sprite.setTextureRect(IntRect(0, 0, fw, fh));
            int dirNow = player.vacuumDirection;
            bool aimRight = Keyboard::isKeyPressed(Keyboard::D);
            bool aimUp = Keyboard::isKeyPressed(Keyboard::W);
            bool aimLeft = Keyboard::isKeyPressed(Keyboard::A);
            bool aimDown = Keyboard::isKeyPressed(Keyboard::S);
            if (!aimRight && !aimUp && !aimLeft && !aimDown)
                dirNow = player.facingRight ? 0 : 2;
            // strict compliance: no origin; compute adjusted position
        }

        float scaleX = 1.0f;
        int dirNow2 = player.vacuumDirection;
        bool aimRight2 = Keyboard::isKeyPressed(Keyboard::D);
        bool aimUp2 = Keyboard::isKeyPressed(Keyboard::W);
        bool aimLeft2 = Keyboard::isKeyPressed(Keyboard::A);
        bool aimDown2 = Keyboard::isKeyPressed(Keyboard::S);
        if (!aimRight2 && !aimUp2 && !aimLeft2 && !aimDown2)
            dirNow2 = player.facingRight ? 0 : 2;
        switch (dirNow2)
        {
        case 0:
            scaleX = -1.0f;
            break;
        case 1:
            scaleX = 1.0f;
            break;
        case 2:
            scaleX = -1.0f;
            break;
        case 3:
            scaleX = 1.0f;
            break;
        }

        float cx = player.getCenterX();
        float cy = player.getCenterY();
        int footRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
        if (footRow == 11)
            cy -= 10.0f;

        float handOffsetX = (player.facingRight ? +player.getFrameWidth() * PLAYER_SCALE * 0.62f
                                                : -player.getFrameWidth() * PLAYER_SCALE * 0.62f);
        float handOffsetY = 0;
        if (player.vacuumDirection == 1)
            handOffsetY = -player.getFrameHeight() * PLAYER_SCALE * 0.10f;
        else if (player.vacuumDirection == 3)
            handOffsetY = player.getFrameHeight() * PLAYER_SCALE * 0.20f;
        else
            handOffsetY = player.getFrameHeight() * PLAYER_SCALE * 0.26f;

        float px = cx + handOffsetX;
        float py = cy + handOffsetY;

        rs.vx = 0;
        rs.vy = 0;
        float rotation = 0;
        switch (dirNow2)
        {
        case 0:
            rotation = 0;
            break;
        case 1:
            rotation = -90;
            break;
        case 2:
            rotation = 180;
            break;
        case 3:
            rotation = 90;
            break;
        }
        // strict compliance: use directional beam textures
        rs.sprite.setPosition(px, py);
        rs.sprite.setScale(scaleX, VACUUM_EFFECT_THICKNESS * (player.rangeBoostTimer > 0 ? 1.70f : 1.0f));

        effects.push_back(rs);
    }

    void createFlashEffect(float x, float y)
    {
        Effect e;
        e.timer = 0;
        e.frames = (int)effectMetaRects["3"].size() > 0 ? (int)effectMetaRects["3"].size() : 3;
        e.fps = 18.0f;
        e.index = 0;
        e.active = true;
        e.vx = 0;
        e.vy = 0;
        e.gravity = 0;
        e.lifetime = 0;
        e.sprite.setTexture(flashTex);
        e.tex = &flashTex;
        if (!effectMetaRects["3"].empty())
        {
            IntRect r = effectMetaRects["3"][0];
            e.sprite.setTextureRect(r);
            e.frameW = r.width;
            e.texH = r.height;
        }
        else
        {
            int fw = flashTex.getSize().x / e.frames;
            e.sprite.setTextureRect(IntRect(0, 0, fw, flashTex.getSize().y));
            e.frameW = fw;
            e.texH = (int)flashTex.getSize().y;
        }
        e.sprite.setPosition(x - (e.frameW * EFFECT_SCALE) / 2.0f,
                             y - (e.texH * EFFECT_SCALE) / 2.0f);
        e.sprite.setScale(EFFECT_SCALE, EFFECT_SCALE);

        effects.push_back(e);
    }

    void createDebrisEffect(float x, float y)
    {
        Effect d;
        d.timer = 0;
        d.frames = (int)effectMetaRects["9"].size() > 0 ? (int)effectMetaRects["9"].size() : 9;
        d.fps = 20.0f;
        d.index = 0;
        d.active = true;
        d.vx = (rand() % 3 - 1) * 1.5f;
        d.vy = -2.5f;
        d.gravity = 0.12f;
        d.lifetime = 0.6f;

        d.sprite.setTexture(debrisTex);
        d.tex = &debrisTex;
        if (!effectMetaRects["9"].empty())
        {
            IntRect r = effectMetaRects["9"][0];
            d.sprite.setTextureRect(r);
            d.frameW = r.width;
            d.texH = r.height;
        }
        else
        {
            int fw = debrisTex.getSize().x / d.frames;
            d.sprite.setTextureRect(IntRect(0, 0, fw, debrisTex.getSize().y));
            d.frameW = fw;
            d.texH = (int)debrisTex.getSize().y;
        }
        d.sprite.setPosition(x - (d.frameW * EFFECT_SCALE) / 2.0f,
                             y - (d.texH * EFFECT_SCALE) / 2.0f);
        d.sprite.setScale(EFFECT_SCALE, EFFECT_SCALE);

        effects.push_back(d);
    }

    void createImpactEffect(float x, float y)
    {
        Effect e;
        e.timer = 0;
        e.frames = (int)effectMetaRects["6"].size() > 0 ? (int)effectMetaRects["6"].size() : 5;
        e.fps = 20.0f;
        e.index = 0;
        e.active = true;
        e.vx = 0;
        e.vy = 0;
        e.gravity = 0;
        e.lifetime = 0;

        e.sprite.setTexture(impactTex);
        e.tex = &impactTex;
        if (!effectMetaRects["6"].empty())
        {
            IntRect r = effectMetaRects["6"][0];
            e.sprite.setTextureRect(r);
            e.frameW = r.width;
            e.texH = r.height;
        }
        else
        {
            int fw = impactTex.getSize().x / e.frames;
            e.sprite.setTextureRect(IntRect(0, 0, fw, impactTex.getSize().y));
            e.frameW = fw;
            e.texH = (int)impactTex.getSize().y;
        }
        e.sprite.setPosition(x - (e.frameW * EFFECT_SCALE) / 2.0f,
                             y - (e.texH * EFFECT_SCALE) / 2.0f);
        e.sprite.setScale(EFFECT_SCALE, EFFECT_SCALE);

        effects.push_back(e);
    }

    void createSkeletonHitEffect(float x, float y)
    {
        if (skeletonTex.getSize().x == 0)
            return;
        Effect e;
        e.timer = 0;
        e.frames = 6;
        e.fps = 14.0f;
        e.index = 0;
        e.active = true;
        e.vx = 0;
        e.vy = 0;
        e.gravity = 0;
        e.lifetime = 0.35f;
        e.sprite.setTexture(skeletonTex);
        e.tex = &skeletonTex;
        int fw = ENEMY_FRAME_WIDTH;
        int fh = skeletonTex.getSize().y;
        for (int i = 19; i <= 24; ++i)
            e.customRects.push_back(IntRect(i * fw, 0, fw, fh));
        IntRect r = e.customRects.empty() ? IntRect(19 * fw, 0, fw, fh) : e.customRects[0];
        e.sprite.setTextureRect(r);
        e.frameW = r.width;
        e.texH = r.height;
        e.sprite.setPosition(x - (e.frameW * EFFECT_SCALE) / 2.0f,
                             y - (e.texH * EFFECT_SCALE) / 2.0f);
        e.sprite.setScale(EFFECT_SCALE, EFFECT_SCALE);
        effects.push_back(e);
    }

    void createVacuumPickupEffect(float x, float y)
    {
        Effect e;
        e.timer = 0;
        // Use the 'disappear' label range from 2_meta.json
        std::pair<int, int> range = std::make_pair(0, 0);
        auto it = effectLabels.find("2");
        if (it != effectLabels.end())
        {
            auto it2 = it->second.find("disappear");
            if (it2 != it->second.end())
                range = it2->second;
        }

        std::vector<IntRect> rects = effectMetaRects["2"];
        int start = range.first;
        int end = range.second;
        start = std::max(0, start);
        end = std::min((int)rects.size() - 1, end);
        if (start > end)
            start = end;

        for (int i = start; i <= end; ++i)
            e.customRects.push_back(rects[i]);

        e.frames = (int)e.customRects.size() > 0 ? (int)e.customRects.size() : 1;
        e.fps = 18.0f;
        e.index = 0;
        e.active = true;
        e.vx = 0;
        e.vy = 0;
        e.gravity = 0;
        e.lifetime = 0.4f;

        e.sprite.setTexture(vacuumEffectTex);
        e.tex = &vacuumEffectTex;
        if (!e.customRects.empty())
        {
            IntRect r = e.customRects[0];
            e.sprite.setTextureRect(r);
            e.frameW = r.width;
            e.texH = r.height;
        }
        else
        {
            int fw = vacuumEffectTex.getSize().x / e.frames;
            e.sprite.setTextureRect(IntRect(0, 0, fw, vacuumEffectTex.getSize().y));
            e.frameW = fw;
            e.texH = (int)vacuumEffectTex.getSize().y;
        }
        e.sprite.setPosition(x - (e.frameW * EFFECT_SCALE) / 2.0f,
                             y - (e.texH * EFFECT_SCALE) / 2.0f);
        e.sprite.setScale(EFFECT_SCALE, EFFECT_SCALE);

        effects.push_back(e);
    }

    void createMysteryBoxEffect(float x, float y)
    {
        Effect e;
        e.timer = 0;
        e.frames = (int)effectMetaRects["4"].size() > 0 ? (int)effectMetaRects["4"].size() : 7;
        e.fps = 16.0f;
        e.index = 0;
        e.active = true;
        e.vx = 0;
        e.vy = 0;
        e.gravity = 0;
        e.lifetime = 0;

        e.sprite.setTexture(mysteryBoxTex);
        if (!effectMetaRects["4"].empty())
        {
            IntRect r = effectMetaRects["4"][0];
            e.sprite.setTextureRect(r);
            e.frameW = r.width;
            e.texH = r.height;
        }
        else
        {
            int fw = mysteryBoxTex.getSize().x / e.frames;
            e.sprite.setTextureRect(IntRect(0, 0, fw, mysteryBoxTex.getSize().y));
            e.frameW = fw;
            e.texH = (int)mysteryBoxTex.getSize().y;
        }
        e.sprite.setPosition(x - (e.frameW * EFFECT_SCALE) / 2.0f,
                             y - (e.texH * EFFECT_SCALE) / 2.0f);
        e.sprite.setScale(EFFECT_SCALE, EFFECT_SCALE);

        effects.push_back(e);
    }

    // Main render dispatcher for each game state
    void render()
    {
        window.clear(Color(20, 20, 40));

        switch (state)
        {
        case CHARACTER_SELECT:
            renderCharacterSelect();
            break;
        case LEVEL_1:
        case LEVEL_2:
            renderGameplay();
            break;
        case GAME_OVER:
            renderGameOver();
            break;
        case VICTORY:
            renderVictory();
            break;
        }

        window.display();
    }

    void renderCharacterSelect()
    {
        bgSprite.setTexture(bgTex);
        bgSprite.setScale((float)SCREEN_WIDTH / bgTex.getSize().x,
                          (float)SCREEN_HEIGHT / bgTex.getSize().y);
        window.draw(bgSprite);

        drawRectTex(uiPxBlack120Tex, 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

        // MASTERCLASS: Enhanced Title with stronger animation
        {
            float tw = 850, th = 130;
            float tx = (SCREEN_WIDTH - tw) / 2;
            float ty = 25 + sin(titleAnimTimer * 2.5f) * 8;
            drawRectTex(uiPxTitleBlue230Tex, tx, ty, tw, th);
            float thick = 6;
            drawRectTex(uiPxYellowTex, tx - thick, ty - thick, tw + 2 * thick, thick);
            drawRectTex(uiPxYellowTex, tx - thick, ty + th, tw + 2 * thick, thick);
            drawRectTex(uiPxYellowTex, tx - thick, ty, thick, th);
            drawRectTex(uiPxYellowTex, tx + tw, ty, thick, th);
        }

        if (useSpriteText)
        {
            std::string t = "TUMBLEPOP";
            float ty2 = 38 + sin(titleAnimTimer * 2.5f) * 8;
            float tw = spriteTextWidth(t, 2.5f); // LARGER title text
            float tx2 = (SCREEN_WIDTH - tw) / 2;
            // Enhanced shadow effect
            drawSpriteText(window, t, tx2 - 3, ty2 + 3, 2.5f, Color::Black);
            drawSpriteText(window, t, tx2 - 2, ty2, 2.5f, Color::Black);
            drawSpriteText(window, t, tx2 + 2, ty2, 2.5f, Color::Black);
            drawSpriteText(window, t, tx2, ty2 - 2, 2.5f, Color::Black);
            drawSpriteText(window, t, tx2, ty2 + 2, 2.5f, Color::Black);
            drawSpriteText(window, t, tx2, ty2, 2.5f, Color::Yellow);

            std::string sub = "MASTERCLASS EDITION";
            float sw = spriteTextWidth(sub, 1.2f);
            float sx2 = (SCREEN_WIDTH - sw) / 2;
            drawSpriteText(window, sub, sx2, 115, 1.2f, Color::Cyan);
        }

        // MASTERCLASS: Much larger character boxes
        float boxWidth = CHAR_SELECT_BOX_W;  // 420 pixels wide
        float boxHeight = CHAR_SELECT_BOX_H; // 540 pixels tall
        float boxY = 175;
        float leftBoxX = SCREEN_WIDTH / 2 - boxWidth - 60;
        float rightBoxX = SCREEN_WIDTH / 2 + 60;

        // MASTERCLASS: Enhanced bounce with breathing effect
        float breatheScale = 1.0f + sin(characterSelectTimer * 3.0f) * CHAR_SELECT_BREATHE_SCALE;
        float yellowBounce = (selectedCharacter == 0) ? sin(characterSelectTimer * 4.0f) * CHAR_SELECT_BOUNCE : 0;
        float greenBounce = (selectedCharacter == 1) ? sin(characterSelectTimer * 4.0f) * CHAR_SELECT_BOUNCE : 0;

        // YELLOW box (LEFT) - MASTERCLASS enhanced with glow effect
        {
            float thick = selectedCharacter == 0 ? 8.0f : 4.0f; // Thicker border when selected
            float bx = leftBoxX, by = boxY + yellowBounce;

            // Draw glow effect for selected character
            if (selectedCharacter == 0)
            {
                float glowPulse = (sin(characterSelectTimer * CHAR_SELECT_GLOW_PULSE) + 1) * 0.5f;
                float glowSize = 12.0f + glowPulse * 8.0f;
                drawRectTex(uiPxYellowTex, bx - glowSize, by - glowSize, boxWidth + 2 * glowSize, boxHeight + 2 * glowSize);
            }

            drawRectTex(uiPxYellowBox200Tex, bx, by, boxWidth, boxHeight);
            Texture &outTex = (selectedCharacter == 0 ? uiPxYellowTex : uiPxOutlineGray80Tex);
            drawRectTex(outTex, bx - thick, by - thick, boxWidth + 2 * thick, thick);
            drawRectTex(outTex, bx - thick, by + boxHeight, boxWidth + 2 * thick, thick);
            drawRectTex(outTex, bx - thick, by, thick, boxHeight);
            drawRectTex(outTex, bx + boxWidth, by, thick, boxHeight);
        }

        // Sweep effect for selected yellow
        if (selectedCharacter == 0 && uiSweep18Tex.getSize().x > 0)
        {
            Sprite sweep(uiSweep18Tex);
            sweep.setTextureRect(IntRect(0, 0, (int)uiSweep18Tex.getSize().x, (int)uiSweep18Tex.getSize().y));
            float sx = leftBoxX - boxWidth * 0.15f + fmod(characterSelectTimer * 280.0f, boxWidth + 350);
            float sy = boxY - 25 + yellowBounce;
            sweep.setPosition(sx, sy);
            window.draw(sweep);
        }

        // GREEN box (RIGHT) - MASTERCLASS enhanced with glow effect
        {
            float thick = selectedCharacter == 1 ? 8.0f : 4.0f;
            float bx = rightBoxX, by = boxY + greenBounce;

            // Draw glow effect for selected character
            if (selectedCharacter == 1)
            {
                float glowPulse = (sin(characterSelectTimer * CHAR_SELECT_GLOW_PULSE) + 1) * 0.5f;
                float glowSize = 12.0f + glowPulse * 8.0f;
                drawRectTex(uiPxYellowTex, bx - glowSize, by - glowSize, boxWidth + 2 * glowSize, boxHeight + 2 * glowSize);
            }

            drawRectTex(uiPxGreenBox200Tex, bx, by, boxWidth, boxHeight);
            Texture &outTex2 = (selectedCharacter == 1 ? uiPxYellowTex : uiPxOutlineGray80Tex);
            drawRectTex(outTex2, bx - thick, by - thick, boxWidth + 2 * thick, thick);
            drawRectTex(outTex2, bx - thick, by + boxHeight, boxWidth + 2 * thick, thick);
            drawRectTex(outTex2, bx - thick, by, thick, boxHeight);
            drawRectTex(outTex2, bx + boxWidth, by, thick, boxHeight);
        }

        // Sweep effect for selected green
        if (selectedCharacter == 1 && uiSweep18Tex.getSize().x > 0)
        {
            Sprite sweep(uiSweep18Tex);
            sweep.setTextureRect(IntRect(0, 0, (int)uiSweep18Tex.getSize().x, (int)uiSweep18Tex.getSize().y));
            float sx = rightBoxX - boxWidth * 0.15f + fmod(characterSelectTimer * 280.0f, boxWidth + 350);
            float sy = boxY - 25 + greenBounce;
            sweep.setPosition(sx, sy);
            window.draw(sweep);
        }

        // MASTERCLASS: Much larger animated character previews
        auto getFrameRect = [&](Texture &tex, int idx)
        {
            int w = (int)tex.getSize().x;
            int h = (int)tex.getSize().y;
            int fw = PLAYER_FRAME_WIDTH;
            int frames = std::max(1, w / fw);
            idx = std::max(0, std::min(idx, frames - 1));
            return IntRect(idx * fw, 0, fw, h);
        };

        // MASTERCLASS: Smoother animation with more frames
        int frame = ((int)(characterSelectTimer * 8)) % 4 + 4; // Faster animation

        // YELLOW PLAYER - MASTERCLASS: Much larger with breathing effect
        if (yellowRow1Tex.getSize().x > 0)
        {
            Sprite yellowPreview;
            yellowPreview.setTexture(yellowRow1Tex);
            IntRect r = getFrameRect(yellowRow1Tex, frame);
            IntRect rflip(r.left + r.width, r.top, -r.width, r.height);
            yellowPreview.setTextureRect(rflip);

            // MASTERCLASS: HUGE character scale (4.2x)
            float baseScale = CHAR_SELECT_SCALE;
            float finalScale = baseScale * (selectedCharacter == 0 ? breatheScale : 1.0f);

            yellowPreview.setScale(finalScale, finalScale);
            float previewX = leftBoxX + boxWidth / 2.0f;
            float previewY = boxY + boxHeight * 0.42f + yellowBounce; // Positioned higher
            yellowPreview.setPosition(previewX - (r.width * finalScale) / 2.0f,
                                      previewY - (r.height * finalScale) / 2.0f);
            window.draw(yellowPreview);
        }

        // GREEN PLAYER - MASTERCLASS: Much larger with breathing effect
        if (greenRow1Tex.getSize().x > 0)
        {
            Sprite greenPreview;
            greenPreview.setTexture(greenRow1Tex);
            IntRect r = getFrameRect(greenRow1Tex, frame);
            greenPreview.setTextureRect(r);

            // MASTERCLASS: HUGE character scale (4.2x)
            float baseScale = CHAR_SELECT_SCALE;
            float finalScale = baseScale * (selectedCharacter == 1 ? breatheScale : 1.0f);

            greenPreview.setScale(finalScale, finalScale);
            float previewX = rightBoxX + boxWidth / 2.0f;
            float previewY = boxY + boxHeight * 0.42f + greenBounce; // Positioned higher
            greenPreview.setPosition(previewX - (r.width * finalScale) / 2.0f,
                                     previewY - (r.height * finalScale) / 2.0f);
            window.draw(greenPreview);
        }

        // MASTERCLASS: Enhanced text layout for larger boxes
        if (useSpriteText)
        {
            // Yellow character info - centered in box
            float yellowTextX = leftBoxX + boxWidth / 2.0f;
            float yellowTextY = boxY + boxHeight - 180 + yellowBounce;

            std::string y1 = "[ 1 ] YELLOW";
            float y1w = spriteTextWidth(y1, 1.5f);
            drawSpriteText(window, y1, yellowTextX - y1w / 2, yellowTextY, 1.5f, Color::Yellow);

            std::string y2 = "TACTICAL FIGHTER";
            float y2w = spriteTextWidth(y2, 1.0f);
            drawSpriteText(window, y2, yellowTextX - y2w / 2, yellowTextY + 40, 1.0f, Color::White);

            std::string y3 = "Vacuum: 1.2x POWER";
            float y3w = spriteTextWidth(y3, 0.95f);
            drawSpriteText(window, y3, yellowTextX - y3w / 2, yellowTextY + 70, 0.95f, Color::Cyan);

            std::string y4 = "Vacuum: 1.2x RANGE";
            float y4w = spriteTextWidth(y4, 0.95f);
            drawSpriteText(window, y4, yellowTextX - y4w / 2, yellowTextY + 95, 0.95f, Color::Cyan);

            // Green character info - centered in box
            float greenTextX = rightBoxX + boxWidth / 2.0f;
            float greenTextY = boxY + boxHeight - 180 + greenBounce;

            std::string g1 = "[ 2 ] GREEN";
            float g1w = spriteTextWidth(g1, 1.5f);
            drawSpriteText(window, g1, greenTextX - g1w / 2, greenTextY, 1.5f, Color::Green);

            std::string g2 = "SPEED DEMON";
            float g2w = spriteTextWidth(g2, 1.0f);
            drawSpriteText(window, g2, greenTextX - g2w / 2, greenTextY + 40, 1.0f, Color::White);

            std::string g3 = "Speed: 1.5x FAST";
            float g3w = spriteTextWidth(g3, 0.95f);
            drawSpriteText(window, g3, greenTextX - g3w / 2, greenTextY + 70, 0.95f, Color::Cyan);

            std::string g4 = "Agile Movement";
            float g4w = spriteTextWidth(g4, 0.95f);
            drawSpriteText(window, g4, greenTextX - g4w / 2, greenTextY + 95, 0.95f, Color::Cyan);

            // MASTERCLASS: Enhanced selection arrow
            float arrowX = (selectedCharacter == 0) ? leftBoxX + boxWidth / 2 : rightBoxX + boxWidth / 2;
            float arrowBob = sin(characterSelectTimer * 5.0f) * 15.0f;
            float arrowY = boxY - 45 + arrowBob;

            // Draw arrow with glow effect
            drawSpriteText(window, "v", arrowX - 20, arrowY - 2, 2.0f, Color::Black);
            drawSpriteText(window, "v", arrowX - 18, arrowY, 2.0f, Color::White);

            // MASTERCLASS: Pulsing start message
            float pulse = (sin(titleAnimTimer * 6) + 1) / 2;
            float startScale = 1.6f + pulse * 0.2f;
            Color pul = Color(255, 255, (Uint8)(50 + pulse * 205));
            std::string startMsg = "Press ENTER to Start!";
            float smw = spriteTextWidth(startMsg, startScale);
            drawSpriteText(window, startMsg, (SCREEN_WIDTH - smw) / 2, 735, startScale, pul);

            std::string ctl = "Press 1 or 2 to select character";
            float ctlw = spriteTextWidth(ctl, 1.1f);
            drawSpriteText(window, ctl, (SCREEN_WIDTH - ctlw) / 2, 800, 1.1f, Color(200, 200, 200));
        }

        // Draw particle effects
        particles.draw(window);
    }

    // Renders the gameplay scene including player, enemies, HUD, and effects
    void renderGameplay()
    {
        display_level(window, mapPtr(), (currentLevel == 2 ? bg2Tex : bgTex), bgSprite, platformTex, platformSprite, LEVEL_HEIGHT, LEVEL_WIDTH, CELL_SIZE);

        // '#' blocks already drawn by display_level; draw ramps and slants next

        // Draw a smooth ramp for the pre-defined 'S' staircase using dense sprite strips
        {
            int sr = 4, sc = 12;
            int er = 9, ec = 7;
            if (levelMap[sr][sc] == 'S' && levelMap[er][ec] == 'S' && platformTex.getSize().x > 0)
            {
                int x1 = sc * CELL_SIZE;
                int y1 = platformTopYAt(sr, sc);
                int x2 = ec * CELL_SIZE;
                int y2 = platformTopYAt(er, ec);
                int thick = (int)(CELL_SIZE * 0.8f);
                float dx = (float)(x2 - x1);
                float dy = (float)(y2 - y1);
                float len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
                int stepPix = 2;
                int steps = std::max(1, (int)(len / stepPix));
                for (int k = 0; k < steps; ++k)
                {
                    float t = k / (float)steps;
                    float bx = x1 + dx * t;
                    float by = y1 + dy * t;
                    int texW = (int)platformTex.getSize().x;
                    int texH = (int)platformTex.getSize().y;
                    int sw = 2;
                    int srcX = (k * sw) % std::max(1, texW);
                    Sprite slice(platformTex);
                    slice.setTextureRect(IntRect(srcX, 0, sw, texH));
                    slice.setScale(1.0f, (float)thick / texH);
                    slice.setPosition((int)bx, (int)by - thick / 2);
                    window.draw(slice);
                }
            }
        }

        // Draw smooth ramps for randomized slants using dense sprite strips
        if (platformTex.getSize().x > 0)
        {
            int thick = (int)(CELL_SIZE * 0.8f);
            for (int i = 0; i < LEVEL_HEIGHT; ++i)
            {
                int j = 0;
                while (j < LEVEL_WIDTH)
                {
                    char cell = levelMap[i][j];
                    if (cell == '/' || cell == '\\')
                    {
                        // Aggregate contiguous slant segment [startCol..endCol]
                        int startCol = j;
                        while (j + 1 < LEVEL_WIDTH && levelMap[i][j + 1] == cell)
                            ++j;
                        int endCol = j;
                        int x1 = startCol * CELL_SIZE;
                        int y1 = platformTopYAt(i, startCol);
                        int x2 = endCol * CELL_SIZE;
                        int y2 = platformTopYAt(i, endCol);
                        float dx = (float)(x2 - x1);
                        float dy = (float)(y2 - y1);
                        float len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
                        int stepPix = 2;
                        int steps = std::max(1, (int)(len / stepPix));
                        for (int k = 0; k < steps; ++k)
                        {
                            float t = k / (float)steps;
                            float bx = x1 + dx * t;
                            float by = y1 + dy * t;
                            int texW = (int)platformTex.getSize().x;
                            int texH = (int)platformTex.getSize().y;
                            int sw = 2;
                            int srcX = (k * sw) % std::max(1, texW);
                            Sprite slice(platformTex);
                            slice.setTextureRect(IntRect(srcX, 0, sw, texH));
                            slice.setScale(1.0f, (float)thick / texH);
                            slice.setPosition((int)bx, (int)by - thick / 2);
                            window.draw(slice);
                        }
                    }
                    ++j;
                }
            }
        }

        // Draw powerups with alignment to nearest platform top
        for (size_t i = 0; i < powerups.size(); i++)
        {
            if (powerups[i].active)
            {
                {
                    // Snap sprite position vertically to closest platform under its column
                    int col = (int)((powerups[i].x + 24) / CELL_SIZE);
                    int bestRow = -1;
                    int bestDist = 1000000;
                    for (int r = 0; r < LEVEL_HEIGHT; ++r)
                    {
                        if (col >= 0 && col < LEVEL_WIDTH && levelMap[r][col] == '#')
                        {
                            int visOffset = (r == 11) ? 22 : 0;
                            int targetY = r * CELL_SIZE - 17 - visOffset;
                            int dist = abs(targetY - (int)powerups[i].y);
                            if (dist < bestDist)
                            {
                                bestDist = dist;
                                bestRow = r;
                            }
                        }
                    }
                    if (bestRow >= 0)
                    {
                        int visOffset = (bestRow == 11) ? 22 : 0;
                        int targetY = bestRow * CELL_SIZE - 17 - visOffset;
                        float bob = sin(powerups[i].bobTimer * 3.0f) * 5.0f;
                        powerups[i].sprite.setPosition(powerups[i].x, targetY + bob);
                    }
                    window.draw(powerups[i].sprite);
                }
            }
        }

        // Draw active, uncaptured enemies
        for (size_t i = 0; i < enemies.size(); i++)
        {
            Enemy &e = enemies[i];
            if (e.active && !e.captured)
            {
                window.draw(e.sprite);
            }
        }

        // Draw enemy projectiles (bombs, heads), fallback to circle if missing texture
        for (size_t i = 0; i < enemyProjectiles.size(); i++)
        {
            EnemyProjectile &ep = enemyProjectiles[i];
            if (ep.active)
            {
                {
                    window.draw(ep.sprite);
                }
            }
        }

        // Draw player projectiles (rolling balls and aerial shots)
        for (size_t i = 0; i < projectiles.size(); i++)
        {
            if (projectiles[i].active)
            {
                window.draw(projectiles[i].sprite);
            }
        }

        // Draw vacuum beam or trail effect when active
        if ((player.vacuumActive || vacuumTrailTimer > 0) && vacuumSuppressTimer <= 0)
        {
            drawVacuumEffect();
        }

        // Draw player sprite with i-frame flicker and special platform offset for row 11
        if (iframeTimer <= 0 || ((int)(iframeTimer * 10) % 2 == 0))
        {
            float pOldX = player.x;
            float pOldY = player.y;
            int footRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
            if (footRow == 11)
            {
                player.sprite.setPosition(pOldX - (PLAYER_FRAME_WIDTH * PLAYER_SCALE) / 2.0f,
                                          pOldY - 22 - (PLAYER_FRAME_HEIGHT * PLAYER_SCALE));
            }
            window.draw(player.sprite);
            if (footRow == 11)
            {
                player.sprite.setPosition(pOldX - (PLAYER_FRAME_WIDTH * PLAYER_SCALE) / 2.0f,
                                          pOldY - (PLAYER_FRAME_HEIGHT * PLAYER_SCALE));
            }
        }

        // Draw heads-up display (score, level, health blocks)
        renderHUD();

        // Draw transient visual effects (impacts, pickups, debris)
        for (size_t i = 0; i < effects.size(); i++)
        {
            Effect &e = effects[i];
            if (!e.active)
                continue;
            window.draw(e.sprite);
        }
    }

    void drawVacuumEffect()
    {
        if (!player.vacuumActive && vacuumTrailTimer <= 0)
            return;

        // Determine direction (Priority: WASD > Facing)
        bool kW = Keyboard::isKeyPressed(Keyboard::W);
        bool kA = Keyboard::isKeyPressed(Keyboard::A);
        bool kS = Keyboard::isKeyPressed(Keyboard::S);
        bool kD = Keyboard::isKeyPressed(Keyboard::D);
        int vacDir = -1;
        if (kD)
            vacDir = 0; // Right
        else if (kA)
            vacDir = 2; // Left
        else if (kW)
            vacDir = 1; // Up
        else if (kS)
            vacDir = 3; // Down
        else
            vacDir = player.facingRight ? 0 : 2;

        // Texture Selection: 11.png (Super) or 8.png (Normal)
        Texture *useTex = &vacuumBeamTex;
        int useTotalFrames = vacuumTotalFrames;
        int useFrameW = vacuumFrameW;

        if (player.powerBoostTimer > 0 && superWaveTotalFrames > 0)
        {
            useTex = &superWaveTex;
            useTotalFrames = superWaveTotalFrames;
            useFrameW = superWaveFrameW;
        }

        if (useTotalFrames <= 0)
            return;

        // Frame Animation
        int frameIdx = (int)player.vacuumAnimFrame;
        if (frameIdx >= useTotalFrames)
            frameIdx = useTotalFrames - 1;
        if (frameIdx < 0)
            frameIdx = 0;

        Sprite beamSprite(*useTex);
        int fh = useTex->getSize().y;
        beamSprite.setTextureRect(IntRect(frameIdx * useFrameW, 0, useFrameW, fh));

        // Origin: Left Center
        beamSprite.setOrigin(0, fh / 2.0f);

        float cx = player.getCenterX();
        float cy = player.getCenterY();
        int footRow = (int)((player.y + player.getHeight()) / CELL_SIZE);
        if (footRow == 11)
            cy -= 10.0f;

        float playerW = player.getFrameWidth() * PLAYER_SCALE;
        float playerH = player.getFrameHeight() * PLAYER_SCALE;
        Vector2f pos(cx, cy);
        float rotation = 0;

        // Positioning & Rotation
        switch (vacDir)
        {
        case 0: // Right
            rotation = 180;
            pos.x -= playerW * (-0.35f);
            pos.y += playerH * 0.22f;
            break;
        case 1: // Up
            rotation = -90;
            pos.y -= playerH * 0.4f;
            break;
        case 2: // Left
            rotation = 0;
            pos.x += playerW * (-0.35f);
            pos.y += playerH * 0.22f;
            break;
        case 3: // Down
            rotation = 90;
            pos.y += playerH * 0.1f;
            break;
        }

        beamSprite.setPosition(pos);
        beamSprite.setRotation(rotation);

        // Effects (Pulse, Scale)
        float pulse = 1.0f + sin(vacuumAnimTimer * 10.0f) * 0.05f;
        float rangeBonus = (player.powerBoostTimer > 0) ? 1.4f : 1.0f;
        beamSprite.setScale(rangeBonus, pulse);

        window.draw(beamSprite);
    }

    // Renders player portrait, label, health line, and colored health blocks
    void renderHUD()
    {
        int maxBlocks = 3;
        int shown = std::min(maxBlocks, player.health);
        float baseX = 20;
        float baseY = 48;

        if (player.sheetRow1)
        {
            int n = std::max(1, player.row1TotalFrames);
            int barIdx = -1, labelIdx = -1, portIdx = -1, blockIdx = -1;
            int maxW = 0;
            for (int i = 0; i < n; i++)
            {
                IntRect r = player.getRectForFrame(player.sheetRow1, i);
                int w = r.width;
                if (w > maxW)
                {
                    maxW = w;
                    barIdx = i;
                }
            }
            int targetBarMin = 100;
            if (barIdx >= 0)
            {
                IntRect br0 = player.getRectForFrame(player.sheetRow1, barIdx);
                if (br0.width < targetBarMin)
                {
                    for (int i = n - 1; i >= 0; --i)
                    {
                        IntRect r = player.getRectForFrame(player.sheetRow1, i);
                        if (r.width >= targetBarMin)
                        {
                            barIdx = i;
                            break;
                        }
                    }
                }
            }
            int searchStart = std::max(0, barIdx - 6);
            for (int i = searchStart; i < barIdx; ++i)
            {
                IntRect r = player.getRectForFrame(player.sheetRow1, i);
                // Narrow frames near the bar serve as HUD label and portrait
                if (labelIdx == -1 && r.width <= 40)
                    labelIdx = i;
                else if (portIdx == -1 && r.width <= 30)
                    portIdx = i;
            }
            if (portIdx == -1 && labelIdx >= 0)
                portIdx = std::max(0, labelIdx - 1);
            if (labelIdx == -1 && portIdx >= 0)
                labelIdx = std::min(barIdx - 1, portIdx + 1);
            blockIdx = std::min(n - 1, barIdx + 1);

            if (portIdx >= 0)
            {
                Sprite p(*player.sheetRow1);
                IntRect pr = player.getRectForFrame(player.sheetRow1, portIdx);
                // Crop upper portion for a compact portrait look
                IntRect prC(pr.left, pr.top, pr.width, std::max(1, pr.height * 40 / 100));
                p.setTextureRect(prC);
                p.setPosition(baseX, baseY - 30);
                p.setScale(1.45f, 1.45f);
                window.draw(p);
                baseX += prC.width * 1.45f + 8;
            }

            if (labelIdx >= 0)
            {
                Sprite s(*player.sheetRow1);
                IntRect lr = player.getRectForFrame(player.sheetRow1, labelIdx);
                // Crop upper portion to create a thin label banner
                IntRect lrC(lr.left, lr.top, lr.width, std::max(1, lr.height * 40 / 100));
                s.setTextureRect(lrC);
                s.setPosition(baseX, baseY - 30);
                s.setScale(1.45f, 1.45f);
                window.draw(s);
                baseX += lrC.width * 1.45f + 12;
            }

            if (barIdx >= 0)
            {
                Sprite bar(*player.sheetRow1);
                IntRect br = player.getRectForFrame(player.sheetRow1, barIdx);
                IntRect brC(br.left, br.top, br.width, std::max(1, br.height * 40 / 100));
                bar.setTextureRect(brC);
                bar.setPosition(baseX, baseY);
                bar.setScale(1.15f, 1.15f);
                window.draw(bar);

                float barW = brC.width * 1.15f;
                float blockSize = 24.0f;
                float gap = 8.0f;
                float totalW = maxBlocks * blockSize + (maxBlocks - 1) * gap;
                float startX = baseX + std::max(0.0f, (barW - totalW) * 0.5f);
                float blockY = baseY + brC.height * 1.15f + 6.0f;

                // Use a small cropped sprite as health blocks (no shapes)
                IntRect hb = IntRect(br.left, br.top, std::max(8, br.width / 10), std::max(8, br.height / 10));
                for (int i = 0; i < maxBlocks; i++)
                {
                    Sprite blk(*player.sheetRow1);
                    blk.setTextureRect(hb);
                    blk.setPosition(startX + i * (blockSize + gap), blockY);
                    blk.setScale(blockSize / (float)hb.width, blockSize / (float)hb.height);
                    if (i >= shown)
                        blk.setScale(0, 0); // hide depleted blocks
                    window.draw(blk);
                }
            }
        }

        if (useSpriteText)
        {
            // MASTERCLASS: Enhanced score display with animation
            float scoreScale = 1.4f;
            // Pulse effect when score increases
            if (scoreManager.scoreAnimTimer > 0)
            {
                scoreScale = 1.4f + scoreManager.scoreAnimTimer * 0.4f;
            }

            // Score with shadow effect
            std::string scoreStr = "SCORE: " + to_string(scoreManager.score);
            drawSpriteText(window, scoreStr, SCREEN_WIDTH - 302, 22, scoreScale, Color::Black);
            drawSpriteText(window, scoreStr, SCREEN_WIDTH - 300, 20, scoreScale, Color::Yellow);

            // Combo counter with glow effect
            if (scoreManager.combo > 1)
            {
                float comboPulse = 1.0f + sin(titleAnimTimer * 8.0f) * 0.1f;
                std::string comboStr = "COMBO x" + to_string(scoreManager.combo);
                Color comboColor = (scoreManager.combo >= 5) ? Color::Cyan : (scoreManager.combo >= 3) ? Color::Yellow
                                                                                                       : Color::White;
                drawSpriteText(window, comboStr, SCREEN_WIDTH - 202, 62, 1.2f * comboPulse, Color::Black);
                drawSpriteText(window, comboStr, SCREEN_WIDTH - 200, 60, 1.2f * comboPulse, comboColor);
            }

            // Level indicator with emphasis
            std::string lvlStr = "LEVEL " + to_string(currentLevel);
            float lvlW = spriteTextWidth(lvlStr, 1.3f);
            drawSpriteText(window, lvlStr, (SCREEN_WIDTH - lvlW) / 2 + 2, 22, 1.3f, Color::Black);
            drawSpriteText(window, lvlStr, (SCREEN_WIDTH - lvlW) / 2, 20, 1.3f, Color::Cyan);

            // Enemy counter - Fix: Use GameFont for reliable number rendering
            int remaining = 0;
            for (size_t i = 0; i < enemies.size(); i++)
            {
                if (enemies[i].active && !enemies[i].captured)
                    remaining++;
            }

            // Draw label with sprite text
            std::string enemyLabel = "Enemies:";
            float enemyLabelW = spriteTextWidth(enemyLabel, 1.1f);
            float totalW = enemyLabelW + 30; // space for number
            float startX = (SCREEN_WIDTH - totalW) / 2;

            drawSpriteText(window, enemyLabel, startX + 1, 56, 1.1f, Color::Black);
            drawSpriteText(window, enemyLabel, startX, 55, 1.1f, Color::White);

            // Draw number with TTF font to avoid sprite doubling glitch
            Text numText(to_string(remaining), gameFont, 24);
            numText.setOutlineColor(Color::Black);
            numText.setOutlineThickness(2);
            numText.setFillColor((remaining <= 3) ? Color::Green : (remaining <= 6) ? Color::Yellow
                                                                                    : Color::White);
            numText.setPosition(startX + enemyLabelW + 5, 52);
            window.draw(numText);

            // Captured enemies indicator
            if (player.capturedCount > 0)
            {
                std::string capStr = "Captured: " + to_string(player.capturedCount);
                drawSpriteText(window, capStr, 22, SCREEN_HEIGHT - 52, 1.0f, Color::Black);
                drawSpriteText(window, capStr, 20, SCREEN_HEIGHT - 50, 1.0f, Color::Cyan);
            }

            // Controls hint - subtler
            std::string ctrlStr = "[Z] Shoot  [X] Burst  [SPACE] Vacuum  [WASD] Aim";
            float ctrlW = spriteTextWidth(ctrlStr, 0.85f);
            drawSpriteText(window, ctrlStr, (SCREEN_WIDTH - ctrlW) / 2, SCREEN_HEIGHT - 28, 0.85f, Color(120, 120, 120));
        }
    }

    void renderGameOver()
    {
        window.draw(bgSprite);

        drawRectTex(uiPxRedOverlay180Tex, 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

        if (useSpriteText)
        {
            // MASTERCLASS: Enhanced game over screen
            std::string t = "GAME OVER";
            float tw = spriteTextWidth(t, 3.0f);
            float ty = 260 + sin(titleAnimTimer * 2.0f) * 5.0f;
            drawSpriteText(window, t, (SCREEN_WIDTH - tw) / 2 + 3, ty + 3, 3.0f, Color::Black);
            drawSpriteText(window, t, (SCREEN_WIDTH - tw) / 2, ty, 3.0f, Color::Red);

            std::string s = "Final Score: " + to_string(scoreManager.score);
            float sw = spriteTextWidth(s, 1.8f);
            drawSpriteText(window, s, (SCREEN_WIDTH - sw) / 2 + 2, 392, 1.8f, Color::Black);
            drawSpriteText(window, s, (SCREEN_WIDTH - sw) / 2, 390, 1.8f, Color::White);

            float pulse = (sin(titleAnimTimer * 4.0f) + 1) / 2;
            Color restartColor = Color(255, 255, (Uint8)(100 + pulse * 155));
            std::string r = "Press R to Restart";
            float rw = spriteTextWidth(r, 1.5f);
            drawSpriteText(window, r, (SCREEN_WIDTH - rw) / 2, 500, 1.5f, restartColor);
        }
    }

    void renderVictory()
    {
        window.draw(bgSprite);

        drawRectTex(uiPxGreenOverlay180Tex, 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

        if (useSpriteText)
        {
            // MASTERCLASS: Enhanced victory screen
            std::string t = "VICTORY!";
            float tw = spriteTextWidth(t, 3.2f);
            float ty = 240 + sin(titleAnimTimer * 3.0f) * 8.0f;
            drawSpriteText(window, t, (SCREEN_WIDTH - tw) / 2 + 3, ty + 3, 3.2f, Color::Black);
            drawSpriteText(window, t, (SCREEN_WIDTH - tw) / 2, ty, 3.2f, Color::Yellow);

            std::string s = "Final Score: " + to_string(scoreManager.score);
            float sw = spriteTextWidth(s, 2.0f);
            drawSpriteText(window, s, (SCREEN_WIDTH - sw) / 2 + 2, 382, 2.0f, Color::Black);
            drawSpriteText(window, s, (SCREEN_WIDTH - sw) / 2, 380, 2.0f, Color::Cyan);

            float pulse = (sin(titleAnimTimer * 4.0f) + 1) / 2;
            Color restartColor = Color(255, 255, (Uint8)(100 + pulse * 155));
            std::string r = "Press R for New Game";
            float rw = spriteTextWidth(r, 1.6f);
            drawSpriteText(window, r, (SCREEN_WIDTH - rw) / 2, 490, 1.6f, restartColor);
        }
    }
};

// ============================================================================
// MAIN
// ============================================================================
int main()
{
    cout << "\n========================================" << endl;
    cout << " TUMBLEPOP - MASTERCLASS EDITION v8.0 " << endl;
    cout << " Portfolio-Ready Professional Build " << endl;
    cout << "========================================\n"
         << endl;

    try
    {
        Game game;
        game.run();
    }
    catch (const exception &e)
    {
        cout << "ERROR: " << e.what() << endl;
        cin.get();
        return 1;
    }

    return 0;
}
void Game::drawSpriteText(RenderWindow &w, const std::string &text, float x, float y, float scale, Color color)
{
    if (!useSpriteText || uiFontTex.getSize().x == 0)
        return;
    Sprite s(uiFontTex);
    float cx = x;
    for (char c : text)
    {
        auto it = uiGlyphs.find(c);
        if (it == uiGlyphs.end())
        {
            // Fallback to space advance
            cx += 10 * scale;
            continue;
        }
        s.setTextureRect(it->second);
        s.setPosition(cx, y);
        s.setScale(scale, scale);
        // strict compliance: color is encoded in texture
        w.draw(s);
        cx += it->second.width * scale;
    }
}
float Game::spriteTextWidth(const std::string &text, float scale)
{
    if (!useSpriteText || uiFontTex.getSize().x == 0)
        return 0;
    float w = 0;
    for (char c : text)
    {
        auto it = uiGlyphs.find(c);
        if (it == uiGlyphs.end())
        {
            w += 10 * scale;
            continue;
        }
        w += it->second.width * scale;
    }
    return w;
}
void Game::drawRectTex(Texture &tex, float x, float y, float rw, float rh)
{
    if (tex.getSize().x == 0)
        return;
    Sprite s(tex);
    s.setTextureRect(IntRect(0, 0, 1, 1));
    s.setPosition(x, y);
    s.setScale(rw, rh);
    window.draw(s);
}
