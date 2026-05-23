#include "Game.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#include <conio.h>
using namespace std;

const int W = 74;
const int H = 30;
const int SHOOT_COOL = 3;
const int MARCH_SPD = 18;

// Color: pure-static utility class — returns ANSI escape strings for terminal colors
// Call enable() once at startup to activate VT-100 on Windows
class Color {
public:
    static void enable() {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD m = 0; GetConsoleMode(h, &m);
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    static string RESET() { return "\033[0m"; }
    static string BOLD() { return "\033[1m"; }
    static string CYAN() { return "\033[96m"; }
    static string MAGENTA() { return "\033[95m"; }
    static string YELLOW() { return "\033[93m"; }
    static string GREEN() { return "\033[92m"; }
    static string RED() { return "\033[91m"; }
    static string WHITE() { return "\033[97m"; }
    static string GRAY() { return "\033[90m"; }
    static string BLUE() { return "\033[94m"; }
    static string ORANGE() { return "\033[33m"; }
    static string PURPLE() { return "\033[35m"; }
private: Color() {}
};

// Renderer: wraps Win32 console handle, provides cursor positioning
// move(x,y) = 1-offset inside border; goTo(x,y) = absolute screen position
class Renderer {
public:
    Renderer() {
        _h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO ci = { 1, FALSE };
        SetConsoleCursorInfo(_h, &ci);
    }
    // move(): positions cursor inside game border (adds 1-cell offset for border)
    void move(int x, int y) {
        COORD c = { (SHORT)(x + 1), (SHORT)(y + 1) };
        SetConsoleCursorPosition(_h, c);
    }
    // goTo(): positions cursor at absolute console screen coordinates
    void goTo(int x, int y) {
        COORD c = { (SHORT)x, (SHORT)y };
        SetConsoleCursorPosition(_h, c);
    }
    // clear(): wipes entire console screen
    void clear() { system("cls"); }
    void showCursor() { CONSOLE_CURSOR_INFO ci = { 10, TRUE };  SetConsoleCursorInfo(_h, &ci); }
    void hideCursor() { CONSOLE_CURSOR_INFO ci = { 1,  FALSE }; SetConsoleCursorInfo(_h, &ci); }
    void eraseRect(int x, int y, int w, int h) {
        string blank(w, ' ');
        for (int row = 0; row < h; row++) { goTo(x, y + row); cout << blank; }
    }
private:
    HANDLE _h;
};

// CRLeaderboardEntry: one row in the high-score table — stores pilot name, score, wave
// serialize() converts to CSV for file storage; deserialize() parses it back
class CRLeaderboardEntry {
public:
    CRLeaderboardEntry() : _name(""), _score(0), _wave(0) {}
    CRLeaderboardEntry(const string& n, int s, int w) : _name(n), _score(s), _wave(w) {}
    string getName()  const { return _name; }
    int    getScore() const { return _score; }
    int    getWave()  const { return _wave; }
    void   setScore(int s) { _score = s; }
    void   setWave(int w) { _wave = w; }
    bool operator>(const CRLeaderboardEntry& o) const { return _score > o._score; }
    string serialize() const {
        return _name + "," + to_string(_score) + "," + to_string(_wave);
    }
    static CRLeaderboardEntry deserialize(const string& line) {
        CRLeaderboardEntry e;
        size_t p1 = line.find(','); if (p1 == string::npos) return e;
        size_t p2 = line.find(',', p1 + 1); if (p2 == string::npos) return e;
        try {
            e._name = line.substr(0, p1);
            e._score = stoi(line.substr(p1 + 1, p2 - p1 - 1));
            e._wave = stoi(line.substr(p2 + 1));
        }
        catch (...) {}
        return e;
    }
    string _name; int _score, _wave;
};

// CRLeaderboard: manages top-10 scores, sorted highest first
// addEntry() appends every run; save()/load() persist to cosmic_rift_scores.txt
class CRLeaderboard {
public:
    static const string FILENAME;
    static const int    MAX = 10;

    CRLeaderboard() { load(); }

    // addEntry(): records a new run — every call adds independently (no dedup)
    void addEntry(const string& name, int score, int wave) {
        if (score == 0) return;
        string lname = toLower(name);
        for (auto& e : _entries) {
            if (toLower(e.getName()) == lname) {
                if (score > e.getScore()) { e.setScore(score); e.setWave(wave); }
                sortAndTrim(); save(); return;
            }
        }
        _entries.push_back(CRLeaderboardEntry(name, score, wave));
        sortAndTrim(); save();
    }

    // save(): writes all entries to cosmic_rift_scores.txt as CSV lines
    void save() const {
        ofstream f(FILENAME); if (!f.is_open()) return;
        f << "# COSMIC RIFT LEADERBOARD\n";
        for (const auto& e : _entries) f << e.serialize() << "\n";
    }

    // load(): reads score file into _entries vector, then sorts and trims to MAX
    void load() {
        _entries.clear(); ifstream f(FILENAME); if (!f.is_open()) return;
        string line;
        while (getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto e = CRLeaderboardEntry::deserialize(line);
            if (!e.getName().empty()) _entries.push_back(e);
        }
        sortAndTrim();
    }

    // getHiScore(): returns the top score, or 0 if no entries exist
    int  getHiScore() const { return _entries.empty() ? 0 : _entries[0].getScore(); }
    bool isEmpty()    const { return _entries.empty(); }
    const vector<CRLeaderboardEntry>& getEntries() const { return _entries; }

    void draw(Renderer& r) {
        r.clear();

        // Fixed column x positions — absolute screen coords, immune to color codes
        const int BX = 12;
        const int BOX = 52;
        const int C0 = 14;  // Rank
        const int C1 = 20;  // Pilot
        const int C2 = 36;  // Score
        const int C3 = 45;  // Wave

        // Title box
        r.goTo(BX, 1); cout << Color::YELLOW() << Color::BOLD() << "+" << string(BOX, '=') << "+" << Color::RESET();
        auto centred = [&](const string& txt) -> string {
            int pad = (BOX - (int)txt.size()) / 2;
            string s(max(0, pad), ' '); s += txt;
            while ((int)s.size() < BOX) s += ' '; return s;
            };
        r.goTo(BX, 2); cout << Color::YELLOW() << Color::BOLD() << "|" << centred("C O S M I C   R I F T") << "|" << Color::RESET();
        r.goTo(BX, 3); cout << Color::YELLOW() << Color::BOLD() << "|" << centred("L E A D E R B O A R D") << "|" << Color::RESET();
        r.goTo(BX, 4); cout << Color::YELLOW() << Color::BOLD() << "+" << string(BOX, '=') << "+" << Color::RESET();

        // Column headers at fixed positions
        r.goTo(C0, 6); cout << Color::WHITE() << Color::BOLD() << "Rank" << Color::RESET();
        r.goTo(C1, 6); cout << Color::WHITE() << Color::BOLD() << "Pilot" << Color::RESET();
        r.goTo(C2, 6); cout << Color::WHITE() << Color::BOLD() << "Score" << Color::RESET();
        r.goTo(C3, 6); cout << Color::WHITE() << Color::BOLD() << "Wave" << Color::RESET();
        r.goTo(BX, 7); cout << Color::GRAY() << "  " << string(BOX - 2, '-') << Color::RESET();

        if (_entries.empty()) {
            r.goTo(BX, 9);
            cout << Color::GRAY() << "  No entries yet. Be the first pilot!" << Color::RESET();
        }

        auto ri = [](int n, int w) { string s = to_string(n); while ((int)s.size() < w) s = " " + s; return s; };

        for (int i = 0; i < (int)_entries.size(); i++) {
            const auto& e = _entries[i];
            int row = 8 + i;
            string rc = i == 0 ? Color::YELLOW() : i == 1 ? Color::GRAY() : i == 2 ? Color::ORANGE() : Color::WHITE();
            string medal = i == 0 ? "1st" : i == 1 ? "2nd" : i == 2 ? "3rd" : to_string(i + 1) + "th";
            string name = e.getName();
            if ((int)name.size() > 14) name = name.substr(0, 14);

            // Each column at its fixed x — no alignment drift from color codes
            r.goTo(C0, row); cout << rc << Color::BOLD() << medal << Color::RESET();
            r.goTo(C1, row); cout << Color::WHITE() << name << Color::RESET();
            r.goTo(C2, row); cout << Color::GREEN() << ri(e.getScore(), 5) << Color::RESET();
            r.goTo(C3, row); cout << Color::CYAN() << ri(e.getWave(), 4) << Color::RESET();
        }

        int br = 8 + max(1, (int)_entries.size()) + 1;
        r.goTo(BX, br);     cout << Color::GRAY() << "  " << string(BOX - 2, '-') << Color::RESET();
        r.goTo(BX, br + 1); cout << Color::GREEN() << Color::BOLD() << "  Press any key to continue..." << Color::RESET();
    }

private:
    vector<CRLeaderboardEntry> _entries;
    static string toLower(const string& s) {
        string r = s; transform(r.begin(), r.end(), r.begin(), ::tolower); return r;
    }
    void sortAndTrim() {
        sort(_entries.begin(), _entries.end(),
            [](const CRLeaderboardEntry& a, const CRLeaderboardEntry& b) { return a > b; });
        if ((int)_entries.size() > MAX) _entries.resize(MAX);
    }
};
const string CRLeaderboard::FILENAME = "cosmic_rift_scores.txt";

// GameObject: abstract base for every visible game object
// Stores position (x,y) and alive flag; subclasses must implement update/render/erase
class GameObject {
public:
    GameObject(int x, int y) : _x(x), _y(y), _alive(true) {}
    virtual void update() = 0;
    virtual void render(Renderer& r) = 0;
    virtual void erase(Renderer& r) = 0;
    virtual int  getWidth()  const = 0;
    virtual int  getHeight() const = 0;
    bool isAlive() const { return _alive; }
    void kill() { _alive = false; }
    int  getX()    const { return _x; }
    int  getY()    const { return _y; }
    void setX(int x) { _x = x; }
    void setY(int y) { _y = y; }
    virtual ~GameObject() {}
protected:
    int _x, _y; bool _alive;
};

// Projectile: abstract bullet base — stores color, symbol, sub-pixel velocity
// Accumulates fractional movement so bullets travel smoothly at non-integer speeds
class Projectile : public GameObject {
public:
    Projectile(int x, int y, const string& col, char sym)
        : GameObject(x, y), _color(col), _sym(sym), _dx(0), _dy(0), _ax(0), _ay(0) {
    }
    // setVelocity(): sets per-tick movement in x and y (fractional values allowed)
    void setVelocity(double dx, double dy) { _dx = dx; _dy = dy; }
    void update() override {
        _ax += _dx; _ay += _dy;
        int mx = (int)_ax, my = (int)_ay;
        if (mx || my) { _x += mx; _y += my; _ax -= mx; _ay -= my; }
        if (_x<0 || _x>W || _y<0 || _y>H) _alive = false;
    }
    void render(Renderer& r) override { if (!_alive)return; r.move(_x, _y); cout << _color << Color::BOLD() << _sym << Color::RESET(); }
    void erase(Renderer& r)  override { r.move(_x, _y); cout << " "; }
    int  getWidth()  const override { return 1; }
    int  getHeight() const override { return 1; }
    virtual bool isPlayerBullet() const { return false; }
    virtual bool isEnemyBullet()  const { return false; }
protected:
    string _color; char _sym; double _dx, _dy, _ax, _ay;
};

class PlayerBullet : public Projectile {
public:
    PlayerBullet(int x, int y) : Projectile(x, y, Color::CYAN(), '|') { setVelocity(0, -3.0); }
    bool isPlayerBullet() const override { return true; }
};
class EnemyBullet : public Projectile {
public:
    EnemyBullet(int x, int y) : Projectile(x, y, Color::RED(), 'o') { setVelocity(0, 1.2); }
    bool isEnemyBullet() const override { return true; }
};
class AimedBullet : public Projectile {
public:
    AimedBullet(int x, int y, int tx, int ty) : Projectile(x, y, Color::MAGENTA(), '*') {
        double ddx = tx - x, ddy = ty - y, len = sqrt(ddx * ddx + ddy * ddy);
        if (len > 0) setVelocity(ddx / len * 1.0, ddy / len * 1.1); else setVelocity(0, 1.1);
    }
    bool isEnemyBullet() const override { return true; }
};
class SpreadBullet : public Projectile {
public:
    SpreadBullet(int x, int y, double angle) : Projectile(x, y, Color::ORANGE(), '+') {
        setVelocity(cos(angle) * 1.1, sin(angle) * 1.0);
    }
    bool isEnemyBullet() const override { return true; }
};

// Player: the player's ship — tracks lives, score, shoot cooldown, invincibility frames
// makeInvincible() grants 70 frames of immunity after taking a hit
class Player : public GameObject {
public:
    Player(int x, int y) : GameObject(x, y), _lives(3), _score(0), _shootCool(0), _invincible(0) {}
    void update() override { if (_shootCool > 0)_shootCool--; if (_invincible > 0)_invincible--; }
    // render(): draws 3-row ship sprite; flickers when invincible (dim every 4 frames)
    void render(Renderer& r) override {
        bool dim = _invincible > 0 && (_invincible / 4) % 2 == 0;
        string cy = dim ? Color::GRAY() : Color::CYAN();
        string cw = dim ? Color::GRAY() : Color::WHITE();
        string cb = dim ? Color::GRAY() : Color::BLUE();
        r.move(_x, _y - 2); cout << cw << Color::BOLD() << "^" << Color::RESET();
        r.move(_x - 1, _y - 1); cout << cy << Color::BOLD() << "(|)" << Color::RESET();
        r.move(_x - 2, _y); cout << cb << "-" << cy << Color::BOLD() << "[+]" << cb << "-" << Color::RESET();
    }
    void erase(Renderer& r) override {
        r.move(_x, _y - 2); cout << "  ";
        r.move(_x - 1, _y - 1); cout << "   ";
        r.move(_x - 2, _y); cout << "     ";
    }
    // moveLeft/Right/Up/Down: move ship one cell, clamped to play-area boundaries
    void moveLeft() { if (_x > 3)     _x--; }
    void moveRight() { if (_x < W - 3)   _x++; }
    void moveUp() { if (_y > 3)     _y--; }
    void moveDown() { if (_y < H - 1)   _y++; }
    // canShoot(): true when shoot cooldown has expired (ready to fire)
    bool canShoot()       const { return _shootCool <= 0; }
    void shoot() { _shootCool = SHOOT_COOL; }
    void makeInvincible() { _invincible = 70; }
    bool isInvincible()   const { return _invincible > 0; }
    void addScore(int s) { _score += s; }
    // loseLife(): reduces life count, grants invincibility, kills player at 0 lives
    void loseLife() { if (_lives > 0)_lives--; makeInvincible(); if (_lives <= 0)_alive = false; }
    int getLives() const { return _lives; }
    int getScore() const { return _score; }
    int getWidth()  const override { return 6; }
    int getHeight() const override { return 3; }
private:
    int _lives, _score, _shootCool, _invincible;
};

// Enemy: abstract base for all enemy types — stores HP, score value, shoot timer
// shouldShoot() counts down each tick and returns true when it fires
class Enemy : public GameObject {
public:
    Enemy(int x, int y, int hp, int score)
        : GameObject(x, y), _hp(hp), _maxHp(hp), _scoreVal(score), _shootTimer(rand() % 50 + 20) {
    }
    void hit(int dmg = 1) { _hp -= dmg; if (_hp <= 0)_alive = false; }
    bool shouldShoot() { if (--_shootTimer <= 0) { _shootTimer = 0; return true; } return false; }
    void resetShootTimer(int t) { _shootTimer = t; }
    int  getHp()       const { return _hp; }
    int  getMaxHp()    const { return _maxHp; }
    int  getScoreVal() const { return _scoreVal; }
    virtual bool isHeavy()   const { return false; }
    virtual void setTargetX(int) {}
    virtual void setTargetY(int) {}
protected:
    int _hp, _maxHp, _scoreVal, _shootTimer;
};

// Rocket: standard enemy — 1 HP, 100 pts, 3-row ASCII sprite
// Does not move on its own; WaveManager marches it left/right/down
class Rocket : public Enemy {
public:
    Rocket(int x, int y) : Enemy(x, y, 1, 100), _px(x), _py(y) {}
    void update() override {}
    void render(Renderer& r) override {
        _px = _x; _py = _y;
        r.move(_x - 1, _y); cout << Color::ORANGE() << Color::BOLD() << "/|\\" << Color::RESET();
        r.move(_x - 1, _y + 1); cout << Color::YELLOW() << Color::BOLD() << "(*)" << Color::RESET();
        r.move(_x, _y + 2); cout << Color::RED() << Color::BOLD() << "v" << Color::RESET();
    }
    void erase(Renderer& r) override {
        r.move(_px - 1, _py); cout << "   ";
        r.move(_px - 1, _py + 1); cout << "   ";
        r.move(_px, _py + 2); cout << " ";
    }
    int getWidth()  const override { return 3; }
    int getHeight() const override { return 3; }
private:
    int _px, _py;
};

// HeavyRocket: elite enemy — 4 HP, 500 pts, tracks player horizontally
// Fires a 5-bullet spread fan + aimed shot; colour changes with remaining HP
class HeavyRocket : public Enemy {
public:
    HeavyRocket(int x, int y)
        : Enemy(x, y, 4, 500), _px(x), _py(y), _tx(x), _trailTimer(0) {
    }
    void setTargetX(int tx) override { _tx = tx; }
    void update() override {
        if (_x < _tx - 1)      _x++;
        else if (_x > _tx + 1) _x--;
        _trailTimer++;
    }
    void render(Renderer& r) override {
        _px = _x; _py = _y;
        string c = _hp >= 3 ? Color::RED() : _hp == 2 ? Color::ORANGE() : Color::YELLOW();
        string inner = _hp >= 3 ? Color::WHITE() : Color::GRAY();
        r.move(_x - 3, _y); cout << c << Color::BOLD() << ".=====." << Color::RESET();
        r.move(_x - 3, _y + 1); cout << c << "|" << inner << "o" << c << "[" << inner << Color::BOLD() << "X" << Color::RESET() << c << "]" << inner << "o" << c << "|" << Color::RESET();
        r.move(_x - 3, _y + 2); cout << c << Color::BOLD() << ".=====." << Color::RESET();
        string fc = (_trailTimer / 3) % 2 == 0 ? Color::ORANGE() : Color::RED();
        r.move(_x - 1, _y + 3); cout << fc << Color::BOLD() << " |||" << Color::RESET();
    }
    void erase(Renderer& r) override {
        // Bounds-checked: never touch border columns or rows outside play area
        for (int row = 0; row <= 4; row++) {
            int ey = _py + row;
            if (ey < 0 || ey >= H) continue;
            for (int col = -6; col < 10; col++) {
                int ex = _px + col;
                if (ex < 1 || ex >= W) continue;
                r.move(ex, ey); cout << " ";
            }
        }
    }
    bool isHeavy()   const override { return true; }
    int  getWidth()  const override { return 7; }
    int  getHeight() const override { return 4; }
private:
    int _px, _py, _tx, _trailTimer;
};

// CollisionSystem: stateless AABB (axis-aligned bounding box) collision helper
// hits() returns true when the bounding boxes of two GameObjects overlap
class CollisionSystem {
public:
    bool hits(const GameObject& a, const GameObject& b) const {
        int ax1 = a.getX() - a.getWidth() / 2, ax2 = ax1 + a.getWidth();
        int ay1 = a.getY(), ay2 = ay1 + a.getHeight();
        int bx1 = b.getX() - b.getWidth() / 2, bx2 = bx1 + b.getWidth();
        int by1 = b.getY(), by2 = by1 + b.getHeight();
        return ax1<bx2 && ax2>bx1 && ay1<by2 && ay2>by1;
    }
};

// WaveManager: controls enemy waves — spawns formations and marches them Space-Invaders style
// marchUpdate() alternates sideways steps and downward steps; returns true on invasion
class WaveManager {
public:
    WaveManager() : _wave(0), _dir(1), _marchTimer(0), _stepDown(false) {}
    int  getWave()  const { return _wave; }
    void nextWave() { _wave++; _dir = 1; _marchTimer = 0; _stepDown = false; }
    void setRenderer(Renderer* r) { _r = r; }
    bool isWaveCleared(const vector<Enemy*>& enemies) const {
        for (auto* e : enemies) if (e->isAlive()) return false;
        return true;
    }
    bool marchUpdate(vector<Enemy*>& enemies) {
        _marchTimer++;
        int spd = max(2, MARCH_SPD - _wave * 2);
        if (_marchTimer < spd) return false;
        _marchTimer = 0;
        if (_stepDown) {
            for (auto* e : enemies) { if (!e->isAlive())continue; e->erase(*_r); e->setY(e->getY() + 1); }
            _stepDown = false;
        }
        else {
            int lx = W, rx = 0;
            for (auto* e : enemies) {
                if (!e->isAlive())continue;
                int el = e->getX() - e->getWidth() / 2, er = e->getX() + e->getWidth() / 2;
                if (el < lx)lx = el; if (er > rx)rx = er;
            }
            bool wall = (_dir == 1 && rx >= W - 1) || (_dir == -1 && lx <= 1);
            if (wall) { _dir *= -1; _stepDown = true; }
            else { for (auto* e : enemies) { if (!e->isAlive())continue; e->erase(*_r); e->setX(e->getX() + _dir); } }
        }
        for (auto* e : enemies) if (e->isAlive() && e->getY() + e->getHeight() >= H - 1) return true;
        return false;
    }
    vector<Enemy*> spawnWave() {
        vector<Enemy*> enemies;
        int cols = min(4 + _wave, 8), rows = min(1 + _wave / 2, 3);
        bool heavy = _wave >= 2;
        int sp = (W - 12) / max(cols, 1);
        for (int row = 0; row < rows; row++) for (int col = 0; col < cols; col++) {
            int x = 6 + col * sp + sp / 2, y = 2 + row * 7;
            if (heavy && row == 0 && (col == 0 || col == cols - 1)) enemies.push_back(new HeavyRocket(x, y));
            else                                      enemies.push_back(new Rocket(x, y));
        }
        return enemies;
    }
private:
    int _wave, _dir, _marchTimer; bool _stepDown; Renderer* _r = nullptr;
};

// HUD: draws fixed UI elements — border, status bar, wave messages, explosions
// drawInfo() updates score/lives/wave bar; drawExplosion() flashes *** at a position
class HUD {
public:
    void drawBorder(Renderer& r) {
        r.goTo(0, 0); cout << Color::BLUE() << Color::BOLD();
        cout << "+"; for (int i = 0; i < W + 2; i++)cout << "-"; cout << "+";
        for (int i = 0; i < H + 2; i++) { r.goTo(0, i + 1); cout << "|"; r.goTo(W + 3, i + 1); cout << "|"; }
        r.goTo(0, H + 3); cout << "+"; for (int i = 0; i < W + 2; i++)cout << "-"; cout << "+" << Color::RESET();
    }
    void drawInfo(Renderer& r, const string& name, int score, int lives, int wave, int hi) {
        r.goTo(1, 0);
        cout << Color::YELLOW() << Color::BOLD() << " COSMIC RIFT "
            << Color::CYAN() << "| " << name << " "
            << Color::YELLOW() << "| SCORE:" << score << " "
            << Color::CYAN() << "| WAVE:" << wave << " "
            << Color::GREEN() << "| HI:" << hi << " "
            << Color::RED() << "| LIVES:";
        for (int i = 0; i < lives; i++) cout << " <3";
        cout << "     " << Color::RESET();
        r.goTo(1, H + 4);
        cout << Color::GRAY() << " W/A/S/D=Move freely   SPACE=Fire   ESC=Quit" << Color::RESET();
    }
    void drawMessage(Renderer& r, const string& msg) {
        r.move(W / 2 - (int)msg.size() / 2, H / 2);
        cout << Color::GREEN() << Color::BOLD() << msg << Color::RESET();
    }
    void drawExplosion(Renderer& r, int x, int y, int w = 3) {
        // Clamped so explosion never overwrites border
        int sx = max(1, x - w / 2), ex = min(W - 1, x + w / 2);
        if (y >= 0 && y < H) { r.move(sx, y); cout << Color::YELLOW() << Color::BOLD(); for (int i = sx; i <= ex; i++)cout << "*"; cout << Color::RESET(); }
        Sleep(45);
        if (y >= 0 && y < H) { r.move(sx, y); for (int i = sx; i <= ex; i++)cout << " "; }
        Sleep(25);
    }
    void eraseEnemy(Renderer& r, int x, int y, int w, int h) {
        // Bounds-checked: never erase border or outside play area
        for (int row = 0; row <= h + 1; row++) {
            int ey = y + row;
            if (ey < 0 || ey >= H) continue;
            for (int col = -w / 2 - 1; col < w / 2 + 3; col++) {
                int ex = x + col;
                if (ex < 1 || ex >= W) continue;
                r.move(ex, ey); cout << " ";
            }
        }
    }
};

// ScoreBoard: Meyer's Singleton — stores the pilot name globally
// getInstance() returns the one shared instance; no copying allowed
class ScoreBoard {
public:
    static ScoreBoard& getInstance() { static ScoreBoard i; return i; }
    void   setName(const string& n) { _name = n; }
    string getName()const { return _name; }
private:
    ScoreBoard() :_name("Pilot") {}
    ScoreBoard(const ScoreBoard&) = delete;
    ScoreBoard& operator=(const ScoreBoard&) = delete;
    string _name;
};

// GameEngine: polls Win32 keyboard state each frame, tracks tick count and quit flag
// held(k) checks if key k is pressed; applyMovement() reads WASD/arrows and moves player
class GameEngine {
public:
    GameEngine() :_tick(0), _quit(false), _prevSpace(false) { for (int i = 0; i < 512; i++) _keys[i] = false; }
    void flushInput() { while (_kbhit())_getch(); Sleep(50); }
    void poll() {
        _keys[(int)'W'] = (GetAsyncKeyState('W') & 0x8000) != 0;
        _keys[(int)'S'] = (GetAsyncKeyState('S') & 0x8000) != 0;
        _keys[(int)'A'] = (GetAsyncKeyState('A') & 0x8000) != 0;
        _keys[(int)'D'] = (GetAsyncKeyState('D') & 0x8000) != 0;
        _keys[VK_UP] = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
        _keys[VK_DOWN] = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
        _keys[VK_LEFT] = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
        _keys[VK_RIGHT] = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
        _keys[VK_SPACE] = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)_quit = true;
    }
    bool held(int k)const { return k < 512 && _keys[k]; }
    void applyMovement(Player& p, Renderer& r) {
        p.erase(r);
        if (held((int)'W') || held(VK_UP))   p.moveUp();
        if (held((int)'S') || held(VK_DOWN)) p.moveDown();
        if (held((int)'A') || held(VK_LEFT)) p.moveLeft();
        if (held((int)'D') || held(VK_RIGHT))p.moveRight();
    }
    bool firePressed() { bool cur = held(VK_SPACE); _prevSpace = cur; return cur; }
    void tick() { _tick++; }
    int  getTick()   const { return _tick; }
    bool shouldQuit()const { return _quit; }
private:
    int _tick; bool _quit, _prevSpace; bool _keys[512];
};

// NOTE: Game class removed — provided by Game.h

// CosmicRift: main game controller — inherits Game, implements play()
// Orchestrates: splash → name entry → leaderboard → game loop → game over → thanks
class CosmicRift : public Game {
public:
    CosmicRift() :Game("Cosmic Rift") {}

    // play(): entry point called by launcher
    // Sets up console, shows splash, collects name, runs game loop until player quits
    void play() override {
        Color::enable();
        system("mode con: cols=84 lines=37");
        _renderer.clear();
        showSplash();
        string name = enterName();
        ScoreBoard::getInstance().setName(name);
        _lb.draw(_renderer);
        waitAnyKey();
        bool go = true;
        while (go) { go = runGame(); if (go)go = askPlayAgain(); }
        _renderer.clear();
        _renderer.showCursor();
        showThanks();
    }

private:
    Renderer    _renderer;
    CRLeaderboard _lb;

    // waitAnyKey(): drains buffered input then blocks until any key is pressed
    void waitAnyKey() { while (_kbhit())_getch(); Sleep(200); _getch(); Sleep(100); }
    // waitSpace(): blocks until SPACE is pressed; ESC exits the program immediately
    void waitSpace() {
        while (GetAsyncKeyState(VK_SPACE) & 0x8000)Sleep(30);
        while (true) {
            if (GetAsyncKeyState(VK_SPACE) & 0x8000)break;
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)exit(0);
            Sleep(20);
        }
        Sleep(200);
    }

    // enterName(): shows an input box, reads up to 14 chars; returns "Pilot" if empty
    string enterName() {
        _renderer.clear();
        int cx = W / 2 - 16;
        _renderer.goTo(cx, 5);  cout << Color::CYAN() << Color::BOLD() << "  +----------------------------------+" << Color::RESET();
        _renderer.goTo(cx, 6);  cout << Color::CYAN() << Color::BOLD() << "  |   Enter Your Pilot Name:        |" << Color::RESET();
        _renderer.goTo(cx, 7);  cout << Color::CYAN() << Color::BOLD() << "  +----------------------------------+" << Color::RESET();
        _renderer.goTo(cx, 9);  cout << Color::GRAY() << "  (max 14 chars, ENTER to confirm)" << Color::RESET();
        _renderer.goTo(cx + 4, 11); cout << Color::CYAN() << Color::BOLD() << "  Name: " << Color::RESET();
        _renderer.showCursor();
        string name = "";
        while (true) {
            int k = _getch();
            if (k == 13 || k == 10)break;
            if (k == 27)break;
            if ((k == 8 || k == 127) && !name.empty()) { name.pop_back(); cout << "\b \b" << flush; }
            else if (k >= 32 && k <= 126 && (int)name.size() < 14) { name += (char)k; cout << (char)k << flush; }
        }
        _renderer.hideCursor();
        if (name.empty())name = "Pilot";
        return name;
    }

    // showSplash(): displays ASCII logo, controls, enemy showcase; waits for SPACE
    void showSplash() {
        _renderer.clear();
        int cx = 2;
        _renderer.goTo(cx, 1);
        cout << Color::CYAN() << Color::BOLD()
            << "  ___ ___  ___ _  _ ___ ___     ___ ___ ___ _____"
            << Color::RESET();
        _renderer.goTo(cx, 2);
        cout << Color::CYAN() << Color::BOLD()
            << " / __/ _ \\/ __| \\/ |_ _/ __|   | _ \\_ _| __|_   _|"
            << Color::RESET();
        _renderer.goTo(cx, 3);
        cout << Color::CYAN() << Color::BOLD()
            << "| (_| (_) \\__ \\   / | | (__    |   /| || _|  | |"
            << Color::RESET();
        _renderer.goTo(cx, 4);
        cout << Color::YELLOW() << Color::BOLD()
            << " \\___\\___/|___/_|\\_|___\\___|   |_|_\\___|_|   |_|"
            << Color::RESET();
        _renderer.goTo(cx, 5);
        cout << Color::GRAY() << "  ------------------------------------------------" << Color::RESET();
        _renderer.goTo(cx, 6);
        cout << Color::GRAY() << "    Space Shooter  --  Defend the grid!" << Color::RESET();
        _renderer.goTo(cx, 8);
        cout << Color::WHITE() << Color::BOLD() << "  CONTROLS:" << Color::RESET();
        _renderer.goTo(cx, 9);
        cout << Color::CYAN() << "  W/A/S/D or Arrow keys  =  Move freely" << Color::RESET();
        _renderer.goTo(cx, 10);
        cout << Color::GREEN() << "  SPACE                  =  Fire laser (hold to rapid-fire)" << Color::RESET();
        _renderer.goTo(cx, 11);
        cout << Color::GRAY() << "  ESC                    =  Quit" << Color::RESET();
        _renderer.goTo(cx, 13);
        cout << Color::WHITE() << Color::BOLD() << "  ENEMIES:" << Color::RESET();
        _renderer.goTo(cx + 2, 14); cout << Color::ORANGE() << Color::BOLD() << "/|\\" << Color::RESET();
        _renderer.goTo(cx + 2, 15); cout << Color::YELLOW() << Color::BOLD() << "(*)" << Color::RESET();
        _renderer.goTo(cx + 3, 16); cout << Color::RED() << Color::BOLD() << "v" << Color::RESET();
        _renderer.goTo(cx + 8, 14); cout << Color::ORANGE() << "Rocket" << Color::RESET();
        _renderer.goTo(cx + 8, 15); cout << Color::GRAY() << "1 HP   100 pts" << Color::RESET();
        _renderer.goTo(cx + 2, 18); cout << Color::RED() << Color::BOLD() << ".=====." << Color::RESET();
        _renderer.goTo(cx + 2, 19); cout << Color::RED() << "|o" << Color::WHITE() << Color::BOLD() << "[X]" << Color::RESET() << Color::RED() << "o|" << Color::RESET();
        _renderer.goTo(cx + 2, 20); cout << Color::RED() << Color::BOLD() << ".=====." << Color::RESET();
        _renderer.goTo(cx + 3, 21); cout << Color::ORANGE() << Color::BOLD() << " |||" << Color::RESET();
        _renderer.goTo(cx + 11, 18); cout << Color::RED() << "HeavyRocket" << Color::RESET();
        _renderer.goTo(cx + 11, 19); cout << Color::GRAY() << "4 HP   500 pts" << Color::RESET();
        _renderer.goTo(cx + 11, 20); cout << Color::GRAY() << "tracks + burst fire" << Color::RESET();
        _renderer.goTo(cx, 23);
        cout << Color::WHITE() << Color::BOLD() << "  BULLETS:" << Color::RESET();
        _renderer.goTo(cx + 2, 24);
        cout << Color::RED() << "o " << Color::GRAY() << "Straight   "
            << Color::MAGENTA() << "* " << Color::GRAY() << "Aimed   "
            << Color::ORANGE() << "+ " << Color::GRAY() << "Spread fan" << Color::RESET();
        _renderer.goTo(cx, 26);
        cout << Color::GREEN() << Color::BOLD() << "  Press SPACE to begin..." << Color::RESET();
        waitSpace();
    }

    // runGame(): one full game session — player vs enemy waves
    // Returns true if player wants to continue, false if ESC was pressed
    bool runGame() {
        _renderer.clear();
        string      pname = ScoreBoard::getInstance().getName();
        Player      player(W / 2, H - 3);
        WaveManager wm;
        HUD         hud;
        GameEngine  engine;
        CollisionSystem cs;
        vector<Enemy*>      enemies;
        vector<Projectile*> bullets;

        wm.setRenderer(&_renderer);
        wm.nextWave();
        enemies = wm.spawnWave();
        hud.drawBorder(_renderer);
        hud.drawInfo(_renderer, pname, 0, player.getLives(), wm.getWave(), _lb.getHiScore());
        for (auto* e : enemies)e->render(_renderer);
        player.render(_renderer);
        engine.flushInput();

        while (player.isAlive() && !engine.shouldQuit()) {
            engine.poll();
            engine.tick();
            engine.applyMovement(player, _renderer);
            if (engine.firePressed() && player.canShoot()) {
                player.shoot();
                bullets.push_back(new PlayerBullet(player.getX(), player.getY() - 2));
            }
            player.update();

            bool invaded = wm.marchUpdate(enemies);
            if (invaded) {
                player.loseLife();
                hud.drawExplosion(_renderer, player.getX(), player.getY(), 5);
                if (!player.isAlive())break;
            }

            for (auto* e : enemies) {
                if (!e->isAlive())continue;
                if (e->isHeavy())e->setTargetX(player.getX());
                e->update();
                e->render(_renderer);
                if (e->shouldShoot()) {
                    int cool = max(8, 48 - wm.getWave() * 4);
                    e->resetShootTimer(cool + rand() % cool);
                    if (e->isHeavy()) {
                        double angles[] = { 1.5708,1.5708 + 0.4,1.5708 - 0.4,1.5708 + 0.8,1.5708 - 0.8 };
                        for (int a = 0; a < 5; a++)
                            bullets.push_back(new SpreadBullet(e->getX(), e->getY() + 5, angles[a]));
                        bullets.push_back(new AimedBullet(e->getX(), e->getY() + 5, player.getX(), player.getY()));
                    }
                    else {
                        if (rand() % 3 == 0)
                            bullets.push_back(new AimedBullet(e->getX(), e->getY() + 3, player.getX(), player.getY()));
                        else
                            bullets.push_back(new EnemyBullet(e->getX(), e->getY() + 3));
                    }
                }
            }

            for (auto* b : bullets) { if (!b->isAlive())continue; b->erase(_renderer); b->update(); b->render(_renderer); }

            for (auto* b : bullets) {
                if (!b->isAlive())continue;
                if (b->isPlayerBullet()) {
                    for (auto* e : enemies) {
                        if (!e->isAlive())continue;
                        if (cs.hits(*b, *e)) {
                            b->kill(); b->erase(_renderer);
                            e->hit();
                            if (!e->isAlive()) {
                                // Step 1: explosion flash
                                hud.drawExplosion(_renderer, e->getX(), e->getY(), e->getWidth());
                                player.addScore(e->getScoreVal());
                                // Step 2: clear entire play area top to enemy bottom
                                int clearDown = min(H - 1, e->getY() + e->getHeight() + 2);
                                for (int ty = 1; ty <= clearDown; ty++) {
                                    _renderer.move(1, ty);
                                    for (int tx = 1; tx < W; tx++) cout << " ";
                                }
                                // Step 3: redraw all still-alive enemies so they reappear
                                for (auto* other : enemies) {
                                    if (other->isAlive() && other != e)
                                        other->render(_renderer);
                                }
                            }
                        }
                    }
                }
                if (b->isEnemyBullet() && !player.isInvincible()) {
                    if (cs.hits(*b, player)) {
                        b->kill(); b->erase(_renderer);
                        player.loseLife();
                        hud.drawExplosion(_renderer, player.getX(), player.getY(), 5);
                    }
                }
            }

            enemies.erase(remove_if(enemies.begin(), enemies.end(),
                [](Enemy* e) {bool d = !e->isAlive(); if (d)delete e; return d; }), enemies.end());
            bullets.erase(remove_if(bullets.begin(), bullets.end(),
                [](Projectile* b) {bool d = !b->isAlive(); if (d)delete b; return d; }), bullets.end());

            player.render(_renderer);
            hud.drawInfo(_renderer, pname, player.getScore(), player.getLives(), wm.getWave(), _lb.getHiScore());

            if (wm.isWaveCleared(enemies)) {
                for (auto* b : bullets)delete b; bullets.clear();
                hud.drawMessage(_renderer, "  *** WAVE " + to_string(wm.getWave()) + " CLEAR!  GET READY... ***  ");
                Sleep(2000);
                wm.nextWave();
                enemies = wm.spawnWave();
                _renderer.clear();
                hud.drawBorder(_renderer);
                for (auto* e : enemies)e->render(_renderer);
                player.render(_renderer);
                hud.drawInfo(_renderer, pname, player.getScore(), player.getLives(), wm.getWave(), _lb.getHiScore());
            }
            Sleep(35);
        }

        _lb.addEntry(pname, player.getScore(), wm.getWave());
        for (auto* e : enemies)delete e;
        for (auto* b : bullets)delete b;
        if (!player.isAlive())
            showGameOver(pname, player.getScore(), wm.getWave());
        return !engine.shouldQuit();
    }

    // showGameOver(): shows score box with pilot/score/wave/best, then leaderboard
    void showGameOver(const string& name, int score, int wave) {
        _renderer.clear();
        int cx = 18, cy = 7;
        _renderer.goTo(cx, cy);   cout << Color::RED() << Color::BOLD() << " +------------------------------------------+" << Color::RESET();
        _renderer.goTo(cx, cy + 1); cout << Color::RED() << Color::BOLD() << " |           ** GAME  OVER **               |" << Color::RESET();
        _renderer.goTo(cx, cy + 2); cout << Color::YELLOW() << Color::BOLD() << " +------------------------------------------+" << Color::RESET();
        auto padded = [](const string& s, int w)->string {
            string r = s; while ((int)r.size() < w)r += " "; if ((int)r.size() > w)r = r.substr(0, w); return r; };
        _renderer.goTo(cx, cy + 4); cout << Color::CYAN() << " |  Pilot : " << padded(name, 14) << "                  |" << Color::RESET();
        _renderer.goTo(cx, cy + 5); cout << Color::WHITE() << " |  Score : " << padded(to_string(score), 8) << "                       |" << Color::RESET();
        _renderer.goTo(cx, cy + 6); cout << Color::YELLOW() << " |  Wave  : " << padded(to_string(wave), 8) << "                       |" << Color::RESET();
        _renderer.goTo(cx, cy + 7); cout << Color::GREEN() << " |  Best  : " << padded(to_string(_lb.getHiScore()), 8) << "                       |" << Color::RESET();
        _renderer.goTo(cx, cy + 8); cout << Color::YELLOW() << Color::BOLD() << " +------------------------------------------+" << Color::RESET();
        _renderer.goTo(cx, cy + 10); cout << Color::GREEN() << "   SPACE = See Leaderboard    ESC = Quit" << Color::RESET();
        while (GetAsyncKeyState(VK_SPACE) & 0x8000)Sleep(30);
        while (true) {
            if (GetAsyncKeyState(VK_SPACE) & 0x8000)break;
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)exit(0);
            Sleep(30);
        }
        Sleep(200);
        _lb.draw(_renderer);
        waitAnyKey();
    }

    // askPlayAgain(): prompts Y/N — returns true if player wants another round
    bool askPlayAgain() {
        _renderer.clear();
        _renderer.goTo(15, 12);
        cout << Color::YELLOW() << Color::BOLD() << "  Play again? (Y = yes,  N = quit): " << Color::RESET();
        _renderer.showCursor();
        while (_kbhit())_getch();
        while (true) {
            if (_kbhit()) {
                char c = (char)tolower(_getch());
                if (c == 'y') { _renderer.hideCursor(); return true; }
                if (c == 'n' || c == 27) { _renderer.hideCursor(); return false; }
            }
            Sleep(30);
        }
    }

    // showThanks(): farewell screen with top score; closes console after 3 seconds
    void showThanks() {
        _renderer.clear(); _renderer.showCursor();
        const int BX = 17;
        const string line(45, '=');
        const string blank(45, ' ');
        _renderer.goTo(BX, 8);  cout << Color::YELLOW() << Color::BOLD() << "+" << line << "+" << Color::RESET();
        _renderer.goTo(BX, 9);  cout << Color::YELLOW() << "|" << Color::RESET() << blank << Color::YELLOW() << "|" << Color::RESET();
        _renderer.goTo(BX, 10); cout << Color::YELLOW() << "|" << Color::RESET();
        string t1 = "      Thanks for playing         "; while ((int)t1.size() < 45)t1 += " ";
        cout << Color::WHITE() << Color::BOLD() << t1 << Color::RESET() << Color::YELLOW() << "|" << Color::RESET();
        _renderer.goTo(BX, 11); cout << Color::YELLOW() << "|" << Color::RESET();
        string t2 = "    C O S M I C   R I F T  !    "; while ((int)t2.size() < 45)t2 += " ";
        cout << Color::CYAN() << Color::BOLD() << t2 << Color::RESET() << Color::YELLOW() << "|" << Color::RESET();
        _renderer.goTo(BX, 12); cout << Color::YELLOW() << "|" << Color::RESET() << blank << Color::YELLOW() << "|" << Color::RESET();
        string t3 = "    Top Score : " + to_string(_lb.getHiScore()); while ((int)t3.size() < 45)t3 += " ";
        _renderer.goTo(BX, 13); cout << Color::YELLOW() << "|" << Color::RESET() << Color::GREEN() << Color::BOLD() << t3 << Color::RESET() << Color::YELLOW() << "|" << Color::RESET();
        _renderer.goTo(BX, 14); cout << Color::YELLOW() << "|" << Color::RESET() << blank << Color::YELLOW() << "|" << Color::RESET();
        _renderer.goTo(BX, 15); cout << Color::YELLOW() << Color::BOLD() << "+" << line << "+" << Color::RESET();
        _renderer.goTo(BX, 17); cout << Color::GRAY() << "   Goodbye, pilot. See you in the stars." << Color::RESET();
        Sleep(3000);
        HWND hw = GetConsoleWindow();
        if (hw)PostMessage(hw, WM_CLOSE, 0, 0); else exit(0);
    }
};
// Factory function for random launcher
// createCosmicRift(): factory function called by main launcher
// Creates and returns a CosmicRift instance via Game* pointer (polymorphism)
Game* createCosmicRift() {
    return new CosmicRift();
}