#ifndef GAME_H
#define GAME_H

// Minimal base class for all games
#include <string>

class Game {
public:
    Game(const std::string& name) : _name(name) {}
    virtual ~Game() {}
    virtual void play() = 0;
    std::string getName() const { return _name; }
private:
    std::string _name;
};

#endif // GAME_H
