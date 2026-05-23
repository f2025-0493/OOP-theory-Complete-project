#ifndef ROCKPAPERSCISSORS_H
#define ROCKPAPERSCISSORS_H

// ============================================================
// RockPaperScissors.h
// Author: [Your Name]
// AI Disclosure: Claude (Anthropic) assisted with structure,
//   color utilities, and leaderboard boilerplate.
//   Game logic, tournament design, and OOP decisions are
//   original student work.
// ============================================================
// OOP Concepts used:
//   - Inheritance     : RockPaperScissors derives from Game
//   - Composition     : has-a RPSLeaderboard, has-a RPSStats
//   - Encapsulation   : all state private; only play() is public
//   - File Handling   : persistent leaderboard in rps_scores.txt
//   - Polymorphism    : called via Game* pointer from main
// ============================================================

#include "Game.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#include <conio.h>
using namespace std;

// ────────────────────────────────────────────────────────────
// RPSColor: static ANSI color helpers (same pattern as friend)
// ────────────────────────────────────────────────────────────
class RPSColor {
public:
    static void enable() {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD m = 0; GetConsoleMode(h, &m);
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    static string RESET()   { return "\033[0m"; }
    static string BOLD()    { return "\033[1m"; }
    static string CYAN()    { return "\033[96m"; }
    static string YELLOW()  { return "\033[93m"; }
    static string GREEN()   { return "\033[92m"; }
    static string RED()     { return "\033[91m"; }
    static string WHITE()   { return "\033[97m"; }
    static string GRAY()    { return "\033[90m"; }
    static string MAGENTA() { return "\033[95m"; }
    static string ORANGE()  { return "\033[33m"; }
private: RPSColor() {}
};

// ────────────────────────────────────────────────────────────
// RPSStats: composition — tracks wins/draws/losses per session
// ────────────────────────────────────────────────────────────
struct RPSStats {
    int wins = 0, losses = 0, draws = 0;
    int total() const { return wins + losses + draws; }
    string summary() const {
        return "W:" + to_string(wins) + " L:" + to_string(losses) + " D:" + to_string(draws);
    }
};

// ────────────────────────────────────────────────────────────
// RPSLeaderboard: composition — top-10 persistent scores
//   Score = wins in a best-of-5 tournament run
// ────────────────────────────────────────────────────────────
class RPSLeaderboard {
public:
    static const string FILE_NAME;
    static const int    MAX = 10;

    struct Entry {
        string name;
        int    score;   // tournament wins
        int    rounds;  // rounds played
        bool operator>(const Entry& o) const { return score > o.score; }
        string serialize() const {
            return name + "," + to_string(score) + "," + to_string(rounds);
        }
        static Entry deserialize(const string& line) {
            Entry e{"", 0, 0};
            size_t p1 = line.find(','); if (p1 == string::npos) return e;
            size_t p2 = line.find(',', p1 + 1); if (p2 == string::npos) return e;
            try {
                e.name   = line.substr(0, p1);
                e.score  = stoi(line.substr(p1 + 1, p2 - p1 - 1));
                e.rounds = stoi(line.substr(p2 + 1));
            } catch (...) {}
            return e;
        }
    };

    RPSLeaderboard() { load(); }

    void addEntry(const string& name, int score, int rounds) {
        if (score == 0) return;
        string ln = toLower(name);
        for (auto& e : entries) {
            if (toLower(e.name) == ln) {
                if (score > e.score) { e.score = score; e.rounds = rounds; }
                sortAndTrim(); save(); return;
            }
        }
        entries.push_back({name, score, rounds});
        sortAndTrim(); save();
    }

    void save() const {
        ofstream f(FILE_NAME); if (!f.is_open()) return;
        f << "# RPS TOURNAMENT LEADERBOARD\n";
        for (const auto& e : entries) f << e.serialize() << "\n";
    }

    void load() {
        entries.clear(); ifstream f(FILE_NAME); if (!f.is_open()) return;
        string line;
        while (getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto e = Entry::deserialize(line);
            if (!e.name.empty()) entries.push_back(e);
        }
        sortAndTrim();
    }

    int  hiScore() const { return entries.empty() ? 0 : entries[0].score; }

    void draw() const {
        system("cls");
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        auto go = [&](int x, int y) { COORD c = {(SHORT)x,(SHORT)y}; SetConsoleCursorPosition(h, c); };

        go(10,1); cout << RPSColor::YELLOW() << RPSColor::BOLD()
                       << "+============================================+"
                       << RPSColor::RESET();
        go(10,2); cout << RPSColor::YELLOW() << RPSColor::BOLD()
                       << "|    RPS TOURNAMENT  --  LEADERBOARD        |"
                       << RPSColor::RESET();
        go(10,3); cout << RPSColor::YELLOW() << RPSColor::BOLD()
                       << "+============================================+"
                       << RPSColor::RESET();

        go(12,5); cout << RPSColor::WHITE() << RPSColor::BOLD()
                       << "Rank  Player           Wins  Rounds"
                       << RPSColor::RESET();
        go(12,6); cout << RPSColor::GRAY() << string(38,'-') << RPSColor::RESET();

        if (entries.empty()) {
            go(12,8); cout << RPSColor::GRAY() << "No entries yet. Be the first champion!" << RPSColor::RESET();
        }
        for (int i = 0; i < (int)entries.size(); i++) {
            const auto& e = entries[i];
            string rc = i==0 ? RPSColor::YELLOW() : i==1 ? RPSColor::GRAY() : i==2 ? RPSColor::ORANGE() : RPSColor::WHITE();
            string medal = i==0?"1st":i==1?"2nd":i==2?"3rd":to_string(i+1)+"th";
            string nm = e.name; if ((int)nm.size()>14) nm = nm.substr(0,14);
            go(12, 7+i);
            cout << rc << RPSColor::BOLD() << medal << RPSColor::RESET();
            COORD c2={(SHORT)18,(SHORT)(7+i)}; SetConsoleCursorPosition(h,c2);
            cout << RPSColor::WHITE() << nm << RPSColor::RESET();
            COORD c3={(SHORT)34,(SHORT)(7+i)}; SetConsoleCursorPosition(h,c3);
            cout << RPSColor::GREEN() << e.score << RPSColor::RESET();
            COORD c4={(SHORT)40,(SHORT)(7+i)}; SetConsoleCursorPosition(h,c4);
            cout << RPSColor::CYAN() << e.rounds << RPSColor::RESET();
        }
        int br = 8 + max(1,(int)entries.size()) + 1;
        COORD cb={(SHORT)12,(SHORT)br}; SetConsoleCursorPosition(h,cb);
        cout << RPSColor::GRAY() << string(38,'-') << RPSColor::RESET();
        COORD cc={(SHORT)12,(SHORT)(br+1)}; SetConsoleCursorPosition(h,cc);
        cout << RPSColor::GREEN() << RPSColor::BOLD() << "Press any key to continue..." << RPSColor::RESET();
    }

    vector<Entry> entries;
private:
    static string toLower(const string& s) {
        string r=s; transform(r.begin(),r.end(),r.begin(),::tolower); return r;
    }
    void sortAndTrim() {
        sort(entries.begin(), entries.end(),
             [](const Entry& a, const Entry& b){ return a>b; });
        if ((int)entries.size() > MAX) entries.resize(MAX);
    }
};
const string RPSLeaderboard::FILE_NAME = "rps_scores.txt";

// ────────────────────────────────────────────────────────────
// RockPaperScissors: main game class — derives from Game
// Tournament mode: best-of-5 rounds vs the computer
// ────────────────────────────────────────────────────────────
class RockPaperScissors : public Game {
public:
    RockPaperScissors() : Game("RPS Tournament") {}

    void play() override {
        RPSColor::enable();
        system("mode con: cols=70 lines=35");

        showSplash();
        string playerName = askName();

        bool keepPlaying = true;
        while (keepPlaying) {
            runTournament(playerName);
            keepPlaying = askPlayAgain();
        }

        showLeaderboardAndBye();
    }

private:
    RPSLeaderboard _lb;

    // ── ASCII art for each choice ─────────────────────────────
    // Each returns a 5-line vector representing the hand
    vector<string> rockArt() const {
        return {
            "   _____  ",
            "  /     \\ ",
            " | () () |",
            "  \\  ^  / ",
            "   |||||  "
        };
    }
    vector<string> paperArt() const {
        return {
            "   _____  ",
            "  |     | ",
            "  |     | ",
            "  |     | ",
            "  |_____|  "
        };
    }
    vector<string> scissorsArt() const {
        return {
            "   __  __ ",
            "  /  \\/  \\",
            "  \\  /\\  /",
            "  /  \\/  \\",
            " /__/\\__\\ "
        };
    }

    vector<string> artFor(int choice) const {
        if (choice == 0) return rockArt();
        if (choice == 1) return paperArt();
        return scissorsArt();
    }

    string nameFor(int c) const {
        const string n[] = {"ROCK","PAPER","SCISSORS"};
        return n[c];
    }

    string colorFor(int c) const {
        if (c==0) return RPSColor::CYAN();
        if (c==1) return RPSColor::GREEN();
        return RPSColor::MAGENTA();
    }

    // ── Cursor helper ─────────────────────────────────────────
    void goTo(int x, int y) const {
        COORD c={(SHORT)x,(SHORT)y};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
    }

    void hideCursor() {
        CONSOLE_CURSOR_INFO ci={1,FALSE};
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE),&ci);
    }
    void showCursor() {
        CONSOLE_CURSOR_INFO ci={10,TRUE};
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE),&ci);
    }

    // ── Splash screen ─────────────────────────────────────────
    void showSplash() {
        system("cls"); hideCursor();
        goTo(5,1);  cout << RPSColor::CYAN()   << RPSColor::BOLD() << " ____   ____   ____  " << RPSColor::RESET();
        goTo(5,2);  cout << RPSColor::CYAN()   << RPSColor::BOLD() << "|  _ \\ |  _ \\ / ___| " << RPSColor::RESET();
        goTo(5,3);  cout << RPSColor::WHITE()  << RPSColor::BOLD() << "| |_) || |_) |\\___ \\ " << RPSColor::RESET();
        goTo(5,4);  cout << RPSColor::YELLOW() << RPSColor::BOLD() << "|  _ < |  __/  ___) |" << RPSColor::RESET();
        goTo(5,5);  cout << RPSColor::YELLOW() << RPSColor::BOLD() << "|_| \\_\\|_|    |____/ " << RPSColor::RESET();
        goTo(5,6);  cout << RPSColor::GRAY()   << " Rock · Paper · Scissors  TOURNAMENT" << RPSColor::RESET();
        goTo(5,8);  cout << RPSColor::WHITE()  << RPSColor::BOLD() << "HOW TO PLAY:" << RPSColor::RESET();
        goTo(5,9);  cout << RPSColor::CYAN()   << "  1 = Rock      2 = Paper      3 = Scissors" << RPSColor::RESET();
        goTo(5,10); cout << RPSColor::GRAY()   << "  First to 3 wins takes the tournament!" << RPSColor::RESET();
        goTo(5,12); cout << RPSColor::WHITE()  << RPSColor::BOLD() << "SCORING:";
        goTo(5,13); cout << RPSColor::GREEN()  << "  Win a round   = +2 pts" << RPSColor::RESET();
        goTo(5,14); cout << RPSColor::YELLOW() << "  Draw a round  = +1 pt" << RPSColor::RESET();
        goTo(5,15); cout << RPSColor::RED()    << "  Lose a round  = +0 pts" << RPSColor::RESET();
        goTo(5,16); cout << RPSColor::CYAN()   << "  Win tourney   = +5 bonus pts!" << RPSColor::RESET();
        goTo(5,18); cout << RPSColor::GREEN()  << RPSColor::BOLD() << "  Press any key to start..." << RPSColor::RESET();
        while(_kbhit())_getch();
        _getch();
    }

    // ── Name entry ────────────────────────────────────────────
    string askName() {
        system("cls"); showCursor();
        goTo(8,5); cout << RPSColor::CYAN() << RPSColor::BOLD()
                        << "+----------------------------------+" << RPSColor::RESET();
        goTo(8,6); cout << RPSColor::CYAN() << RPSColor::BOLD()
                        << "|   Enter Your Name (max 14):     |" << RPSColor::RESET();
        goTo(8,7); cout << RPSColor::CYAN() << RPSColor::BOLD()
                        << "+----------------------------------+" << RPSColor::RESET();
        goTo(10,9); cout << RPSColor::YELLOW() << "Name: " << RPSColor::RESET();
        string name;
        while (true) {
            int k = _getch();
            if (k==13||k==10) break;
            if ((k==8||k==127) && !name.empty()) { name.pop_back(); cout<<"\b \b"<<flush; }
            else if (k>=32&&k<=126&&(int)name.size()<14) { name+=(char)k; cout<<(char)k<<flush; }
        }
        hideCursor();
        return name.empty() ? "Player" : name;
    }

    // ── Draw both hands side-by-side ─────────────────────────
    void drawHandsBattle(int playerChoice, int cpuChoice, bool reveal) {
        // Clear art area
        for (int row = 0; row < 7; row++) {
            goTo(2, 10+row); cout << string(60,' ');
        }
        auto pArt = artFor(playerChoice);
        auto cArt = reveal ? artFor(cpuChoice) : vector<string>{"  ?????  ","  ?????  ","  ?????  ","  ?????  ","  ?????  "};

        string pCol = colorFor(playerChoice);
        string cCol = reveal ? colorFor(cpuChoice) : RPSColor::GRAY();

        goTo(2,10); cout << pCol  << RPSColor::BOLD() << "   YOU            CPU" << RPSColor::RESET();
        goTo(2,11); cout << pCol  << RPSColor::BOLD() << nameFor(playerChoice);
        if (reveal) {
            goTo(22,11); cout << cCol << RPSColor::BOLD() << nameFor(cpuChoice) << RPSColor::RESET();
        }
        for (int i = 0; i < 5; i++) {
            goTo(2, 12+i);  cout << pCol  << pArt[i] << RPSColor::RESET();
            goTo(22, 12+i); cout << cCol  << cArt[i] << RPSColor::RESET();
        }
    }

    // ── One round ─────────────────────────────────────────────
    // Returns: 1=player wins, -1=cpu wins, 0=draw
    int playRound(int pWins, int cWins, int roundNum, RPSStats& stats) {
        // Draw current state
        goTo(2,7);
        cout << RPSColor::YELLOW() << RPSColor::BOLD()
             << "Round " << roundNum
             << "  |  You: " << pWins << "  CPU: " << cWins
             << "        " << RPSColor::RESET();

        // Clear choice prompt area
        for (int r=17;r<=21;r++) { goTo(2,r); cout << string(55,' '); }

        goTo(2,17); cout << RPSColor::WHITE() << RPSColor::BOLD()
                         << "Your move:  1=Rock   2=Paper   3=Scissors  (ESC=quit): "
                         << RPSColor::RESET();

        int playerChoice = -1;
        while (playerChoice == -1) {
            if (!_kbhit()) { Sleep(30); continue; }
            int k = _getch();
            if (k==27) return -99; // quit signal
            if (k>='1'&&k<='3') playerChoice = k-'1';
        }

        int cpuChoice = rand() % 3;

        // Animate: show player choice, hide cpu
        drawHandsBattle(playerChoice, cpuChoice, false);
        goTo(2,18); cout << RPSColor::CYAN() << "  Throwing..." << RPSColor::RESET();
        Sleep(700);

        // Reveal
        drawHandsBattle(playerChoice, cpuChoice, true);

        // Determine result
        int result = 0;
        if (playerChoice == cpuChoice) {
            result = 0;
        } else if ((playerChoice==0&&cpuChoice==2) ||
                   (playerChoice==1&&cpuChoice==0) ||
                   (playerChoice==2&&cpuChoice==1)) {
            result = 1;
        } else {
            result = -1;
        }

        string resultMsg;
        string resultColor;
        if (result==1)       { resultMsg="  YOU WIN this round!  "; resultColor=RPSColor::GREEN(); stats.wins++; }
        else if (result==-1) { resultMsg="  CPU WINS this round! "; resultColor=RPSColor::RED();   stats.losses++; }
        else                 { resultMsg="  DRAW!                "; resultColor=RPSColor::YELLOW();stats.draws++; }

        goTo(2,18); cout << resultColor << RPSColor::BOLD() << resultMsg << RPSColor::RESET();
        goTo(2,19); cout << RPSColor::GRAY() << "  Press any key for next round..." << RPSColor::RESET();
        while(_kbhit())_getch();
        _getch();

        return result;
    }

    // ── Full tournament (best of 5, first to 3) ───────────────
    void runTournament(const string& playerName) {
        system("cls"); hideCursor();

        // Header
        goTo(2,1); cout << RPSColor::CYAN() << RPSColor::BOLD()
                        << "+--------------------------------------------------+" << RPSColor::RESET();
        goTo(2,2); cout << RPSColor::CYAN() << RPSColor::BOLD()
                        << "|         RPS TOURNAMENT  --  Best of 5           |" << RPSColor::RESET();
        goTo(2,3); cout << RPSColor::CYAN() << RPSColor::BOLD()
                        << "+--------------------------------------------------+" << RPSColor::RESET();
        goTo(2,4); cout << RPSColor::YELLOW() << "  Player: " << playerName
                        << "     HI-SCORE: " << _lb.hiScore() << RPSColor::RESET();

        int playerWins = 0, cpuWins = 0, roundNum = 1;
        int totalScore = 0;
        RPSStats stats;
        bool quit = false;

        while (playerWins < 3 && cpuWins < 3 && roundNum <= 5) {
            int result = playRound(playerWins, cpuWins, roundNum, stats);
            if (result == -99) { quit = true; break; }
            if (result == 1)  { playerWins++; totalScore += 2; }
            else if (result==0) { totalScore += 1; }
            roundNum++;
        }

        if (quit) return;

        // Tournament result
        bool playerWon = playerWins >= 3;
        if (playerWon) totalScore += 5; // bonus

        system("cls");
        goTo(8,3);
        if (playerWon) {
            cout << RPSColor::GREEN() << RPSColor::BOLD()
                 << "*** TOURNAMENT CHAMPION!  +" << 5 << " bonus pts! ***"
                 << RPSColor::RESET();
        } else {
            cout << RPSColor::RED() << RPSColor::BOLD()
                 << "*** CPU wins the tournament. Better luck next time! ***"
                 << RPSColor::RESET();
        }

        goTo(8,5); cout << RPSColor::YELLOW() << "  Final Score : " << RPSColor::GREEN() << totalScore << RPSColor::RESET();
        goTo(8,6); cout << RPSColor::CYAN()   << "  " << stats.summary() << RPSColor::RESET();
        goTo(8,7); cout << RPSColor::WHITE()  << "  You : " << playerWins << "  CPU : " << cpuWins << RPSColor::RESET();

        _lb.addEntry(playerName, totalScore, stats.total());

        goTo(8,9); cout << RPSColor::GRAY() << "  Press any key..." << RPSColor::RESET();
        while(_kbhit())_getch();
        _getch();
    }

    // ── Ask play again ────────────────────────────────────────
    bool askPlayAgain() {
        system("cls"); showCursor();
        goTo(10,8); cout << RPSColor::YELLOW() << RPSColor::BOLD()
                         << "  Play another tournament? (Y/N): " << RPSColor::RESET();
        while(_kbhit())_getch();
        while (true) {
            if (_kbhit()) {
                char c = (char)tolower(_getch());
                hideCursor();
                if (c=='y') return true;
                if (c=='n'||c==27) return false;
            }
            Sleep(30);
        }
    }

    // ── Show leaderboard then goodbye ────────────────────────
    void showLeaderboardAndBye() {
        _lb.draw();
        while(_kbhit())_getch();
        _getch();
        system("cls"); showCursor();
        goTo(15,10); cout << RPSColor::CYAN() << RPSColor::BOLD()
                          << "Thanks for playing RPS Tournament!" << RPSColor::RESET();
        goTo(15,11); cout << RPSColor::GRAY() << "See you next time, champion." << RPSColor::RESET();
        Sleep(2000);
    }
};

#endif // ROCKPAPERSCISSORS_H
