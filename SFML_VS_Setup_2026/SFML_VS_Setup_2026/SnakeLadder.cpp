#include "SnakeLadder.h"
#include <cmath>

// ??? Constants ????????????????????????????????????????????????????????????????
const float SnakeLadder::BOARD_X = 30.f;
const float SnakeLadder::BOARD_Y = 30.f;
const float SnakeLadder::CELL_SIZE = 78.f;

// ??? String helpers ???????????????????????????????????????????????????????????
static int  lLen(const char* s) { int i = 0; while (s[i])i++; return i; }
static void lCpy(char* d, const char* s) { int i = 0; while ((d[i] = s[i]))i++; }
static void lCat(char* d, const char* s) { int i = lLen(d), j = 0; while ((d[i++] = s[j++])); }
static void lIts(int v, char* b) {
    if (v == 0) { b[0] = '0'; b[1] = '\0'; return; }
    char t[12]; int i = 0;
    while (v > 0) { t[i++] = '0' + v % 10; v /= 10; }
    for (int j = 0; j < i; j++)b[j] = t[i - 1 - j]; b[i] = '\0';
}

// ??? Constructor ??????????????????????????????????????????????????????????????
SnakeLadder::SnakeLadder(sf::RenderWindow* win, sf::Music* music)
    : window(win), hubMusic(music), glowTimer(0.f),
    state(SLState::WELCOME),
    currentPlayer(0), diceValue(1), totalGames(0),
    diceAnimTimer(0.f), diceAnimDuration(0.8f),
    diceDisplay(1), diceSettled(false),
    moveTimer(0.f), moveDuration(0.4f),
    moveTargetPos(0), moveStartPos(0),
    slideTimer(0.f), slideDuration(0.6f),
    slideFrom(0), slideTo(0),
    messageTimer(0.f), inputLen(0)
{
    neonCyan = sf::Color(0, 240, 255);
    neonPink = sf::Color(255, 30, 180);
    neonYellow = sf::Color(255, 220, 0);
    neonGreen = sf::Color(0, 255, 120);
    neonOrange = sf::Color(255, 140, 0);
    darkBg = sf::Color(5, 5, 18);
    cellLight = sf::Color(18, 18, 48);
    cellDark = sf::Color(10, 10, 30);

    lCpy(players[0].name, "PLAYER 1");
    lCpy(players[1].name, "PLAYER 2");
    players[0].color = neonCyan;
    players[1].color = neonPink;
    players[0].wins = players[1].wins = 0;
    message[0] = '\0';
    inputBuffer[0] = '\0';

    font.loadFromFile("Orbitron-VariableFont_wght.ttf");
    initScanlines();
    initBoard();
    resetGame();

    if (bufSnake.loadFromFile("snake.ogg"))   sndSnake.setBuffer(bufSnake);
    if (bufLadder.loadFromFile("ladder.ogg")) sndLadder.setBuffer(bufLadder);
    if (bufDice.loadFromFile("dice.ogg"))     sndDice.setBuffer(bufDice);
    if (bufWin.loadFromFile("win.ogg"))       sndWin.setBuffer(bufWin);
    sndSnake.setVolume(85.f);
    sndLadder.setVolume(85.f);
    sndDice.setVolume(80.f);
    sndWin.setVolume(90.f);
}

// ??? Scanlines ????????????????????????????????????????????????????????????????
void SnakeLadder::initScanlines() {
    for (int i = 0; i < 120; i++) {
        scanlines[i].setSize({ 1200.f,1.f });
        scanlines[i].setPosition(0.f, i * 7.5f);
        scanlines[i].setFillColor(sf::Color(0, 0, 0, 14));
    }
}

// ??? Board setup ?????????????????????????????????????????????????????????????
void SnakeLadder::initBoard() {
    // Classic snake & ladder positions
    snakes[0] = { 97,78 }; snakes[1] = { 95,56 }; snakes[2] = { 88,24 };
    snakes[3] = { 62,18 }; snakes[4] = { 48,26 }; snakes[5] = { 36,6 };
    snakes[6] = { 32,10 }; snakes[7] = { 17,7 };

    ladders[0] = { 1,38 };  ladders[1] = { 4,14 };  ladders[2] = { 9,31 };
    ladders[3] = { 20,38 }; ladders[4] = { 28,84 }; ladders[5] = { 40,59 };
    ladders[6] = { 51,67 }; ladders[7] = { 63,81 };
}

void SnakeLadder::resetGame() {
    for (int i = 0; i < 2; i++) {
        players[i].pos = 0;
        players[i].snakeHits = 0;
        players[i].ladderClimbs = 0;
        players[i].totalRolls = 0;
        sf::Vector2f start = cellToPixel(0);
        players[i].animX = players[i].targetX = start.x;
        players[i].animY = players[i].targetY = start.y;
    }
    currentPlayer = 0;
    diceSettled = true;
    diceValue = 1;
    diceDisplay = 1;
    messageTimer = 0.f;
    message[0] = '\0';
}

// ??? Cell to pixel ????????????????????????????????????????????????????????????
// Cell 0 = off-board start, cells 1-100 on board
// Row 0 (bottom) = cells 1-10, row 1 = 11-20 (right to left), etc.
sf::Vector2f SnakeLadder::cellToPixel(int cellNum) {
    if (cellNum <= 0) return { BOARD_X + CELL_SIZE * 5.f, BOARD_Y + CELL_SIZE * 10.5f };

    int idx = cellNum - 1;          // 0-99
    int row = idx / GRID;           // 0=bottom row
    int col = idx % GRID;
    if (row % 2 == 1) col = GRID - 1 - col; // snake pattern - alternate rows reverse

    float px = BOARD_X + col * CELL_SIZE + CELL_SIZE / 2.f;
    float py = BOARD_Y + (GRID - 1 - row) * CELL_SIZE + CELL_SIZE / 2.f;
    return { px, py };
}

// ??? Dice ?????????????????????????????????????????????????????????????????????
int SnakeLadder::rollDice() {
    // Pseudo-random using glowTimer
    int r = (int)(std::abs(std::sin(glowTimer * 73.1f + (float)players[currentPlayer].totalRolls * 17.3f)) * 1000.f) % 6 + 1;
    return r;
}

void SnakeLadder::setMessage(const char* msg) {
    lCpy(message, msg);
    messageTimer = 3.f;
}

// ??? Process roll result ??????????????????????????????????????????????????????
void SnakeLadder::processRoll() {
    int newPos = players[currentPlayer].pos + diceValue;
    players[currentPlayer].totalRolls++;

    if (newPos > 100) {
        // Can't move - need exact roll
        setMessage("NEED EXACT ROLL TO WIN!");
        state = SLState::PLAYING;
        currentPlayer = 1 - currentPlayer;
        return;
    }

    // Animate movement
    moveStartPos = players[currentPlayer].pos;
    moveTargetPos = newPos;
    moveTimer = 0.f;
    state = SLState::MOVING;
}

// ??? Background ???????????????????????????????????????????????????????????????
void SnakeLadder::drawBackground() {
    window->clear(darkBg);
    sf::CircleShape glow(380.f);
    glow.setFillColor(sf::Color(0, 40, 80, 16));
    glow.setOrigin(380.f, 380.f);
    glow.setPosition(430.f, 430.f);
    window->draw(glow);
    sf::CircleShape dot(1.f);
    dot.setFillColor(sf::Color(255, 255, 255, 10));
    for (int x = 820; x < 1200; x += 40)
        for (int y = 30; y < 900; y += 40) {
            dot.setPosition((float)x, (float)y);
            window->draw(dot);
        }
    for (int i = 0; i < 120; i++) window->draw(scanlines[i]);
}

// ??? Draw board ???????????????????????????????????????????????????????????????
void SnakeLadder::drawBoard() {
    for (int cell = 1; cell <= 100; cell++) {
        int idx = cell - 1;
        int row = idx / GRID;
        int col = idx % GRID;
        if (row % 2 == 1) col = GRID - 1 - col;

        float px = BOARD_X + col * CELL_SIZE;
        float py = BOARD_Y + (GRID - 1 - row) * CELL_SIZE;

        // Alternating cell colors
        sf::RectangleShape rect({ CELL_SIZE - 1.f, CELL_SIZE - 1.f });
        rect.setPosition(px, py);
        bool isLight = (row + col) % 2 == 0;
        rect.setFillColor(isLight ? cellLight : cellDark);
        rect.setOutlineThickness(1.f);
        rect.setOutlineColor(sf::Color(0, 240, 255, 25));
        window->draw(rect);

        // Cell number
        char buf[8]; lIts(cell, buf);
        sf::Text t; t.setFont(font); t.setCharacterSize(10);
        t.setFillColor(sf::Color(100, 100, 140, 180));
        t.setString(buf);
        t.setPosition(px + 3.f, py + 3.f);
        window->draw(t);
    }

    // Board border glow
    sf::RectangleShape border({ CELL_SIZE * GRID + 2.f, CELL_SIZE * GRID + 2.f });
    border.setPosition(BOARD_X - 1.f, BOARD_Y - 1.f);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(3.f);
    border.setOutlineColor(sf::Color(0, 240, 255, 120));
    window->draw(border);
}

// ??? Draw thick line helper ???????????????????????????????????????????????????
void SnakeLadder::drawThickLine(float x1, float y1, float x2, float y2, float thick, sf::Color col) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.f) return;
    float angle = std::atan2(dy, dx) * 180.f / 3.14159f;
    sf::RectangleShape line({ len,thick });
    line.setOrigin(0.f, thick / 2.f);
    line.setPosition(x1, y1);
    line.setRotation(angle);
    line.setFillColor(col);
    window->draw(line);
}

// ??? Draw snakes ??????????????????????????????????????????????????????????????
void SnakeLadder::drawSnakes() {
    for (int i = 0; i < SNAKE_COUNT; i++) {
        sf::Vector2f h = cellToPixel(snakes[i].head);
        sf::Vector2f t = cellToPixel(snakes[i].tail);

        float dx = t.x - h.x, dy = t.y - h.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.f) continue;

        float pulse = (std::sin(glowTimer * 2.5f + i * 0.8f) + 1.f) / 2.f;

        // ?? Compute S-curve waypoints ?????????????????????????????????????????
        const int SEGS = 22;
        float bodyThick = 9.f;
        sf::Vector2f pts[SEGS + 1];
        float nx0 = -dy / len, ny0 = dx / len;
        for (int s = 0; s <= SEGS; s++) {
            float frac = (float)s / SEGS;
            float amp = 14.f * (1.f - frac * 0.5f);
            float wave = std::sin(frac * 3.14f * 2.5f) * amp;
            pts[s] = { h.x + dx * frac + nx0 * wave, h.y + dy * frac + ny0 * wave };
        }

        // ?? Neon glow behind body ?????????????????????????????????????????????
        for (int s = 0; s < SEGS; s++) {
            float frac = (float)s / SEGS;
            float thick = (bodyThick * (1.f - frac * 0.45f) + 6.f);
            drawThickLine(pts[s].x, pts[s].y, pts[s + 1].x, pts[s + 1].y,
                thick, sf::Color(200, 30, 30, (sf::Uint8)(25 + 15 * pulse)));
        }

        // ?? Body segments (alternating scale colors) ??????????????????????????
        for (int s = 0; s < SEGS; s++) {
            float frac = (float)s / SEGS;
            float thick = bodyThick * (1.f - frac * 0.45f);
            bool even = (s % 2 == 0);
            sf::Color segCol = even ?
                sf::Color(180, (sf::Uint8)(50 + 30 * pulse), 40, 230) :
                sf::Color(140, (sf::Uint8)(30 + 20 * pulse), 30, 210);
            drawThickLine(pts[s].x, pts[s].y, pts[s + 1].x, pts[s + 1].y, thick, segCol);
        }

        // ?? Scale cross markings ??????????????????????????????????????????????
        for (int s = 1; s < SEGS; s += 2) {
            float frac = (float)s / SEGS;
            float thick = bodyThick * (1.f - frac * 0.45f) * 0.55f;
            float sdx = pts[s + 1 > SEGS ? SEGS : s + 1].x - pts[s - 1].x;
            float sdy = pts[s + 1 > SEGS ? SEGS : s + 1].y - pts[s - 1].y;
            float slen = std::sqrt(sdx * sdx + sdy * sdy);
            if (slen < 1.f) continue;
            float cx2 = pts[s].x, cy2 = pts[s].y;
            float nx2 = -sdy / slen, ny2 = sdx / slen;
            drawThickLine(cx2 - nx2 * thick, cy2 - ny2 * thick,
                cx2 + nx2 * thick, cy2 + ny2 * thick,
                1.8f, sf::Color(255, 120, 80, 160));
        }

        // ?? Belly stripe ??????????????????????????????????????????????????????
        for (int s = 0; s < SEGS; s++) {
            float frac = (float)s / SEGS;
            float thick = bodyThick * (1.f - frac * 0.45f) * 0.28f;
            drawThickLine(pts[s].x, pts[s].y, pts[s + 1].x, pts[s + 1].y,
                thick, sf::Color(255, 180, 120, 120));
        }

        // ?? Head direction ????????????????????????????????????????????????????
        float hdx = pts[1].x - pts[0].x, hdy = pts[1].y - pts[0].y;
        float hlen2 = std::sqrt(hdx * hdx + hdy * hdy);
        if (hlen2 > 0.f) { hdx /= hlen2; hdy /= hlen2; }
        float hnx = -hdy, hny = hdx;

        // Head oval
        sf::CircleShape headOval(12.f, 20);
        headOval.setOrigin(12.f, 12.f);
        headOval.setPosition(h);
        headOval.setScale(1.3f, 1.f);
        headOval.setFillColor(sf::Color(200, 35, 35, 255));
        headOval.setOutlineThickness(2.f);
        headOval.setOutlineColor(sf::Color(255, (sf::Uint8)(80 + 60 * pulse), 60, 255));
        window->draw(headOval);

        // Forked tongue
        float tx1 = h.x - hdx * 18.f, ty1 = h.y - hdy * 18.f;
        drawThickLine(h.x - hdx * 4.f, h.y - hdy * 4.f, tx1, ty1, 2.f, sf::Color(255, 60, 60, 220));
        drawThickLine(tx1, ty1, tx1 - hdx * 8.f + hnx * 5.f, ty1 - hdy * 8.f + hny * 5.f, 1.5f, sf::Color(255, 60, 60, 200));
        drawThickLine(tx1, ty1, tx1 - hdx * 8.f - hnx * 5.f, ty1 - hdy * 8.f - hny * 5.f, 1.5f, sf::Color(255, 60, 60, 200));

        // Yellow eyes with black pupils
        sf::CircleShape eyeS(4.f);
        eyeS.setOrigin(4.f, 4.f);
        eyeS.setFillColor(sf::Color(255, 220, 0, 230));
        eyeS.setPosition(h.x + hnx * 6.f - hdx * 3.f, h.y + hny * 6.f - hdy * 3.f); window->draw(eyeS);
        eyeS.setPosition(h.x - hnx * 6.f - hdx * 3.f, h.y - hny * 6.f - hdy * 3.f); window->draw(eyeS);
        sf::CircleShape pupil(2.f);
        pupil.setOrigin(2.f, 2.f);
        pupil.setFillColor(sf::Color::Black);
        pupil.setPosition(h.x + hnx * 6.f - hdx * 4.f, h.y + hny * 6.f - hdy * 4.f); window->draw(pupil);
        pupil.setPosition(h.x - hnx * 6.f - hdx * 4.f, h.y - hny * 6.f - hdy * 4.f); window->draw(pupil);

        // Head number
        char buf[8]; lIts(snakes[i].head, buf);
        sf::Text ht; ht.setFont(font); ht.setCharacterSize(10);
        ht.setFillColor(sf::Color(255, 140, 100, 220));
        ht.setStyle(sf::Text::Bold);
        ht.setString(buf);
        ht.setPosition(h.x + 14.f, h.y - 8.f);
        window->draw(ht);
    }
}

// ??? Draw ladders ?????????????????????????????????????????????????????????????
void SnakeLadder::drawLadders() {
    for (int i = 0; i < LADDER_COUNT; i++) {
        sf::Vector2f b = cellToPixel(ladders[i].bottom);
        sf::Vector2f t = cellToPixel(ladders[i].top);

        float pulse = (std::sin(glowTimer * 2.f + i * 0.7f) + 1.f) / 2.f;
        sf::Color lc(0, (sf::Uint8)(200 + 55 * pulse), 80, 200);

        // Two rails
        float dx = t.x - b.x, dy = t.y - b.y;
        float len = std::sqrt(dx * dx + dy * dy);
        float nx = -dy / len * 7.f, ny = dx / len * 7.f;
        drawThickLine(b.x + nx, b.y + ny, t.x + nx, t.y + ny, 3.f, lc);
        drawThickLine(b.x - nx, b.y - ny, t.x - nx, t.y - ny, 3.f, lc);

        // Rungs
        int rungs = (int)(len / 18.f);
        for (int r = 1; r < rungs; r++) {
            float frac = (float)r / rungs;
            float mx = b.x + dx * frac, my = b.y + dy * frac;
            drawThickLine(mx + nx, my + ny, mx - nx, my - ny, 3.f,
                sf::Color(lc.r, lc.g, lc.b, 140));
        }

        // Bottom label
        char buf[8]; lIts(ladders[i].bottom, buf);
        sf::Text bt; bt.setFont(font); bt.setCharacterSize(9);
        bt.setFillColor(sf::Color(0, 220, 100, 200));
        bt.setString(buf);
        bt.setPosition(b.x + 10.f, b.y - 8.f); window->draw(bt);
    }
}

// ??? Draw tokens ??????????????????????????????????????????????????????????????
void SnakeLadder::drawTokens() {
    for (int i = 0; i < 2; i++) {
        float ox = (i == 0) ? -10.f : 10.f;  // offset so tokens don't overlap

        // Glow
        sf::CircleShape glow(18.f);
        glow.setOrigin(18.f, 18.f);
        glow.setPosition(players[i].animX + ox, players[i].animY);
        glow.setFillColor(sf::Color(players[i].color.r, players[i].color.g,
            players[i].color.b, 25));
        window->draw(glow);

        // Token body
        sf::CircleShape token(13.f);
        token.setOrigin(13.f, 13.f);
        token.setPosition(players[i].animX + ox, players[i].animY);
        token.setFillColor(sf::Color(players[i].color.r / 4,
            players[i].color.g / 4,
            players[i].color.b / 4, 240));
        token.setOutlineThickness(3.f);
        token.setOutlineColor(players[i].color);
        window->draw(token);

        // Player initial
        char ini[3]; ini[0] = players[i].name[0]; ini[1] = '\0';
        sf::Text it; it.setFont(font); it.setCharacterSize(12);
        it.setFillColor(players[i].color); it.setStyle(sf::Text::Bold);
        it.setString(ini);
        sf::FloatRect ib = it.getLocalBounds();
        it.setOrigin(ib.width / 2.f, ib.height / 2.f);
        it.setPosition(players[i].animX + ox, players[i].animY);
        window->draw(it);
    }
}

// ??? Draw dice ????????????????????????????????????????????????????????????????
void SnakeLadder::drawDice() {
    float dx = 920.f, dy = 500.f;
    float size = 90.f;

    // Die box
    sf::RectangleShape box({ size,size });
    box.setOrigin(size / 2.f, size / 2.f);

    float pulse = (std::sin(glowTimer * 6.f) + 1.f) / 2.f;
    sf::Color borderCol = diceSettled ?
        players[currentPlayer].color :
        sf::Color(255, (sf::Uint8)(180 + 75 * pulse), 0);

    box.setFillColor(sf::Color(14, 14, 40, 240));
    box.setOutlineThickness(3.f);
    box.setOutlineColor(borderCol);
    box.setPosition(dx, dy);
    window->draw(box);

    // Dice dots layout for 1-6
    // dot positions relative to center
    struct DotPos { float x, y; };
    DotPos layouts[6][6] = {
        {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},          // 1
        {{-22,-22},{22,22},{0,0},{0,0},{0,0},{0,0}},     // 2
        {{-22,-22},{0,0},{22,22},{0,0},{0,0},{0,0}},     // 3
        {{-22,-22},{22,-22},{-22,22},{22,22},{0,0},{0,0}},// 4
        {{-22,-22},{22,-22},{0,0},{-22,22},{22,22},{0,0}},// 5
        {{-22,-22},{22,-22},{-22,0},{22,0},{-22,22},{22,22}} // 6
    };
    int shown = (diceDisplay >= 1 && diceDisplay <= 6) ? diceDisplay : 1;
    int dotCount = shown;
    for (int d = 0; d < dotCount; d++) {
        float dotX = dx + layouts[shown - 1][d].x;
        float dotY = dy + layouts[shown - 1][d].y;
        sf::CircleShape dot(6.f);
        dot.setOrigin(6.f, 6.f);
        dot.setPosition(dotX, dotY);
        dot.setFillColor(borderCol);
        window->draw(dot);
    }
}

// ??? Side panel ???????????????????????????????????????????????????????????????
void SnakeLadder::drawSidePanel() {
    float px = 820.f;

    // Panel background
    sf::RectangleShape panel({ 370.f,900.f });
    panel.setFillColor(sf::Color(8, 8, 26, 210));
    panel.setPosition(px, 0.f); window->draw(panel);
    sf::RectangleShape pline({ 2.f,900.f });
    pline.setFillColor(sf::Color(0, 240, 255, 40));
    pline.setPosition(px, 0.f); window->draw(pline);

    // Title
    sf::Text title; title.setFont(font); title.setCharacterSize(22);
    title.setFillColor(neonCyan); title.setStyle(sf::Text::Bold);
    title.setString("SNAKE & LADDER");
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.width / 2.f, tb.height / 2.f);
    title.setPosition(px + 185.f, 40.f); window->draw(title);

    sf::RectangleShape tul({ 300.f,2.f });
    tul.setFillColor(sf::Color(0, 240, 255, 100));
    tul.setPosition(px + 35.f, 62.f); window->draw(tul);

    // Player cards
    for (int i = 0; i < 2; i++) {
        float cy = 100.f + i * 130.f;
        bool isActive = (currentPlayer == i && (state == SLState::PLAYING || state == SLState::ROLLING));

        sf::RectangleShape card({ 330.f,110.f });
        card.setFillColor(sf::Color(players[i].color.r / 10,
            players[i].color.g / 10,
            players[i].color.b / 10, 200));
        card.setOutlineThickness(isActive ? 3.f : 1.5f);
        float pulse = (std::sin(glowTimer * 3.f) + 1.f) / 2.f;
        sf::Color oc = players[i].color;
        if (isActive) oc.a = (sf::Uint8)(160 + 95 * pulse);
        else oc.a = 80;
        card.setOutlineColor(oc);
        card.setPosition(px + 20.f, cy); window->draw(card);

        // Active indicator
        if (isActive) {
            sf::RectangleShape ind({ 5.f,110.f });
            ind.setFillColor(players[i].color);
            ind.setPosition(px + 20.f, cy); window->draw(ind);
        }

        // Name
        sf::Text nt; nt.setFont(font); nt.setCharacterSize(16);
        nt.setFillColor(players[i].color); nt.setStyle(sf::Text::Bold);
        nt.setString(players[i].name);
        nt.setPosition(px + 36.f, cy + 10.f); window->draw(nt);

        // Position
        char posBuf[32]; lCpy(posBuf, "SQUARE: ");
        char pn[8]; lIts(players[i].pos, pn); lCat(posBuf, pn);
        sf::Text pt; pt.setFont(font); pt.setCharacterSize(13);
        pt.setFillColor(sf::Color(200, 200, 220, 200));
        pt.setString(posBuf);
        pt.setPosition(px + 36.f, cy + 38.f); window->draw(pt);

        // Stats
        char sb[48]; lCpy(sb, "S:");
        char sn[8]; lIts(players[i].snakeHits, sn); lCat(sb, sn);
        lCat(sb, "  L:"); char ln[8]; lIts(players[i].ladderClimbs, ln); lCat(sb, ln);
        lCat(sb, "  ROLLS:"); char rn[8]; lIts(players[i].totalRolls, rn); lCat(sb, rn);
        sf::Text st; st.setFont(font); st.setCharacterSize(11);
        st.setFillColor(sf::Color(140, 140, 180, 180));
        st.setString(sb);
        st.setPosition(px + 36.f, cy + 62.f); window->draw(st);

        // Wins
        char wb[24]; lCpy(wb, "WINS: ");
        char wn[8]; lIts(players[i].wins, wn); lCat(wb, wn);
        sf::Text wt; wt.setFont(font); wt.setCharacterSize(12);
        wt.setFillColor(neonYellow);
        wt.setString(wb);
        wt.setPosition(px + 36.f, cy + 84.f); window->draw(wt);
    }

    // Dice section
    sf::Text diceLabel; diceLabel.setFont(font); diceLabel.setCharacterSize(15);
    diceLabel.setFillColor(sf::Color(160, 160, 200));
    diceLabel.setString("ROLL THE DICE");
    sf::FloatRect dlb = diceLabel.getLocalBounds();
    diceLabel.setOrigin(dlb.width / 2.f, 0.f);
    diceLabel.setPosition(px + 185.f, 460.f); window->draw(diceLabel);

    drawDice();

    // Current turn label
    char turnBuf[64];
    lCpy(turnBuf, players[currentPlayer].name);
    lCat(turnBuf, "'S TURN");
    sf::Text turnText; turnText.setFont(font); turnText.setCharacterSize(16);
    turnText.setFillColor(players[currentPlayer].color);
    turnText.setStyle(sf::Text::Bold);
    turnText.setString(turnBuf);
    sf::FloatRect ttb = turnText.getLocalBounds();
    turnText.setOrigin(ttb.width / 2.f, 0.f);
    turnText.setPosition(px + 185.f, 620.f); window->draw(turnText);

    // Instruction
    const char* inst = (state == SLState::PLAYING) ? "PRESS  SPACE  TO  ROLL" : "";
    if (state == SLState::ROLLING) inst = "ROLLING...";
    sf::Text inst_t; inst_t.setFont(font); inst_t.setCharacterSize(14);
    float ipulse = (std::sin(glowTimer * 3.f) + 1.f) / 2.f;
    inst_t.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(140 + 115 * ipulse)));
    inst_t.setStyle(sf::Text::Bold);
    inst_t.setString(inst);
    sf::FloatRect ib = inst_t.getLocalBounds();
    inst_t.setOrigin(ib.width / 2.f, 0.f);
    inst_t.setPosition(px + 185.f, 655.f); window->draw(inst_t);

    // Message
    if (messageTimer > 0.f) {
        float alpha = messageTimer > 0.5f ? 1.f : messageTimer / 0.5f;
        sf::Text msg; msg.setFont(font); msg.setCharacterSize(14);
        msg.setFillColor(sf::Color(255, 200, 80, (sf::Uint8)(alpha * 220)));
        msg.setString(message);
        sf::FloatRect mb = msg.getLocalBounds();
        msg.setOrigin(mb.width / 2.f, 0.f);
        msg.setPosition(px + 185.f, 695.f); window->draw(msg);
    }

    // Legend
    sf::RectangleShape lul({ 300.f,2.f });
    lul.setFillColor(sf::Color(40, 40, 80));
    lul.setPosition(px + 35.f, 750.f); window->draw(lul);

    sf::CircleShape snakeDot(5.f); snakeDot.setFillColor(sf::Color(220, 40, 40));
    snakeDot.setPosition(px + 35.f, 762.f); window->draw(snakeDot);
    sf::Text sl; sl.setFont(font); sl.setCharacterSize(12);
    sl.setFillColor(sf::Color(180, 100, 100, 180)); sl.setString("= SNAKE  (slides down)");
    sl.setPosition(px + 52.f, 762.f); window->draw(sl);

    sf::RectangleShape ladderDot({ 10.f,10.f });
    ladderDot.setFillColor(sf::Color(0, 200, 80));
    ladderDot.setPosition(px + 35.f, 784.f); window->draw(ladderDot);
    sf::Text ll; ll.setFont(font); ll.setCharacterSize(12);
    ll.setFillColor(sf::Color(80, 180, 100, 180)); ll.setString("= LADDER (climbs up)");
    ll.setPosition(px + 52.f, 784.f); window->draw(ll);

    // ESC hint
    sf::Text esc; esc.setFont(font); esc.setCharacterSize(11);
    esc.setFillColor(sf::Color(80, 80, 110));
    esc.setString("[ ESC ]  BACK TO MENU");
    sf::FloatRect eb = esc.getLocalBounds();
    esc.setOrigin(eb.width / 2.f, 0.f);
    esc.setPosition(px + 185.f, 870.f); window->draw(esc);
}

// ??? Centered text ????????????????????????????????????????????????????????????
void SnakeLadder::centeredText(const char* s, float y, unsigned size, sf::Color col, bool bold) {
    sf::Text t; t.setFont(font); t.setString(s);
    t.setCharacterSize(size); t.setFillColor(col);
    if (bold) t.setStyle(sf::Text::Bold);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.width / 2.f, b.height / 2.f);
    t.setPosition(600.f, y); window->draw(t);
}

// ??? Input box ????????????????????????????????????????????????????????????????
void SnakeLadder::drawInputBox(const char* prompt, sf::Color col, const char* sub) {
    sf::RectangleShape panel({ 640.f,sub ? 280.f : 240.f });
    panel.setFillColor(sf::Color(10, 10, 32, 235));
    panel.setOutlineThickness(2.f); panel.setOutlineColor(col);
    panel.setOrigin(320.f, panel.getSize().y / 2.f);
    panel.setPosition(600.f, 480.f); window->draw(panel);
    centeredText(prompt, 390.f, 20, col, true);
    sf::RectangleShape ibox({ 480.f,54.f });
    ibox.setFillColor(sf::Color(20, 20, 55, 210));
    ibox.setOutlineThickness(2.f); ibox.setOutlineColor(col);
    ibox.setOrigin(240.f, 27.f); ibox.setPosition(600.f, 460.f);
    window->draw(ibox);
    char display[36]; lCpy(display, inputBuffer);
    float pulse = (std::sin(glowTimer * 4.f) + 1.f) / 2.f;
    if (pulse > 0.5f) lCat(display, "|");
    sf::Text it; it.setFont(font); it.setCharacterSize(26);
    it.setFillColor(sf::Color::White); it.setString(display);
    sf::FloatRect ib = it.getLocalBounds();
    it.setOrigin(ib.width / 2.f, ib.height / 2.f);
    it.setPosition(600.f, 458.f); window->draw(it);
    centeredText("PRESS ENTER TO CONFIRM", 510.f, 13, sf::Color(130, 130, 170));
    if (sub) centeredText(sub, 550.f, 15, sf::Color(0, 200, 220, 200));
}

// ??????????????????????????????????????????????????????????????????????????????
//  WELCOME
// ??????????????????????????????????????????????????????????????????????????????
void SnakeLadder::drawWelcome() {
    float pulse = (std::sin(glowTimer * 1.8f) + 1.f) / 2.f;
    sf::Uint8 r = (sf::Uint8)(pulse * 60);
    centeredText("SNAKE & LADDER", 260.f, 62, sf::Color(r, 220, 0), true);

    // Decorative dice
    float dcx = 600.f, dcy = 430.f, ds = 70.f;
    sf::RectangleShape die({ ds,ds });
    die.setOrigin(ds / 2.f, ds / 2.f);
    die.setFillColor(sf::Color(14, 14, 40, 220));
    die.setOutlineThickness(3.f);
    die.setOutlineColor(sf::Color(255, 220, 0, (sf::Uint8)(160 + 95 * pulse)));
    die.setPosition(dcx, dcy); window->draw(die);
    int shown = (int)(glowTimer * 3.f) % 6 + 1;
    struct DP { float x, y; };
    DP dots5[5] = { {-18,-18},{18,-18},{0,0},{-18,18},{18,18} };
    for (int d = 0; d < (shown > 5 ? 5 : shown); d++) {
        sf::CircleShape dot(5.f); dot.setOrigin(5.f, 5.f);
        dot.setFillColor(neonYellow);
        dot.setPosition(dcx + dots5[d].x, dcy + dots5[d].y);
        window->draw(dot);
    }

    float uw = 320.f + 60.f * pulse;
    sf::RectangleShape ul({ uw,3.f });
    ul.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(140 + 115 * pulse)));
    ul.setOrigin(uw / 2.f, 1.5f); ul.setPosition(600.f, 360.f);
    window->draw(ul);

    centeredText("RACE TO SQUARE 100", 530.f, 16, sf::Color(180, 180, 220, 180));
    centeredText("AVOID SNAKES — CLIMB LADDERS", 570.f, 14, sf::Color(160, 160, 200, 160));
    sf::Uint8 pa = (sf::Uint8)(140 + 115 * pulse);
    centeredText("PRESS  ENTER  TO  START", 650.f, 22, sf::Color(255, 220, 0, pa), true);
    centeredText("[ ESC ]  BACK TO HUB", 860.f, 13, sf::Color(100, 100, 140));
}

// ??????????????????????????????????????????????????????????????????????????????
//  NAME ENTRY
// ??????????????????????????????????????????????????????????????????????????????
void SnakeLadder::drawEnterName(int player) {
    centeredText("SNAKE & LADDER", 100.f, 38, neonYellow, true);
    if (player == 1) {
        centeredText("PLAYER 1", 230.f, 52, neonCyan, true);
        drawInputBox("ENTER YOUR NAME:", neonCyan);
    }
    else {
        centeredText("PLAYER 2", 230.f, 52, neonPink, true);
        char sub[48]; lCpy(sub, "P1: "); lCat(sub, players[0].name); lCat(sub, " is ready!");
        drawInputBox("ENTER YOUR NAME:", neonPink, sub);
    }
}

void SnakeLadder::handleEnterP1(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            lCpy(players[0].name, inputBuffer);
            for (int i = 0; players[0].name[i]; i++)
                if (players[0].name[i] >= 'a' && players[0].name[i] <= 'z') players[0].name[i] -= 32;
            state = SLState::ENTER_P2; inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}

void SnakeLadder::handleEnterP2(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            lCpy(players[1].name, inputBuffer);
            for (int i = 0; players[1].name[i]; i++)
                if (players[1].name[i] >= 'a' && players[1].name[i] <= 'z') players[1].name[i] -= 32;
            totalGames = 0;
            resetGame();
            state = SLState::PLAYING;
            if (hubMusic) {
                for (int v = 50; v >= 0; v -= 5) {
                    hubMusic->setVolume((float)v);
                    sf::sleep(sf::milliseconds(20));
                }
                hubMusic->stop();
            }
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  PLAYING
// ??????????????????????????????????????????????????????????????????????????????
void SnakeLadder::handlePlaying(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Space) {
        if (state == SLState::PLAYING) {
            state = SLState::ROLLING;
            sndDice.play();
            diceSettled = false;
            diceAnimTimer = 0.f;
        }
    }
}

void SnakeLadder::drawPlaying() {
    drawBoard();
    drawLadders();
    drawSnakes();
    drawTokens();
    drawSidePanel();
}

// ??????????????????????????????????????????????????????????????????????????????
//  GAME OVER
// ??????????????????????????????????????????????????????????????????????????????
void SnakeLadder::drawGameOver() {
    drawBoard();
    drawLadders();
    drawSnakes();
    drawTokens();
    drawSidePanel();

    // Dark overlay only over board side
    sf::RectangleShape ov({ 820.f,900.f });
    ov.setFillColor(sf::Color(0, 0, 0, 170)); window->draw(ov);

    // Find winner
    int winner = (players[0].pos >= 100) ? 0 : (players[1].pos >= 100) ? 1 : -1;
    sf::Color winCol = (winner == 0) ? neonCyan : (winner == 1) ? neonPink : neonYellow;

    float pulse = (std::sin(glowTimer * 3.f) + 1.f) / 2.f;

    // ?? Outer glow behind card ????????????????????????????????????????????????
    sf::RectangleShape glowBox({ 580.f + 10.f * pulse,330.f + 8.f * pulse });
    glowBox.setOrigin(glowBox.getSize().x / 2.f, glowBox.getSize().y / 2.f);
    glowBox.setPosition(410.f, 440.f);
    glowBox.setFillColor(sf::Color(winCol.r, winCol.g, winCol.b, (sf::Uint8)(18 + 12 * pulse)));
    window->draw(glowBox);

    // ?? Main card ?????????????????????????????????????????????????????????????
    sf::RectangleShape card({ 560.f,310.f });
    card.setFillColor(sf::Color(6, 6, 22, 250));
    card.setOutlineThickness(0.f);
    card.setOrigin(280.f, 155.f); card.setPosition(410.f, 440.f);
    window->draw(card);

    // Top accent bar (winner color)
    sf::RectangleShape topBar({ 560.f,5.f });
    topBar.setFillColor(winCol);
    topBar.setOrigin(280.f, 2.5f); topBar.setPosition(410.f, 285.f);
    window->draw(topBar);

    // Bottom accent bar
    sf::RectangleShape botBar({ 560.f,5.f });
    botBar.setFillColor(sf::Color(winCol.r, winCol.g, winCol.b, 120));
    botBar.setOrigin(280.f, 2.5f); botBar.setPosition(410.f, 595.f);
    window->draw(botBar);

    // Left + right side lines
    sf::RectangleShape sideL({ 3.f,310.f });
    sideL.setFillColor(sf::Color(winCol.r, winCol.g, winCol.b, 160));
    sideL.setOrigin(1.5f, 155.f); sideL.setPosition(130.f, 440.f);
    window->draw(sideL);
    sf::RectangleShape sideR({ 3.f,310.f });
    sideR.setFillColor(sf::Color(winCol.r, winCol.g, winCol.b, 160));
    sideR.setOrigin(1.5f, 155.f); sideR.setPosition(690.f, 440.f);
    window->draw(sideR);

    // ?? Trophy icon ???????????????????????????????????????????????????????????
    float tx = 410.f, ty = 335.f;
    sf::CircleShape cup(18.f, 20);
    cup.setOrigin(18.f, 18.f); cup.setPosition(tx, ty);
    cup.setFillColor(sf::Color(winCol.r / 4, winCol.g / 4, winCol.b / 4, 200));
    cup.setOutlineThickness(2.5f); cup.setOutlineColor(winCol);
    window->draw(cup);
    // Star inside trophy
    sf::CircleShape star(8.f, 5);
    star.setOrigin(8.f, 8.f); star.setPosition(tx, ty);
    star.setFillColor(winCol); star.setRotation(glowTimer * 40.f);
    window->draw(star);

    // ?? Winner name ???????????????????????????????????????????????????????????
    if (winner >= 0) {
        sf::Text wn; wn.setFont(font); wn.setCharacterSize(42);
        wn.setFillColor(winCol); wn.setStyle(sf::Text::Bold);
        char wb[32]; lCpy(wb, players[winner].name);
        wn.setString(wb);
        sf::FloatRect wnb = wn.getLocalBounds();
        wn.setOrigin(wnb.width / 2.f, wnb.height / 2.f);
        wn.setPosition(410.f, 368.f); window->draw(wn);

        // "WINS!" text
        sf::Text ws; ws.setFont(font); ws.setCharacterSize(28);
        ws.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(200 + 55 * pulse)));
        ws.setStyle(sf::Text::Bold); ws.setString("WINS!");
        sf::FloatRect wsb = ws.getLocalBounds();
        ws.setOrigin(wsb.width / 2.f, wsb.height / 2.f);
        ws.setPosition(410.f, 415.f); window->draw(ws);

        // Divider line
        sf::RectangleShape div({ 300.f,1.f });
        div.setFillColor(sf::Color(winCol.r, winCol.g, winCol.b, 80));
        div.setOrigin(150.f, 0.5f); div.setPosition(410.f, 445.f);
        window->draw(div);

        // "REACHED SQUARE 100"
        sf::Text sub; sub.setFont(font); sub.setCharacterSize(14);
        sub.setFillColor(sf::Color(180, 180, 210, 200));
        sub.setString("REACHED SQUARE 100!");
        sf::FloatRect sb = sub.getLocalBounds();
        sub.setOrigin(sb.width / 2.f, sb.height / 2.f);
        sub.setPosition(410.f, 462.f); window->draw(sub);
    }

    // ?? Action buttons ????????????????????????????????????????????????????????
    // ENTER button
    sf::RectangleShape btn1({ 240.f,44.f });
    btn1.setFillColor(sf::Color(0, 60, 30, 200));
    btn1.setOutlineThickness(2.f); btn1.setOutlineColor(neonGreen);
    btn1.setOrigin(120.f, 22.f); btn1.setPosition(410.f, 507.f);
    window->draw(btn1);
    sf::Text t1; t1.setFont(font); t1.setCharacterSize(15);
    t1.setFillColor(neonGreen); t1.setStyle(sf::Text::Bold);
    t1.setString("[ ENTER ]  PLAY AGAIN");
    sf::FloatRect t1b = t1.getLocalBounds();
    t1.setOrigin(t1b.width / 2.f, t1b.height / 2.f);
    t1.setPosition(410.f, 505.f); window->draw(t1);

    // F button
    sf::RectangleShape btn2({ 240.f,44.f });
    btn2.setFillColor(sf::Color(60, 20, 0, 200));
    btn2.setOutlineThickness(2.f); btn2.setOutlineColor(neonOrange);
    btn2.setOrigin(120.f, 22.f); btn2.setPosition(410.f, 558.f);
    window->draw(btn2);
    sf::Text t2; t2.setFont(font); t2.setCharacterSize(15);
    t2.setFillColor(neonOrange); t2.setStyle(sf::Text::Bold);
    t2.setString("[ F ]  FINAL RESULTS");
    sf::FloatRect t2b = t2.getLocalBounds();
    t2.setOrigin(t2b.width / 2.f, t2b.height / 2.f);
    t2.setPosition(410.f, 556.f); window->draw(t2);

    // ESC
    sf::Text esc; esc.setFont(font); esc.setCharacterSize(12);
    esc.setFillColor(sf::Color(90, 90, 120));
    esc.setString("[ ESC ]  BACK TO HUB");
    sf::FloatRect eb = esc.getLocalBounds();
    esc.setOrigin(eb.width / 2.f, eb.height / 2.f);
    esc.setPosition(410.f, 592.f); window->draw(esc);
}

// ??????????????????????????????????????????????????????????????????????????????
//  FINAL RESULTS
// ??????????????????????????????????????????????????????????????????????????????
void SnakeLadder::drawFinalResults() {
    float pulse = (std::sin(glowTimer * 2.f) + 1.f) / 2.f;
    sf::CircleShape glow(260.f);
    glow.setFillColor(sf::Color(255, 200, 0, (sf::Uint8)(10 + 10 * pulse)));
    glow.setOrigin(260.f, 260.f); glow.setPosition(600.f, 430.f);
    window->draw(glow);

    centeredText("FINAL RESULTS", 95.f, 46, neonYellow, true);
    float uw = 380.f + 40.f * pulse;
    sf::RectangleShape ul({ uw,3.f });
    ul.setFillColor(neonYellow); ul.setOrigin(uw / 2.f, 1.5f);
    ul.setPosition(600.f, 148.f); window->draw(ul);

    char tgBuf[48]; lCpy(tgBuf, "TOTAL GAMES: ");
    char tgn[8]; lIts(totalGames, tgn); lCat(tgBuf, tgn);
    centeredText(tgBuf, 192.f, 15, sf::Color(160, 160, 200));

    for (int i = 0; i < 2; i++) {
        float posX = (i == 0) ? 340.f : 860.f;

        sf::RectangleShape c({ 380.f,240.f });
        c.setFillColor(sf::Color(players[i].color.r / 12,
            players[i].color.g / 12,
            players[i].color.b / 12, 200));
        c.setOutlineThickness(2.f); c.setOutlineColor(players[i].color);
        c.setOrigin(190.f, 120.f); c.setPosition(posX, 390.f);
        window->draw(c);

        sf::Text nt; nt.setFont(font); nt.setCharacterSize(20);
        nt.setFillColor(players[i].color); nt.setStyle(sf::Text::Bold);
        nt.setString(players[i].name);
        sf::FloatRect nb = nt.getLocalBounds();
        nt.setOrigin(nb.width / 2.f, nb.height / 2.f);
        nt.setPosition(posX, 305.f); window->draw(nt);

        char wb[8]; lIts(players[i].wins, wb);
        sf::Text wt; wt.setFont(font); wt.setCharacterSize(62);
        wt.setFillColor(players[i].color); wt.setStyle(sf::Text::Bold);
        wt.setString(wb);
        sf::FloatRect wtb = wt.getLocalBounds();
        wt.setOrigin(wtb.width / 2.f, wtb.height / 2.f);
        wt.setPosition(posX, 375.f); window->draw(wt);

        sf::Text wl; wl.setFont(font); wl.setCharacterSize(13);
        wl.setFillColor(sf::Color(180, 180, 200, 180)); wl.setString("WINS");
        sf::FloatRect wlb = wl.getLocalBounds();
        wl.setOrigin(wlb.width / 2.f, wl.getLocalBounds().height / 2.f);
        wl.setPosition(posX, 440.f); window->draw(wl);

        // Stats
        char sb[48]; lCpy(sb, "Snakes: ");
        char sn[8]; lIts(players[i].snakeHits, sn); lCat(sb, sn);
        lCat(sb, "  Ladders: ");
        char ln[8]; lIts(players[i].ladderClimbs, ln); lCat(sb, ln);
        sf::Text st; st.setFont(font); st.setCharacterSize(12);
        st.setFillColor(sf::Color(160, 160, 190, 180)); st.setString(sb);
        sf::FloatRect stb = st.getLocalBounds();
        st.setOrigin(stb.width / 2.f, stb.height / 2.f);
        st.setPosition(posX, 475.f); window->draw(st);
    }

    // Champion
    char banBuf[64];
    if (players[0].wins > players[1].wins) { lCpy(banBuf, players[0].name); lCat(banBuf, " IS THE CHAMPION!"); }
    else if (players[1].wins > players[0].wins) { lCpy(banBuf, players[1].name); lCat(banBuf, " IS THE CHAMPION!"); }
    else lCpy(banBuf, "PERFECTLY EVEN!");
    centeredText(banBuf, 545.f, 24, neonYellow, true);

    centeredText("[ ENTER ]  NEW SESSION", 640.f, 17, neonGreen);
    centeredText("[ ESC ]  BACK TO HUB", 690.f, 14, sf::Color(100, 100, 140));
}

// ??? Update animations ????????????????????????????????????????????????????????
void SnakeLadder::updateAnimations(float dt) {
    glowTimer += dt;
    if (messageTimer > 0.f) messageTimer -= dt;

    // Dice rolling animation
    if (state == SLState::ROLLING) {
        diceAnimTimer += dt;
        // Fast random display during roll
        diceDisplay = (int)(std::abs(std::sin(glowTimer * 20.f)) * 6.f) % 6 + 1;

        if (diceAnimTimer >= diceAnimDuration) {
            diceValue = rollDice();
            diceDisplay = diceValue;
            diceSettled = true;
            processRoll();
        }
    }

    // Token movement animation
    if (state == SLState::MOVING) {
        moveTimer += dt;
        float t = moveTimer / moveDuration;
        if (t > 1.f) t = 1.f;

        // Smooth lerp between start and target
        sf::Vector2f startPx = cellToPixel(moveStartPos);
        sf::Vector2f endPx = cellToPixel(moveTargetPos);

        // Ease in-out
        float ease = t < 0.5f ? 2.f * t * t : 1.f - 2.f * (1.f - t) * (1.f - t);
        players[currentPlayer].animX = startPx.x + (endPx.x - startPx.x) * ease;
        players[currentPlayer].animY = startPx.y + (endPx.y - startPx.y) * ease;

        if (t >= 1.f) {
            players[currentPlayer].pos = moveTargetPos;
            players[currentPlayer].animX = endPx.x;
            players[currentPlayer].animY = endPx.y;

            // Check win
            if (players[currentPlayer].pos >= 100) {
                players[currentPlayer].wins++;
                totalGames++;
                sndWin.play();
                state = SLState::GAME_OVER;
                return;
            }

            // Check snake
            for (int i = 0; i < SNAKE_COUNT; i++) {
                if (snakes[i].head == players[currentPlayer].pos) {
                    players[currentPlayer].snakeHits++;
                    slideFrom = snakes[i].head;
                    slideTo = snakes[i].tail;
                    slideTimer = 0.f;
                    state = SLState::SNAKE_SLIDE;
                    sndSnake.play();
                    char msg[64]; lCpy(msg, "SNAKE! "); lCat(msg, players[currentPlayer].name);
                    lCat(msg, " SLIDES DOWN!");
                    setMessage(msg);
                    return;
                }
            }
            // Check ladder
            for (int i = 0; i < LADDER_COUNT; i++) {
                if (ladders[i].bottom == players[currentPlayer].pos) {
                    players[currentPlayer].ladderClimbs++;
                    slideFrom = ladders[i].bottom;
                    slideTo = ladders[i].top;
                    slideTimer = 0.f;
                    state = SLState::LADDER_CLIMB;
                    sndLadder.play();
                    char msg[64]; lCpy(msg, "LADDER! "); lCat(msg, players[currentPlayer].name);
                    lCat(msg, " CLIMBS UP!");
                    setMessage(msg);
                    return;
                }
            }

            // Normal — next player
            currentPlayer = 1 - currentPlayer;
            state = SLState::PLAYING;
        }
    }

    // Snake/Ladder slide animation
    if (state == SLState::SNAKE_SLIDE || state == SLState::LADDER_CLIMB) {
        slideTimer += dt;
        float t = slideTimer / slideDuration;
        if (t > 1.f) t = 1.f;

        sf::Vector2f from = cellToPixel(slideFrom);
        sf::Vector2f to = cellToPixel(slideTo);
        float ease = t < 0.5f ? 2.f * t * t : 1.f - 2.f * (1.f - t) * (1.f - t);
        players[currentPlayer].animX = from.x + (to.x - from.x) * ease;
        players[currentPlayer].animY = from.y + (to.y - from.y) * ease;

        if (t >= 1.f) {
            players[currentPlayer].pos = slideTo;
            players[currentPlayer].animX = to.x;
            players[currentPlayer].animY = to.y;
            currentPlayer = 1 - currentPlayer;
            state = SLState::PLAYING;
        }
    }

    // Keep anim positions synced when not animating
    if (state == SLState::PLAYING || state == SLState::ROLLING) {
        for (int i = 0; i < 2; i++) {
            sf::Vector2f p = cellToPixel(players[i].pos);
            players[i].animX = p.x; players[i].animY = p.y;
        }
    }
}

// ??? Main Run Loop ????????????????????????????????????????????????????????????
void SnakeLadder::run() {
    sf::Clock clock;

    while (window->isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        sf::Event event;
        while (window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) window->close();

            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape &&
                state != SLState::PLAYING)
                return;

            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape &&
                state == SLState::PLAYING)
                return;

            switch (state) {
            case SLState::WELCOME:
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                    state = SLState::ENTER_P1; inputBuffer[0] = '\0'; inputLen = 0;
                } break;
            case SLState::ENTER_P1:      handleEnterP1(event); break;
            case SLState::ENTER_P2:      handleEnterP2(event); break;
            case SLState::PLAYING:       handlePlaying(event); break;
            case SLState::GAME_OVER:
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Enter) { resetGame(); state = SLState::PLAYING; }
                    if (event.key.code == sf::Keyboard::F)     state = SLState::FINAL_RESULTS;
                } break;
            case SLState::FINAL_RESULTS:
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                    players[0].wins = 0; players[1].wins = 0; totalGames = 0;
                    inputBuffer[0] = '\0'; inputLen = 0;
                    state = SLState::ENTER_P1;
                } break;
            default: break;
            }
        }

        updateAnimations(dt);
        drawBackground();

        switch (state) {
        case SLState::WELCOME:       drawWelcome();     break;
        case SLState::ENTER_P1:      drawEnterName(1);  break;
        case SLState::ENTER_P2:      drawEnterName(2);  break;
        case SLState::PLAYING:
        case SLState::ROLLING:
        case SLState::MOVING:
        case SLState::SNAKE_SLIDE:
        case SLState::LADDER_CLIMB:  drawPlaying();     break;
        case SLState::GAME_OVER:     drawGameOver();    break;
        case SLState::FINAL_RESULTS: drawFinalResults(); break;
        }

        window->display();
    }
}

// ??? getResult ????????????????????????????????????????????????????????????????
void SnakeLadder::getResult(char* p1Out, int& p1WinsOut, int& p2WinsOut,
    char* p2Out, int& totalGamesOut) const
{
    lCpy(p1Out, players[0].name);
    lCpy(p2Out, players[1].name);
    p1WinsOut = players[0].wins;
    p2WinsOut = players[1].wins;
    totalGamesOut = totalGames;
}