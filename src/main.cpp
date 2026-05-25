#include <SFML/Graphics.hpp>
#include "Hub.h"
#include "TicTacToe.h"
#include "Pong.h"
#include "SnakeLadder.h"
#include "SpaceShooter.h"
#include "Leaderboard.h"

int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Faiq's Gaming Hub",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    // ?? Create leaderboard — loads from file on startup ??????????????????????
    Leaderboard lb(&window);
    lb.LoadFromFile("leaderboard.txt");

    while (window.isOpen()) {
        Hub hub(&window);
        int selectedGame = hub.run();

        if (!window.isOpen()) break;

        // ?? 4 = user pressed [L] — show leaderboard ??????????????????????????
        if (selectedGame == 4) {
            lb.DisplayLeaderboard();
            continue;
        }

        // ?? Launch game, save results to leaderboard after ????????????????????
        switch (selectedGame) {

        case 0: {
            TicTacToe ttt(&window);
            ttt.run();
            if (!window.isOpen()) break;

            char p1[32], p2[32];
            int  p1w = 0, p2w = 0, total = 0;
            ttt.getResult(p1, p1w, p2w, p2, total);

            if (total > 0) {
                if (p1w > 0) lb.AddRecord(p1, "TicTacToe", p1w, total);
                if (p2w > 0) lb.AddRecord(p2, "TicTacToe", p2w, total);
                lb.SaveToFile("leaderboard.txt");
            }
            break;
        }

        case 1: {
            Pong png(&window);
            png.run();
            if (!window.isOpen()) break;

            char p1[32], p2[32];
            int  p1w = 0, p2w = 0, total = 0;
            png.getResult(p1, p1w, p2w, p2, total);

            if (total > 0) {
                if (p1w > 0) lb.AddRecord(p1, "Pong", p1w, total);
                if (p2w > 0) lb.AddRecord(p2, "Pong", p2w, total);
                lb.SaveToFile("leaderboard.txt");
            }
            break;
        }

        case 2: {
            SnakeLadder sl(&window);
            sl.run();
            if (!window.isOpen()) break;

            char p1[32], p2[32];
            int  p1w = 0, p2w = 0, total = 0;
            sl.getResult(p1, p1w, p2w, p2, total);

            if (total > 0) {
                if (p1w > 0) lb.AddRecord(p1, "SnakeLadder", p1w, total);
                if (p2w > 0) lb.AddRecord(p2, "SnakeLadder", p2w, total);
                lb.SaveToFile("leaderboard.txt");
            }
            break;
        }

        case 3: {
            SpaceShooter ss(&window);
            ss.run();
            if (!window.isOpen()) break;

            char p1[32], p2[32];
            int  p1s = 0, p2s = 0, waveReached = 0;
            ss.getResult(p1, p1s, p2, p2s, waveReached);

            // For SpaceShooter use score as "wins" metric, wave as totalGames
            if (waveReached > 0) {
                if (p1s > 0) lb.AddRecord(p1, "SpaceShooter", p1s, waveReached);
                if (p2s > 0) lb.AddRecord(p2, "SpaceShooter", p2s, waveReached);
                lb.SaveToFile("leaderboard.txt");
            }
            break;
        }

        default: break;
        }
    }

    return 0;
}