#ifndef MATHCHALLENGE_H
#define MATHCHALLENGE_H

// ============================================================
// MathChallenge.h  -  Group Member 1's Game
// AI Disclosure: Claude (Anthropic) assisted with structure.
// ============================================================
// OOP: Inheritance, Composition (MCLeaderboard), File I/O
// ============================================================

#include "Game.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdlib>
using namespace std;

// Composition: leaderboard helper class
class MCLeaderboard {
public:
    static const string FILE_NAME;
    static const int    MAX = 10;

    struct Entry {
        string name; int score;
        bool operator>(const Entry& o) const { return score > o.score; }
        string serialize() const { return name + "," + to_string(score); }
        static Entry deserialize(const string& line) {
            Entry e{"",0};
            size_t p = line.find(','); if (p==string::npos) return e;
            try { e.name=line.substr(0,p); e.score=stoi(line.substr(p+1)); } catch(...) {}
            return e;
        }
    };

    MCLeaderboard() { load(); }

    void addEntry(const string& name, int score) {
        if (score<=0) return;
        string ln=toLower(name);
        for (auto& e:entries) {
            if (toLower(e.name)==ln) {
                if (score>e.score) e.score=score;
                sortAndTrim(); save(); return;
            }
        }
        entries.push_back({name,score});
        sortAndTrim(); save();
    }

    void save() const {
        ofstream f(FILE_NAME); if (!f.is_open()) return;
        f << "# MATH CHALLENGE SCORES\n";
        for (const auto& e:entries) f<<e.serialize()<<"\n";
    }
    void load() {
        entries.clear(); ifstream f(FILE_NAME); if (!f.is_open()) return;
        string line;
        while (getline(f,line)) {
            if (line.empty()||line[0]=='#') continue;
            auto e=Entry::deserialize(line);
            if (!e.name.empty()) entries.push_back(e);
        }
        sortAndTrim();
    }

    void print() const {
        cout<<"\n--- Leaderboard (Top 5) ---\n";
        if (entries.empty()) { cout<<"  No entries yet.\n"; return; }
        int lim=min((int)entries.size(),5);
        for (int i=0;i<lim;i++)
            cout<<"  "<<i+1<<". "<<entries[i].name<<" - "<<entries[i].score<<" pts\n";
        cout<<string(28,'-')<<"\n";
    }

    vector<Entry> entries;
private:
    static string toLower(const string& s) {
        string r=s; transform(r.begin(),r.end(),r.begin(),::tolower); return r;
    }
    void sortAndTrim() {
        sort(entries.begin(),entries.end(),[](const Entry&a,const Entry&b){return a>b;});
        if ((int)entries.size()>MAX) entries.resize(MAX);
    }
};
const string MCLeaderboard::FILE_NAME = "math_scores.txt";

// Main game class: derives from Game (Inheritance)
class MathChallenge : public Game {
public:
    MathChallenge() : Game("Math Challenge"), score(0), answered(0) {}

    void play() override {
        score = 0;
        answered = 0;
        cout << "\n=== Math Challenge ===\n";
        cout << "Answer arithmetic questions!\n";
        cout << "Difficulty increases as your score rises.\n";
        cout << "Type -999 to quit.\n";
        cout << string(40, '-') << "\n";
        cout << "Enter your name: ";
        string name; cin >> name;

        char cont = 'y';
        while (cont == 'y' || cont == 'Y') {
            int result = askQuestion();
            if (result == -1) break;          // quit signal
            if (result == 1) score++;
            answered++;
            cout << "Score: " << score << "/" << answered << "  | Continue? (y/n): ";
            cin >> cont;
        }

        cout << "\n=== Final Score: " << score << "/" << answered << " ===\n";
        _lb.addEntry(name, score);
        _lb.print();
    }

private:
    int score, answered;
    MCLeaderboard _lb;

    int askQuestion() {
        // Difficulty: range grows every 5 correct answers
        int maxVal = min(10 + (score / 5) * 5, 50);
        int a = rand() % maxVal + 1;
        int b = rand() % maxVal + 1;
        int op = rand() % 4;
        int answer;

        if (op == 0) {
            answer = a + b;
            cout << "\n" << a << " + " << b << " = ? ";
        } else if (op == 1) {
            answer = a - b;
            cout << "\n" << a << " - " << b << " = ? ";
        } else if (op == 2) {
            a = rand() % 12 + 1;
            b = rand() % 12 + 1;
            answer = a * b;
            cout << "\n" << a << " x " << b << " = ? ";
        } else {
            b = rand() % 9 + 1;
            a = b * (rand() % 10 + 1);
            answer = a / b;
            cout << "\n" << a << " / " << b << " = ? ";
        }

        int userAns;
        if (!(cin >> userAns)) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "  Invalid input. Please enter a number.\n";
            return 0;
        }
        if (userAns == -999) {
            cout << "  Quitting Math Challenge.\n";
            return -1;
        }
        if (userAns == answer) {
            cout << "  Correct! +1\n";
            return 1;
        }
        cout << "  Wrong! Answer was " << answer << "\n";
        return 0;
    }
};

#endif // MATHCHALLENGE_H
