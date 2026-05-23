// ============================================================
// main_gamehub.cpp  -  GameHub Launcher
// AI Disclosure: Claude (Anthropic) assisted with structure.
// ============================================================
// Polymorphism: all games stored as Game* base pointers.
// Option 0 randomly picks a game via virtual dispatch.
// ============================================================

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

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "Game.h"
#include "MathChallenge.h"
#include "Hangman.h"
#include "RockPaperScissors.h"
#include "CosmicRift.h"

using namespace std;

#define CY  "\033[96m"
#define YL  "\033[93m"
#define GR  "\033[92m"
#define WH  "\033[97m"
#define GY  "\033[90m"
#define RD  "\033[91m"
#define MG  "\033[95m"
#define BD  "\033[1m"
#define RS  "\033[0m"

static void enableVT() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m=0; GetConsoleMode(h,&m);
    SetConsoleMode(h,m|ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

static void goTo(int x,int y) {
    COORD c={(SHORT)x,(SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),c);
}

void showMenu(const vector<Game*>& games) {
    system("cls");
    system("mode con: cols=62 lines=26");
    HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci={(DWORD)1,FALSE}; SetConsoleCursorInfo(h,&ci);

    goTo(2,1);  cout<<CY<<BD<<"+----------------------------------------------------+"<<RS;
    goTo(2,2);  cout<<YL<<BD<<"|         G A M E H U B   A R C A D E              |"<<RS;
    goTo(2,3);  cout<<CY<<BD<<"+----------------------------------------------------+"<<RS;
    goTo(2,5);  cout<<WH<<BD<<"  SELECT A GAME:"<<RS;
    goTo(2,6);  cout<<GY<<"  --------------------------------------------------"<<RS;
    goTo(2,7);  cout<<MG<<BD<<"    0.  Surprise Me!  (random pick from all 4 games)"<<RS;
    goTo(2,8);  cout<<CY<<BD<<"    1.  "<<games[0]->getName()<<""<<RS;
    goTo(2,9);  cout<<GR<<BD<<"    2.  "<<games[1]->getName()<<""<<RS;
    goTo(2,10); cout<<YL<<BD<<"    3.  "<<games[2]->getName()<<""<<RS;
    goTo(2,11); cout<<MG<<BD<<"    4.  "<<games[3]->getName()<<""<<RS;

    int qrow = 9+(int)games.size();
    goTo(2,qrow);   cout<<GY<<"  --------------------------------------------------"<<RS;
    goTo(2,qrow+1); cout<<RD<<"    "<<(games.size()+1)<<".  Quit"<<RS;
    goTo(2,qrow+3); cout<<WH<<"  Choice: "<<RS;

    CONSOLE_CURSOR_INFO ci2={(DWORD)10,TRUE}; SetConsoleCursorInfo(h,&ci2);
}

int main() {
    srand((unsigned)time(nullptr));
    enableVT();

    // Polymorphism: all games as base class pointers
    // Option 4 is Cosmic Rift and it is included in the random picker.
    vector<Game*> games;
    games.push_back(new MathChallenge());
    games.push_back(new Hangman());
    games.push_back(new RockPaperScissors());
    // CosmicRift implementation lives in COSMIC-RIFT.cpp; use factory
    games.push_back(createCosmicRift());

    int quitOpt = (int)games.size()+1;

    while (true) {
        showMenu(games);
        int choice=-1;
        cin>>choice;
        if (cin.fail()) { cin.clear(); cin.ignore(1000,'\n'); continue; }

        if (choice==quitOpt) {
            system("cls");
            goTo(10,10); cout<<CY<<BD<<"Thanks for playing GameHub! Goodbye."<<RS<<"\n\n";
            Sleep(1500); break;
        }
        else if (choice==0) {
            int idx=rand()%(int)games.size();
            system("cls");
            goTo(8,8);
            cout<<YL<<BD<<"[ Random Pick ] --> "<<RS<<CY<<BD<<games[idx]->getName()<<RS<<"\n";
            Sleep(1200);
            games[idx]->play();  // virtual dispatch
        }
        else if (choice>=1 && choice<=(int)games.size()) {
            games[choice-1]->play();  // virtual dispatch
        }
        else {
            goTo(2,20); cout<<RD<<"  Invalid choice."<<RS; Sleep(800);
        }
        system("mode con: cols=62 lines=26");
    }

    for (Game* g:games) delete g;
    return 0;
}
