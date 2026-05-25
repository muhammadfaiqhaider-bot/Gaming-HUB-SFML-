#include "TicTacToe.h"
#include <cmath>

// ??? No-library string helpers ????????????????????????????????????????????????
static int myLen(const char* s) { int i = 0; while (s[i]) i++; return i; }
static void myCpy(char* d, const char* s) { int i = 0; while ((d[i] = s[i])) i++; }
static void myCat(char* d, const char* s) {
    int i = myLen(d), j = 0; while ((d[i++] = s[j++]));
}
static void intToStr(int v, char* buf) {
    if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12]; int i = 0;
    while (v > 0) { tmp[i++] = '0' + v % 10; v /= 10; }
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

// ??? Constructor ??????????????????????????????????????????????????????????????
TicTacToe::TicTacToe(sf::RenderWindow* win)
    : window(win), glowTimer(0.f), welcomeTimer(0.f),
    state(TTTState::WELCOME),
    currentPlayer(1), winner(0), totalGames(0),
    hasWinLine(false), inputLen(0),
    p1Wins(0), p2Wins(0), draws(0)
{
    neonCyan = sf::Color(0, 240, 255);
    neonPink = sf::Color(255, 30, 180);
    neonYellow = sf::Color(255, 220, 0);
    neonGreen = sf::Color(0, 255, 120);
    darkBg = sf::Color(6, 6, 20);

    myCpy(p1Name, "PLAYER 1");
    myCpy(p2Name, "PLAYER 2");
    inputBuffer[0] = '\0';

    boardX = 300.f;
    boardY = 185.f;
    cellSize = 175.f;

    font.loadFromFile("Orbitron-VariableFont_wght.ttf");
    initScanlines();
    initGrid();
    resetBoard();

    // Load sound effects
    if (bufX.loadFromFile("place_x.ogg"))   sndX.setBuffer(bufX);
    if (bufO.loadFromFile("place_o.ogg"))   sndO.setBuffer(bufO);
    if (bufWin.loadFromFile("win.ogg"))     sndWin.setBuffer(bufWin);
    sndX.setVolume(80.f);
    sndO.setVolume(80.f);
    sndWin.setVolume(90.f);
}

// ??? Scanlines ????????????????????????????????????????????????????????????????
void TicTacToe::initScanlines() {
    for (int i = 0; i < 120; i++) {
        scanlines[i].setSize(sf::Vector2f(1200.f, 1.f));
        scanlines[i].setPosition(0.f, i * 7.5f);
        scanlines[i].setFillColor(sf::Color(0, 0, 0, 18));
    }
}

// ??? Grid ?????????????????????????????????????????????????????????????????????
void TicTacToe::initGrid() {
    float total = cellSize * 3.f;
    sf::Color gc(0, 240, 255, 150);
    gridLines[0].setSize({ 2.f, total });
    gridLines[0].setPosition(boardX + cellSize, boardY);
    gridLines[1].setSize({ 2.f, total });
    gridLines[1].setPosition(boardX + cellSize * 2.f, boardY);
    gridLines[2].setSize({ total, 2.f });
    gridLines[2].setPosition(boardX, boardY + cellSize);
    gridLines[3].setSize({ total, 2.f });
    gridLines[3].setPosition(boardX, boardY + cellSize * 2.f);
    for (int i = 0; i < 4; i++) gridLines[i].setFillColor(gc);
}

// ??? Reset board ??????????????????????????????????????????????????????????????
void TicTacToe::resetBoard() {
    for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) board[r][c] = Cell::EMPTY;
    currentPlayer = 1;
    winner = 0;
    hasWinLine = false;
}

// ??? Background ???????????????????????????????????????????????????????????????
void TicTacToe::drawBackground() {
    window->clear(darkBg);
    sf::CircleShape glow(400.f);
    glow.setFillColor(sf::Color(0, 40, 90, 18));
    glow.setOrigin(400.f, 400.f);
    glow.setPosition(600.f, 480.f);
    window->draw(glow);

    sf::CircleShape dot(1.f);
    dot.setFillColor(sf::Color(255, 255, 255, 11));
    for (int x = 30; x < 1200; x += 45)
        for (int y = 30; y < 900; y += 45) {
            dot.setPosition((float)x, (float)y);
            window->draw(dot);
        }
    for (int i = 0; i < 120; i++) window->draw(scanlines[i]);
}

// ??? Centered text helper ?????????????????????????????????????????????????????
void TicTacToe::centeredText(const char* str, float y, unsigned size,
    sf::Color col, bool bold) {
    sf::Text t;
    t.setFont(font);
    t.setString(str);
    t.setCharacterSize(size);
    t.setFillColor(col);
    if (bold) t.setStyle(sf::Text::Bold);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.width / 2.f, b.height / 2.f);
    t.setPosition(600.f, y);
    window->draw(t);
}

// ??? Score bar ????????????????????????????????????????????????????????????????
void TicTacToe::drawScoreBar() {
    // Background strip
    sf::RectangleShape bar(sf::Vector2f(1200.f, 58.f));
    bar.setFillColor(sf::Color(10, 10, 35, 220));
    bar.setPosition(0.f, 0.f);
    window->draw(bar);

    sf::RectangleShape line(sf::Vector2f(1200.f, 2.f));
    line.setFillColor(sf::Color(0, 240, 255, 80));
    line.setPosition(0.f, 58.f);
    window->draw(line);

    // P1 name + wins (left)
    char buf[64];
    myCpy(buf, p1Name); myCat(buf, "  [X]");
    sf::Text t1; t1.setFont(font); t1.setCharacterSize(16);
    t1.setFillColor(neonCyan); t1.setStyle(sf::Text::Bold);
    t1.setString(buf);
    t1.setPosition(30.f, 14.f);
    window->draw(t1);

    // P1 score box
    char s1[8]; intToStr(p1Wins, s1);
    sf::RectangleShape box1(sf::Vector2f(50.f, 34.f));
    box1.setFillColor(sf::Color(0, 240, 255, 30));
    box1.setOutlineThickness(2.f);
    box1.setOutlineColor(neonCyan);
    box1.setPosition(300.f, 12.f);
    window->draw(box1);
    sf::Text sc1; sc1.setFont(font); sc1.setCharacterSize(20);
    sc1.setFillColor(neonCyan); sc1.setStyle(sf::Text::Bold);
    sc1.setString(s1);
    sf::FloatRect f1 = sc1.getLocalBounds();
    sc1.setOrigin(f1.width / 2.f, f1.height / 2.f);
    sc1.setPosition(325.f, 28.f);
    window->draw(sc1);

    // VS in center
    sf::Text vs; vs.setFont(font); vs.setCharacterSize(14);
    vs.setFillColor(sf::Color(160, 160, 200, 180));
    vs.setString("VS");
    sf::FloatRect vb = vs.getLocalBounds();
    vs.setOrigin(vb.width / 2.f, vb.height / 2.f);
    vs.setPosition(600.f, 28.f);
    window->draw(vs);

    // Draws counter (small, below VS)
    char dbuf[32]; myCpy(dbuf, "DRAWS: "); char ds[8]; intToStr(draws, ds); myCat(dbuf, ds);
    sf::Text dt; dt.setFont(font); dt.setCharacterSize(11);
    dt.setFillColor(sf::Color(140, 140, 180, 160));
    dt.setString(dbuf);
    sf::FloatRect dbb = dt.getLocalBounds();
    dt.setOrigin(dbb.width / 2.f, dbb.height / 2.f);
    dt.setPosition(600.f, 46.f);
    window->draw(dt);

    // P2 score box
    char s2[8]; intToStr(p2Wins, s2);
    sf::RectangleShape box2(sf::Vector2f(50.f, 34.f));
    box2.setFillColor(sf::Color(255, 30, 180, 30));
    box2.setOutlineThickness(2.f);
    box2.setOutlineColor(neonPink);
    box2.setPosition(850.f, 12.f);
    window->draw(box2);
    sf::Text sc2; sc2.setFont(font); sc2.setCharacterSize(20);
    sc2.setFillColor(neonPink); sc2.setStyle(sf::Text::Bold);
    sc2.setString(s2);
    sf::FloatRect f2 = sc2.getLocalBounds();
    sc2.setOrigin(f2.width / 2.f, f2.height / 2.f);
    sc2.setPosition(875.f, 28.f);
    window->draw(sc2);

    // P2 name + tag (right)
    char buf2[64]; myCpy(buf2, "[O]  "); myCat(buf2, p2Name);
    sf::Text t2; t2.setFont(font); t2.setCharacterSize(16);
    t2.setFillColor(neonPink); t2.setStyle(sf::Text::Bold);
    t2.setString(buf2);
    sf::FloatRect tb2 = t2.getLocalBounds();
    t2.setOrigin(tb2.width, tb2.height / 2.f);
    t2.setPosition(1170.f, 28.f);
    window->draw(t2);
}

// ??? Input box helper ?????????????????????????????????????????????????????????
void TicTacToe::drawInputBox(const char* prompt, sf::Color promptColor,
    const char* subInfo) {
    // Card panel
    sf::RectangleShape panel(sf::Vector2f(640.f, subInfo ? 280.f : 240.f));
    panel.setFillColor(sf::Color(10, 10, 32, 235));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(promptColor);
    panel.setOrigin(320.f, panel.getSize().y / 2.f);
    panel.setPosition(600.f, 480.f);
    window->draw(panel);

    // Prompt
    centeredText(prompt, 390.f, 20, promptColor, true);

    // Input box
    sf::RectangleShape ibox(sf::Vector2f(480.f, 54.f));
    ibox.setFillColor(sf::Color(20, 20, 55, 210));
    ibox.setOutlineThickness(2.f);
    ibox.setOutlineColor(promptColor);
    ibox.setOrigin(240.f, 27.f);
    ibox.setPosition(600.f, 460.f);
    window->draw(ibox);

    // Typed text + blinking cursor
    char display[36]; myCpy(display, inputBuffer);
    float pulse = (std::sin(glowTimer * 4.f) + 1.f) / 2.f;
    if (pulse > 0.5f) myCat(display, "|");
    sf::Text it; it.setFont(font); it.setCharacterSize(26);
    it.setFillColor(sf::Color::White);
    it.setString(display);
    sf::FloatRect ib = it.getLocalBounds();
    it.setOrigin(ib.width / 2.f, ib.height / 2.f);
    it.setPosition(600.f, 458.f);
    window->draw(it);

    // Hint
    centeredText("PRESS ENTER TO CONFIRM", 510.f, 13, sf::Color(130, 130, 170));

    // Sub info (already confirmed player)
    if (subInfo)
        centeredText(subInfo, 550.f, 15, sf::Color(0, 240, 255, 200));
}

// ??? Draw X / O ???????????????????????????????????????????????????????????????
void TicTacToe::drawXAt(float cx, float cy, float size, sf::Color col) {
    float arm = size / 2.f - 22.f;
    float thick = 9.f;
    sf::RectangleShape l1(sf::Vector2f(arm * 2.f * 1.414f, thick));
    l1.setFillColor(col);
    l1.setOrigin(l1.getSize().x / 2.f, thick / 2.f);
    l1.setPosition(cx, cy); l1.setRotation(45.f);
    window->draw(l1);
    sf::RectangleShape l2(sf::Vector2f(arm * 2.f * 1.414f, thick));
    l2.setFillColor(col);
    l2.setOrigin(l2.getSize().x / 2.f, thick / 2.f);
    l2.setPosition(cx, cy); l2.setRotation(-45.f);
    window->draw(l2);
}
void TicTacToe::drawOAt(float cx, float cy, float radius, sf::Color col) {
    sf::CircleShape c(radius);
    c.setFillColor(sf::Color::Transparent);
    c.setOutlineThickness(9.f);
    c.setOutlineColor(col);
    c.setOrigin(radius, radius);
    c.setPosition(cx, cy);
    window->draw(c);
}

// ??? Draw grid + board ????????????????????????????????????????????????????????
void TicTacToe::drawGrid() {
    for (int i = 0; i < 4; i++) window->draw(gridLines[i]);
}
void TicTacToe::drawBoard() {
    for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) {
        float cx = boardX + c * cellSize + cellSize / 2.f;
        float cy = boardY + r * cellSize + cellSize / 2.f;
        if (board[r][c] == Cell::X) drawXAt(cx, cy, cellSize, neonCyan);
        else if (board[r][c] == Cell::O) drawOAt(cx, cy, cellSize / 2.f - 26.f, neonPink);
    }
    if (hasWinLine) {
        float pulse = (std::sin(glowTimer * 4.f) + 1.f) / 2.f;
        winLineShape.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(160 + 95 * pulse)));
        window->draw(winLineShape);
    }
}

// ??? Check winner ?????????????????????????????????????????????????????????????
void TicTacToe::checkWinner() {
    auto mark = [&](int r0, int c0, int r1, int c1, int r2, int c2, Cell who) {
        winner = (who == Cell::X) ? 1 : 2;
        winLine[0][0] = r0; winLine[0][1] = c0;
        winLine[1][0] = r1; winLine[1][1] = c1;
        winLine[2][0] = r2; winLine[2][1] = c2;
        hasWinLine = true; computeWinLine();
        };
    for (int r = 0; r < 3; r++)
        if (board[r][0] != Cell::EMPTY && board[r][0] == board[r][1] && board[r][1] == board[r][2])
        {
            mark(r, 0, r, 1, r, 2, board[r][0]); return;
        }
    for (int c = 0; c < 3; c++)
        if (board[0][c] != Cell::EMPTY && board[0][c] == board[1][c] && board[1][c] == board[2][c])
        {
            mark(0, c, 1, c, 2, c, board[0][c]); return;
        }
    if (board[0][0] != Cell::EMPTY && board[0][0] == board[1][1] && board[1][1] == board[2][2])
    {
        mark(0, 0, 1, 1, 2, 2, board[0][0]); return;
    }
    if (board[0][2] != Cell::EMPTY && board[0][2] == board[1][1] && board[1][1] == board[2][0])
    {
        mark(0, 2, 1, 1, 2, 0, board[0][2]); return;
    }
    bool full = true;
    for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) if (board[r][c] == Cell::EMPTY) full = false;
    if (full) winner = 3;
}

void TicTacToe::computeWinLine() {
    int r0 = winLine[0][0], c0 = winLine[0][1], r2 = winLine[2][0], c2 = winLine[2][1];
    float x0 = boardX + c0 * cellSize + cellSize / 2.f, y0 = boardY + r0 * cellSize + cellSize / 2.f;
    float x2 = boardX + c2 * cellSize + cellSize / 2.f, y2 = boardY + r2 * cellSize + cellSize / 2.f;
    float dx = x2 - x0, dy = y2 - y0;
    float len = std::sqrt(dx * dx + dy * dy) + 50.f;
    float angle = std::atan2(dy, dx) * 180.f / 3.14159f;
    winLineShape.setSize({ len,7.f });
    winLineShape.setOrigin(len / 2.f, 3.5f);
    winLineShape.setPosition((x0 + x2) / 2.f, (y0 + y2) / 2.f);
    winLineShape.setRotation(angle);
    winLineShape.setFillColor(neonYellow);
}

sf::Vector2i TicTacToe::getBoardCell(sf::Vector2i mp) {
    float fx = (float)mp.x - boardX, fy = (float)mp.y - boardY;
    int col = (int)(fx / cellSize), row = (int)(fy / cellSize);
    if (col < 0 || col>2 || row < 0 || row>2) return { -1,-1 };
    return { col,row };
}

// ??????????????????????????????????????????????????????????????????????????????
//  SCREEN: WELCOME
// ??????????????????????????????????????????????????????????????????????????????
void TicTacToe::drawWelcome() {
    float pulse = (std::sin(glowTimer * 1.8f) + 1.f) / 2.f;

    // Big title
    sf::Uint8 r = (sf::Uint8)(pulse * 80);
    centeredText("TIC TAC TOE", 280.f, 62, sf::Color(r, 240, 255), true);

    // Decorative X and O either side
    drawXAt(200.f, 450.f, 160.f, sf::Color(0, 240, 255, 100));
    drawOAt(1000.f, 450.f, 70.f, sf::Color(255, 30, 180, 100));

    // Underline
    float uw = 400.f + 80.f * pulse;
    sf::RectangleShape ul(sf::Vector2f(uw, 3.f));
    ul.setFillColor(sf::Color(0, 240, 255, (sf::Uint8)(160 + 95 * pulse)));
    ul.setOrigin(uw / 2.f, 1.5f);
    ul.setPosition(600.f, 360.f);
    window->draw(ul);

    centeredText("WELCOME TO THE ULTIMATE SHOWDOWN", 420.f, 18,
        sf::Color(180, 180, 220, 200));

    // Pulsing start prompt
    sf::Uint8 alpha = (sf::Uint8)(140 + 115 * pulse);
    centeredText("PRESS  ENTER  TO  START", 550.f, 22,
        sf::Color(255, 220, 0, alpha), true);

    centeredText("[ ESC ]  BACK TO HUB", 840.f, 13, sf::Color(100, 100, 140));
}

void TicTacToe::handleWelcome(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Enter) {
            state = TTTState::ENTER_P1;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        if (e.key.code == sf::Keyboard::Escape) window; // handled in run()
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  SCREEN: ENTER NAME
// ??????????????????????????????????????????????????????????????????????????????
void TicTacToe::drawEnterName(int player) {
    centeredText("TIC TAC TOE", 100.f, 36, neonCyan, true);

    if (player == 1) {
        centeredText("PLAYER 1", 220.f, 52, neonCyan, true);
        centeredText("You will play as  X", 290.f, 17, sf::Color(160, 160, 210));
        drawInputBox("ENTER YOUR NAME:", neonCyan);
    }
    else {
        centeredText("PLAYER 2", 220.f, 52, neonPink, true);
        centeredText("You will play as  O", 290.f, 17, sf::Color(160, 160, 210));
        char sub[48]; myCpy(sub, "P1: "); myCat(sub, p1Name); myCat(sub, " is ready!");
        drawInputBox("ENTER YOUR NAME:", neonPink, sub);
    }
}

void TicTacToe::handleEnterP1(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            myCpy(p1Name, inputBuffer);
            for (int i = 0; p1Name[i]; i++)
                if (p1Name[i] >= 'a' && p1Name[i] <= 'z') p1Name[i] -= 32;
            state = TTTState::ENTER_P2;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}
void TicTacToe::handleEnterP2(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            myCpy(p2Name, inputBuffer);
            for (int i = 0; p2Name[i]; i++)
                if (p2Name[i] >= 'a' && p2Name[i] <= 'z') p2Name[i] -= 32;
            p1Wins = 0; p2Wins = 0; draws = 0; totalGames = 0;
            resetBoard();
            state = TTTState::PLAYING;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  SCREEN: PLAYING
// ??????????????????????????????????????????????????????????????????????????????
void TicTacToe::drawPlaying() {
    drawScoreBar();
    drawGrid();
    drawBoard();

    // Turn indicator below score bar
    char turnBuf[64];
    myCpy(turnBuf, (currentPlayer == 1) ? p1Name : p2Name);
    myCat(turnBuf, (currentPlayer == 1) ? "'S TURN  [ X ]" : "'S TURN  [ O ]");
    sf::Color tc = (currentPlayer == 1) ? neonCyan : neonPink;
    centeredText(turnBuf, 100.f, 18, tc, true);

    // ESC hint
    centeredText("[ ESC ]  BACK TO HUB", 870.f, 13, sf::Color(100, 100, 140));
}

void TicTacToe::handlePlaying(sf::Event& e) {
    if (e.type == sf::Event::MouseButtonPressed &&
        e.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i cell = getBoardCell(sf::Mouse::getPosition(*window));
        if (cell.x >= 0 && cell.y >= 0 && board[cell.y][cell.x] == Cell::EMPTY) {
            board[cell.y][cell.x] = (currentPlayer == 1) ? Cell::X : Cell::O;
            // Play place sound
            if (currentPlayer == 1) sndX.play();
            else                 sndO.play();
            checkWinner();
            if (winner != 0) {
                totalGames++;
                if (winner == 1) p1Wins++;
                else if (winner == 2) p2Wins++;
                else draws++;
                // Play win sound (not for draw)
                if (winner != 3) sndWin.play();
                state = TTTState::ROUND_OVER;
            }
            else {
                currentPlayer = (currentPlayer == 1) ? 2 : 1;
            }
        }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  SCREEN: ROUND OVER
// ??????????????????????????????????????????????????????????????????????????????
void TicTacToe::drawRoundOver() {
    drawScoreBar();
    drawGrid();
    drawBoard();

    // Dark overlay
    sf::RectangleShape ov(sf::Vector2f(1200.f, 900.f));
    ov.setFillColor(sf::Color(0, 0, 0, 140));
    window->draw(ov);

    // Result card
    sf::RectangleShape card(sf::Vector2f(620.f, 290.f));
    card.setFillColor(sf::Color(8, 8, 28, 245));
    card.setOutlineThickness(3.f);
    card.setOutlineColor(neonYellow);
    card.setOrigin(310.f, 145.f);
    card.setPosition(600.f, 450.f);
    window->draw(card);

    // Round result
    char resBuf[64];
    if (winner == 3) {
        myCpy(resBuf, "IT'S A DRAW!");
        centeredText(resBuf, 350.f, 30, sf::Color(200, 200, 100), true);
    }
    else {
        myCpy(resBuf, (winner == 1) ? p1Name : p2Name);
        myCat(resBuf, " WINS!");
        centeredText(resBuf, 350.f, 30, neonYellow, true);
    }

    // Games played
    char gpBuf[48]; myCpy(gpBuf, "GAME #"); char gn[8]; intToStr(totalGames, gn);
    myCat(gpBuf, gn); myCat(gpBuf, " COMPLETE");
    centeredText(gpBuf, 400.f, 15, sf::Color(160, 160, 200));

    // Options
    centeredText("[ ENTER ]  PLAY NEXT ROUND", 460.f, 19, neonGreen, true);
    centeredText("[ F ]  FINISH SESSION  &  SEE RESULTS", 510.f, 19,
        sf::Color(255, 120, 80), true);
    centeredText("[ ESC ]  BACK TO HUB", 565.f, 13, sf::Color(100, 100, 140));
}

void TicTacToe::handleRoundOver(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Enter) {
            resetBoard();
            state = TTTState::PLAYING;
        }
        if (e.key.code == sf::Keyboard::F)
            state = TTTState::FINAL_RESULTS;
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  SCREEN: FINAL RESULTS
// ??????????????????????????????????????????????????????????????????????????????
void TicTacToe::drawFinalResults() {
    // Trophy glow
    float pulse = (std::sin(glowTimer * 2.f) + 1.f) / 2.f;
    sf::CircleShape glow(260.f);
    glow.setFillColor(sf::Color(255, 200, 0, (sf::Uint8)(12 + 10 * pulse)));
    glow.setOrigin(260.f, 260.f);
    glow.setPosition(600.f, 400.f);
    window->draw(glow);

    centeredText("FINAL RESULTS", 100.f, 48, neonYellow, true);

    // Underline
    float uw = 380.f + 40.f * pulse;
    sf::RectangleShape ul(sf::Vector2f(uw, 3.f));
    ul.setFillColor(neonYellow);
    ul.setOrigin(uw / 2.f, 1.5f);
    ul.setPosition(600.f, 152.f);
    window->draw(ul);

    // Total games
    char tgBuf[48]; myCpy(tgBuf, "TOTAL GAMES PLAYED: ");
    char tgn[8]; intToStr(totalGames, tgn); myCat(tgBuf, tgn);
    centeredText(tgBuf, 195.f, 16, sf::Color(160, 160, 200));

    // P1 result card
    sf::RectangleShape c1(sf::Vector2f(380.f, 160.f));
    c1.setFillColor(sf::Color(0, 240, 255, 18));
    c1.setOutlineThickness(2.f);
    c1.setOutlineColor(neonCyan);
    c1.setOrigin(190.f, 80.f);
    c1.setPosition(340.f, 360.f);
    window->draw(c1);
    // P1 card text
    sf::Text n1; n1.setFont(font); n1.setCharacterSize(22);
    n1.setFillColor(neonCyan); n1.setStyle(sf::Text::Bold);
    n1.setString(p1Name);
    sf::FloatRect nb1 = n1.getLocalBounds();
    n1.setOrigin(nb1.width / 2.f, nb1.height / 2.f);
    n1.setPosition(340.f, 300.f); window->draw(n1);

    char w1[8]; intToStr(p1Wins, w1);
    sf::Text wt1; wt1.setFont(font); wt1.setCharacterSize(56);
    wt1.setFillColor(neonCyan); wt1.setStyle(sf::Text::Bold);
    wt1.setString(w1);
    sf::FloatRect wb1 = wt1.getLocalBounds();
    wt1.setOrigin(wb1.width / 2.f, wb1.height / 2.f);
    wt1.setPosition(340.f, 370.f); window->draw(wt1);
    sf::Text wl1; wl1.setFont(font); wl1.setCharacterSize(14);
    wl1.setFillColor(sf::Color(0, 200, 220, 200));
    wl1.setString("WINS"); sf::FloatRect wlb = wl1.getLocalBounds();
    wl1.setOrigin(wlb.width / 2.f, wlb.height / 2.f);
    wl1.setPosition(340.f, 430.f); window->draw(wl1);

    // P2 result card
    sf::RectangleShape c2(sf::Vector2f(380.f, 160.f));
    c2.setFillColor(sf::Color(255, 30, 180, 18));
    c2.setOutlineThickness(2.f);
    c2.setOutlineColor(neonPink);
    c2.setOrigin(190.f, 80.f);
    c2.setPosition(860.f, 360.f);
    window->draw(c2);

    sf::Text n2; n2.setFont(font); n2.setCharacterSize(22);
    n2.setFillColor(neonPink); n2.setStyle(sf::Text::Bold);
    n2.setString(p2Name);
    sf::FloatRect nb2 = n2.getLocalBounds();
    n2.setOrigin(nb2.width / 2.f, nb2.height / 2.f);
    n2.setPosition(860.f, 300.f); window->draw(n2);

    char w2[8]; intToStr(p2Wins, w2);
    sf::Text wt2; wt2.setFont(font); wt2.setCharacterSize(56);
    wt2.setFillColor(neonPink); wt2.setStyle(sf::Text::Bold);
    wt2.setString(w2);
    sf::FloatRect wb2 = wt2.getLocalBounds();
    wt2.setOrigin(wb2.width / 2.f, wb2.height / 2.f);
    wt2.setPosition(860.f, 370.f); window->draw(wt2);
    sf::Text wl2; wl2.setFont(font); wl2.setCharacterSize(14);
    wl2.setFillColor(sf::Color(220, 0, 160, 200));
    wl2.setString("WINS"); sf::FloatRect wl2b = wl2.getLocalBounds();
    wl2.setOrigin(wl2b.width / 2.f, wl2b.height / 2.f);
    wl2.setPosition(860.f, 430.f); window->draw(wl2);

    // Draws
    char dbuf[32]; myCpy(dbuf, "DRAWS:  "); char dn[8]; intToStr(draws, dn); myCat(dbuf, dn);
    centeredText(dbuf, 380.f, 18, sf::Color(200, 200, 100));

    // Overall winner banner
    char bannerBuf[64];
    if (p1Wins > p2Wins) {
        myCpy(bannerBuf, p1Name); myCat(bannerBuf, " IS THE CHAMPION!");
        centeredText(bannerBuf, 530.f, 26, neonYellow, true);
    }
    else if (p2Wins > p1Wins) {
        myCpy(bannerBuf, p2Name); myCat(bannerBuf, " IS THE CHAMPION!");
        centeredText(bannerBuf, 530.f, 26, neonYellow, true);
    }
    else {
        centeredText("IT'S AN EVEN MATCH!", 530.f, 26,
            sf::Color(200, 200, 100), true);
    }

    centeredText("[ ENTER ]  PLAY AGAIN  (NEW SESSION)",
        640.f, 17, neonGreen);
    centeredText("[ ESC ]  BACK TO HUB",
        690.f, 14, sf::Color(100, 100, 140));
}

void TicTacToe::handleFinalResults(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Enter) {
            // New session — re-enter names
            p1Wins = 0; p2Wins = 0; draws = 0; totalGames = 0;
            inputBuffer[0] = '\0'; inputLen = 0;
            state = TTTState::ENTER_P1;
        }
    }
}

// ??? Main Run Loop ????????????????????????????????????????????????????????????
void TicTacToe::run() {
    sf::Clock clock;

    while (window->isOpen()) {
        float dt = clock.restart().asSeconds();
        glowTimer += dt;
        welcomeTimer += dt;

        sf::Event event;
        while (window->pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window->close();

            // Global ESC ? back to hub
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape &&
                state != TTTState::PLAYING)
                return;

            switch (state) {
            case TTTState::WELCOME:       handleWelcome(event);      break;
            case TTTState::ENTER_P1:      handleEnterP1(event);      break;
            case TTTState::ENTER_P2:      handleEnterP2(event);      break;
            case TTTState::PLAYING:       handlePlaying(event);      break;
            case TTTState::ROUND_OVER:    handleRoundOver(event);    break;
            case TTTState::FINAL_RESULTS: handleFinalResults(event); break;
            }
        }

        drawBackground();

        switch (state) {
        case TTTState::WELCOME:       drawWelcome();         break;
        case TTTState::ENTER_P1:      drawEnterName(1);      break;
        case TTTState::ENTER_P2:      drawEnterName(2);      break;
        case TTTState::PLAYING:       drawPlaying();         break;
        case TTTState::ROUND_OVER:    drawRoundOver();       break;
        case TTTState::FINAL_RESULTS: drawFinalResults();    break;
        }

        window->display();
    }
}

// ??? getResult ????????????????????????????????????????????????????????????????
void TicTacToe::getResult(char* p1Out, int& p1WinsOut, int& p2WinsOut,
    char* p2Out, int& totalGamesOut) const
{
    myCpy(p1Out, p1Name);
    myCpy(p2Out, p2Name);
    p1WinsOut = p1Wins;
    p2WinsOut = p2Wins;
    totalGamesOut = totalGames;
}