# GameHub Arcade

A Windows console-based C++ game hub featuring four mini-games with polymorphic design and persistent score tracking.

## Overview

`main_gamehub.cpp` launches a menu-driven arcade that lets the player choose from these games:

- **Math Challenge**
  - Adaptive arithmetic quiz with increasing difficulty
  - Saves high scores in `math_scores.txt`
- **Hangman**
  - Classic letter-guessing game
  - Loads words from `hangman_words.txt` if available, otherwise uses built-in defaults
- **Rock Paper Scissors (RPS Tournament)**
  - Best-of-5 tournament rounds against the computer
  - Saves leaderboard scores in `rps_scores.txt`
- **Cosmic Rift**
  - Console space shooter-style game with wave scoring
  - Saves high scores in `cosmic_rift_scores.txt`

The hub also supports a **Surprise Me!** option that randomly selects one of the four games.

## Key Concepts

- Uses a `Game` base class in `Game.h`
- Implements polymorphism via `Game*` pointers and virtual `play()` methods
- Uses Windows console APIs for cursor control, colors, and screen layout
- Includes file-based leaderboard persistence for selected games

## Files

- `main_gamehub.cpp` - launcher and menu system
- `Game.h` - base game interface
- `MathChallenge.h` - math quiz game with leaderboard
- `Hangman.h` - word guessing game with ASCII gallows
- `RockPaperScissors.h` - RPS tournament game with leaderboard
- `CosmicRift.h` - factory declaration for the cosmic game
- `COSMIC-RIFT.cpp` - Cosmic Rift game implementation
- `math_scores.txt` - saved Math Challenge scores
- `rps_scores.txt` - saved RPS leaderboard data
- `cosmic_rift_scores.txt` - saved Cosmic Rift scores

## Build Instructions

Open a terminal in the project folder and compile with g++:

```powershell
cd "c:\Users\HP\Desktop\OOP theory"
g++ -std=c++17 main_gamehub.cpp "COSMIC-RIFT.cpp" -O2 -o main_gamehub.exe
```

> Note: This project is designed for Windows and uses `windows.h` and `conio.h`.

## Run

```powershell
./main_gamehub.exe
```

Select one of the menu options and enjoy the game.

## Notes

- If `hangman_words.txt` is missing, Hangman falls back to a built-in word list.
- Score files are generated automatically when games save results.
- The program uses ANSI escape codes for console colors, enabled via Windows terminal mode.
