#ifndef HANGMAN_H
#define HANGMAN_H

// ============================================================
// Hangman.h  -  Group Member 2's Game
// AI Disclosure: Claude (Anthropic) assisted with structure
//   and ASCII art gallows. Logic is student work.
// ============================================================
// OOP: Inheritance, Composition (WordLoader), File I/O
// ============================================================

#include "Game.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>
using namespace std;

// Composition: loads word list from file
class WordLoader {
public:
    vector<string> words;

    WordLoader(const string& filename) {
        ifstream f(filename);
        string w;
        while (f>>w) {
            for (char& c:w) c=toupper(c);
            if ((int)w.size()>=4) words.push_back(w);
        }
        if (words.empty()) {
            // Fallback built-in words
            words={"POLYMORPHISM","INHERITANCE","ENCAPSULATION",
                   "ABSTRACTION","COMPILER","ALGORITHM","RECURSION",
                   "POINTER","TEMPLATE","ITERATOR","OVERLOADING",
                   "CONSTRUCTOR","DESTRUCTOR","VARIABLE","FUNCTION"};
        }
    }

    string randomWord() const {
        return words[rand()%words.size()];
    }
};

// Main game class: derives from Game (Inheritance)
class Hangman : public Game {
public:
    Hangman() : Game("Hangman"), loader("hangman_words.txt"), maxWrong(6) {}

    void play() override {
        cout<<"\n=== Hangman ===\n";
        cout<<"Guess the hidden word letter by letter!\n";
        cout<<loader.words.size()<<" words loaded.\n";
        cout<<string(40,'-')<<"\n";

        char again='y';
        while (again=='y'||again=='Y') {
            playRound();
            cout<<"\nPlay again? (y/n): ";
            cin>>again;
        }
        cout<<"Thanks for playing Hangman!\n";
    }

private:
    WordLoader loader;
    int maxWrong;

    // ASCII gallows — 7 stages
    void drawGallows(int wrong) const {
        const char* art[7][7]={
            {"  +---+","  |   |","      |","      |","      |","      |","========="},
            {"  +---+","  |   |","  O   |","      |","      |","      |","========="},
            {"  +---+","  |   |","  O   |","  |   |","      |","      |","========="},
            {"  +---+","  |   |","  O   |"," /|   |","      |","      |","========="},
            {"  +---+","  |   |","  O   |"," /|\\  |","      |","      |","========="},
            {"  +---+","  |   |","  O   |"," /|\\  |"," /    |","      |","========="},
            {"  +---+","  |   |","  O   |"," /|\\  |"," / \\  |","      |","========="}
        };
        int idx=min(wrong,6);
        cout<<"\n";
        for (int i=0;i<7;i++) cout<<art[idx][i]<<"\n";
    }

    void showWord(const string& word, const vector<char>& guessed) const {
        cout<<"\nWord: ";
        for (char c:word) {
            if (find(guessed.begin(),guessed.end(),c)!=guessed.end())
                cout<<c<<" ";
            else cout<<"_ ";
        }
        cout<<"\n";
    }

    bool solved(const string& word, const vector<char>& guessed) const {
        for (char c:word)
            if (find(guessed.begin(),guessed.end(),c)==guessed.end()) return false;
        return true;
    }

    void playRound() {
        string word = loader.randomWord();
        vector<char> guessed;
        int wrong=0;

        while (wrong<maxWrong && !solved(word,guessed)) {
            drawGallows(wrong);
            showWord(word,guessed);
            cout<<"Guessed: ";
            for (char c:guessed) cout<<c<<" ";
            cout<<"\nWrong left: "<<(maxWrong-wrong)<<"\n";
            cout<<"Guess a letter: ";

            char g; cin>>g; g=toupper(g);
            if (!isalpha(g)) { cout<<"Enter a letter.\n"; continue; }
            if (find(guessed.begin(),guessed.end(),g)!=guessed.end()) {
                cout<<"Already guessed '"<<g<<"'.\n"; continue;
            }
            guessed.push_back(g);
            if (word.find(g)==string::npos) {
                cout<<"'"<<g<<"' is NOT in the word!\n"; wrong++;
            } else {
                cout<<"'"<<g<<"' is in the word!\n";
            }
        }

        drawGallows(wrong);
        showWord(word,guessed);
        if (solved(word,guessed))
            cout<<"\n*** You guessed it: "<<word<<" ***\n";
        else
            cout<<"\n*** Game over! The word was: "<<word<<" ***\n";
    }
};

#endif // HANGMAN_H
