#include "Pong.h"
#include <cmath>

// ??? Static constants ?????????????????????????????????????????????????????????
const float Pong::BALL_RADIUS = 10.f;
const float Pong::BASE_SPEED = 420.f;
const float Pong::SPEED_INC = 22.f;
const float Pong::PADDLE_W = 14.f;
const float Pong::PADDLE_H = 100.f;
const float Pong::PADDLE_SPEED = 580.f;
const float Pong::PADDLE_MARGIN = 40.f;
const float Pong::W = 1200.f;
const float Pong::H = 900.f;
const int   Pong::WIN_SCORE = 5;

// ??? String helpers ???????????????????????????????????????????????????????????
static int   pLen(const char* s) { int i = 0; while (s[i]) i++; return i; }
static void  pCpy(char* d, const char* s) { int i = 0; while ((d[i] = s[i])) i++; }
static void  pCat(char* d, const char* s) { int i = pLen(d), j = 0; while ((d[i++] = s[j++])); }
static void  pIts(int v, char* b) {
    if (v == 0) { b[0] = '0'; b[1] = '\0'; return; }
    char t[12]; int i = 0;
    while (v > 0) { t[i++] = '0' + v % 10; v /= 10; }
    for (int j = 0; j < i; j++) b[j] = t[i - 1 - j]; b[i] = '\0';
}

// ??? Constructor ??????????????????????????????????????????????????????????????
Pong::Pong(sf::RenderWindow* win)
    : window(win), glowTimer(0.f),
    state(PongState::WELCOME),
    p1Score(0), p2Score(0),
    p1Wins(0), p2Wins(0), totalMatches(0),
    inputLen(0), ballVx(0), ballVy(0), ballSpeed(BASE_SPEED),
    countdownTimer(0.f), countdownNum(3),
    pointFlashTimer(0.f), lastScorer(0),
    trailIdx(0)
{
    neonCyan = sf::Color(0, 240, 255);
    neonPink = sf::Color(255, 30, 180);
    neonYellow = sf::Color(255, 220, 0);
    neonGreen = sf::Color(0, 255, 120);
    darkBg = sf::Color(6, 6, 20);

    pCpy(p1Name, "PLAYER 1");
    pCpy(p2Name, "PLAYER 2");
    inputBuffer[0] = '\0';

    font.loadFromFile("Orbitron-VariableFont_wght.ttf");

    // Ball setup
    ball.setRadius(BALL_RADIUS);
    ball.setFillColor(sf::Color::White);
    ball.setOrigin(BALL_RADIUS, BALL_RADIUS);

    // Paddle setup
    p1Paddle.setSize({ PADDLE_W, PADDLE_H });
    p1Paddle.setFillColor(neonCyan);
    p2Paddle.setSize({ PADDLE_W, PADDLE_H });
    p2Paddle.setFillColor(neonPink);

    // Trail init
    for (int i = 0; i < TRAIL_LEN; i++) trail[i] = { W / 2.f, H / 2.f };

    initScanlines();
    resetPaddles();
    resetBall(1);

    if (bufHit.loadFromFile("paddle_hit.ogg"))  sndHit.setBuffer(bufHit);
    if (bufMiss.loadFromFile("ball_miss.ogg"))  sndMiss.setBuffer(bufMiss);
    if (bufWin.loadFromFile("win.ogg"))         sndWin.setBuffer(bufWin);
    sndHit.setVolume(80.f);
    sndMiss.setVolume(80.f);
    sndWin.setVolume(90.f);
}

// ??? Scanlines ????????????????????????????????????????????????????????????????
void Pong::initScanlines() {
    for (int i = 0; i < 120; i++) {
        scanlines[i].setSize({ 1200.f,1.f });
        scanlines[i].setPosition(0.f, i * 7.5f);
        scanlines[i].setFillColor(sf::Color(0, 0, 0, 18));
    }
}

// ??? Reset helpers ????????????????????????????????????????????????????????????
void Pong::resetPaddles() {
    p1Paddle.setPosition(PADDLE_MARGIN, H / 2.f - PADDLE_H / 2.f);
    p2Paddle.setPosition(W - PADDLE_MARGIN - PADDLE_W, H / 2.f - PADDLE_H / 2.f);
}

void Pong::resetBall(int serveDir) {
    ball.setPosition(W / 2.f, H / 2.f);
    ballSpeed = BASE_SPEED;

    // Random-ish vertical angle
    float angle = (3.14159f / 6.f) * ((glowTimer * 3.7f) - (int)(glowTimer * 3.7f) > 0.5f ? 1.f : -1.f);
    ballVx = ballSpeed * serveDir;
    ballVy = ballSpeed * 0.55f * (std::sin(glowTimer * 7.3f) > 0 ? 1.f : -1.f);

    for (int i = 0; i < TRAIL_LEN; i++) trail[i] = { W / 2.f,H / 2.f };
    trailIdx = 0;
    (void)angle;
}

void Pong::resetMatch() {
    p1Score = 0; p2Score = 0;
    resetPaddles();
    resetBall(1);
}

void Pong::startCountdown() {
    state = PongState::COUNTDOWN;
    countdownTimer = 0.f;
    countdownNum = 3;
    resetPaddles();
}

// ??? Background ???????????????????????????????????????????????????????????????
void Pong::drawBackground() {
    window->clear(darkBg);
    sf::CircleShape glow(400.f);
    glow.setFillColor(sf::Color(0, 30, 80, 18));
    glow.setOrigin(400.f, 400.f);
    glow.setPosition(W / 2.f, H / 2.f);
    window->draw(glow);
    sf::CircleShape dot(1.f);
    dot.setFillColor(sf::Color(255, 255, 255, 10));
    for (int x = 30; x < 1200; x += 45)
        for (int y = 30; y < 900; y += 45) {
            dot.setPosition((float)x, (float)y);
            window->draw(dot);
        }
    for (int i = 0; i < 120; i++) window->draw(scanlines[i]);
}

// ??? Field ????????????????????????????????????????????????????????????????????
void Pong::drawField() {
    // Top & bottom walls
    sf::RectangleShape wall({ W, 4.f });
    wall.setFillColor(sf::Color(0, 240, 255, 60));
    wall.setPosition(0.f, 70.f); window->draw(wall);
    wall.setPosition(0.f, H - 4.f); window->draw(wall);

    // Dashed center line
    for (int i = 0; i < 28; i++) {
        sf::RectangleShape dash({ 3.f,18.f });
        dash.setFillColor(sf::Color(255, 255, 255, 30));
        dash.setPosition(W / 2.f - 1.5f, 75.f + i * 30.f);
        window->draw(dash);
    }
}

// ??? Score bar ????????????????????????????????????????????????????????????????
void Pong::drawScoreBar() {
    sf::RectangleShape bar({ W,70.f });
    bar.setFillColor(sf::Color(8, 8, 28, 220));
    bar.setPosition(0.f, 0.f); window->draw(bar);
    sf::RectangleShape line({ W,2.f });
    line.setFillColor(sf::Color(0, 240, 255, 60));
    line.setPosition(0.f, 70.f); window->draw(line);

    // P1 name
    sf::Text n1; n1.setFont(font); n1.setCharacterSize(18);
    n1.setFillColor(neonCyan); n1.setStyle(sf::Text::Bold);
    n1.setString(p1Name);
    n1.setPosition(30.f, 14.f); window->draw(n1);

    // P1 score
    char s1[8]; pIts(p1Score, s1);
    sf::Text sc1; sc1.setFont(font); sc1.setCharacterSize(32);
    sc1.setFillColor(neonCyan); sc1.setStyle(sf::Text::Bold);
    sc1.setString(s1);
    sf::FloatRect f1 = sc1.getLocalBounds();
    sc1.setOrigin(f1.width / 2.f, f1.height / 2.f);
    sc1.setPosition(W / 2.f - 80.f, 32.f); window->draw(sc1);

    // VS
    sf::Text vs; vs.setFont(font); vs.setCharacterSize(16);
    vs.setFillColor(sf::Color(160, 160, 200, 160)); vs.setString(":");
    sf::FloatRect vb = vs.getLocalBounds();
    vs.setOrigin(vb.width / 2.f, vb.height / 2.f);
    vs.setPosition(W / 2.f, 32.f); window->draw(vs);

    // P2 score
    char s2[8]; pIts(p2Score, s2);
    sf::Text sc2; sc2.setFont(font); sc2.setCharacterSize(32);
    sc2.setFillColor(neonPink); sc2.setStyle(sf::Text::Bold);
    sc2.setString(s2);
    sf::FloatRect f2 = sc2.getLocalBounds();
    sc2.setOrigin(f2.width / 2.f, f2.height / 2.f);
    sc2.setPosition(W / 2.f + 80.f, 32.f); window->draw(sc2);

    // P2 name
    sf::Text n2; n2.setFont(font); n2.setCharacterSize(18);
    n2.setFillColor(neonPink); n2.setStyle(sf::Text::Bold);
    n2.setString(p2Name);
    sf::FloatRect nb2 = n2.getLocalBounds();
    n2.setOrigin(nb2.width, nb2.height / 2.f);
    n2.setPosition(W - 30.f, 32.f); window->draw(n2);

    // Win counters (small)
    char w1[32]; pCpy(w1, "WINS: "); char wn1[8]; pIts(p1Wins, wn1); pCat(w1, wn1);
    sf::Text wt1; wt1.setFont(font); wt1.setCharacterSize(11);
    wt1.setFillColor(sf::Color(0, 200, 220, 160)); wt1.setString(w1);
    wt1.setPosition(30.f, 50.f); window->draw(wt1);

    char w2[32]; pCpy(w2, "WINS: "); char wn2[8]; pIts(p2Wins, wn2); pCat(w2, wn2);
    sf::Text wt2; wt2.setFont(font); wt2.setCharacterSize(11);
    wt2.setFillColor(sf::Color(220, 0, 160, 160)); wt2.setString(w2);
    sf::FloatRect wb2 = wt2.getLocalBounds();
    wt2.setOrigin(wb2.width, 0.f);
    wt2.setPosition(W - 30.f, 50.f); window->draw(wt2);
}

// ??? Ball with trail ??????????????????????????????????????????????????????????
void Pong::drawBall() {
    // Trail
    for (int i = 0; i < TRAIL_LEN; i++) {
        int idx = (trailIdx - i - 1 + TRAIL_LEN) % TRAIL_LEN;
        float alpha = (float)(TRAIL_LEN - i) / TRAIL_LEN;
        float radius = BALL_RADIUS * alpha * 0.8f;
        sf::CircleShape t(radius);
        t.setOrigin(radius, radius);
        t.setPosition(trail[idx]);
        t.setFillColor(sf::Color(0, 240, 255, (sf::Uint8)(alpha * 80)));
        window->draw(t);
    }

    // Glow behind ball
    float pulse = (std::sin(glowTimer * 6.f) + 1.f) / 2.f;
    sf::CircleShape glow(BALL_RADIUS * 2.2f + pulse * 4.f);
    glow.setFillColor(sf::Color(255, 255, 255, 18));
    glow.setOrigin(glow.getRadius(), glow.getRadius());
    glow.setPosition(ball.getPosition());
    window->draw(glow);

    window->draw(ball);
}

// ??? Paddles with glow ????????????????????????????????????????????????????????
void Pong::drawPaddles() {
    // P1 glow
    sf::RectangleShape g1({ PADDLE_W + 12.f, PADDLE_H + 12.f });
    g1.setFillColor(sf::Color(0, 240, 255, 22));
    g1.setPosition(p1Paddle.getPosition().x - 6.f, p1Paddle.getPosition().y - 6.f);
    window->draw(g1);
    window->draw(p1Paddle);

    // P2 glow
    sf::RectangleShape g2({ PADDLE_W + 12.f, PADDLE_H + 12.f });
    g2.setFillColor(sf::Color(255, 30, 180, 22));
    g2.setPosition(p2Paddle.getPosition().x - 6.f, p2Paddle.getPosition().y - 6.f);
    window->draw(g2);
    window->draw(p2Paddle);
}

// ??? Countdown ????????????????????????????????????????????????????????????????
void Pong::drawCountdown() {
    drawField();
    drawPaddles();
    drawScoreBar();

    char buf[4];
    if (countdownNum > 0) pIts(countdownNum, buf);
    else { buf[0] = 'G'; buf[1] = 'O'; buf[2] = '!'; buf[3] = '\0'; }

    float pulse = (std::sin(glowTimer * 8.f) + 1.f) / 2.f;
    sf::Text ct; ct.setFont(font); ct.setCharacterSize(120);
    ct.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(180 + 75 * pulse)));
    ct.setStyle(sf::Text::Bold);
    ct.setString(buf);
    sf::FloatRect fb = ct.getLocalBounds();
    ct.setOrigin(fb.width / 2.f, fb.height / 2.f);
    ct.setPosition(W / 2.f, H / 2.f);
    window->draw(ct);
}

// ??? Centered text helper ?????????????????????????????????????????????????????
void Pong::centeredText(const char* s, float y, unsigned size, sf::Color col, bool bold) {
    sf::Text t; t.setFont(font); t.setString(s);
    t.setCharacterSize(size); t.setFillColor(col);
    if (bold) t.setStyle(sf::Text::Bold);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.width / 2.f, b.height / 2.f);
    t.setPosition(W / 2.f, y);
    window->draw(t);
}

// ??? Input box ????????????????????????????????????????????????????????????????
void Pong::drawInputBox(const char* prompt, sf::Color col, const char* sub) {
    sf::RectangleShape panel({ 640.f, sub ? 280.f : 240.f });
    panel.setFillColor(sf::Color(10, 10, 32, 235));
    panel.setOutlineThickness(2.f); panel.setOutlineColor(col);
    panel.setOrigin(320.f, panel.getSize().y / 2.f);
    panel.setPosition(W / 2.f, 480.f); window->draw(panel);

    centeredText(prompt, 390.f, 20, col, true);

    sf::RectangleShape ibox({ 480.f,54.f });
    ibox.setFillColor(sf::Color(20, 20, 55, 210));
    ibox.setOutlineThickness(2.f); ibox.setOutlineColor(col);
    ibox.setOrigin(240.f, 27.f); ibox.setPosition(W / 2.f, 460.f);
    window->draw(ibox);

    char display[36]; pCpy(display, inputBuffer);
    float pulse = (std::sin(glowTimer * 4.f) + 1.f) / 2.f;
    if (pulse > 0.5f) pCat(display, "|");
    sf::Text it; it.setFont(font); it.setCharacterSize(26);
    it.setFillColor(sf::Color::White); it.setString(display);
    sf::FloatRect ib = it.getLocalBounds();
    it.setOrigin(ib.width / 2.f, ib.height / 2.f);
    it.setPosition(W / 2.f, 458.f); window->draw(it);

    centeredText("PRESS ENTER TO CONFIRM", 510.f, 13, sf::Color(130, 130, 170));
    if (sub) centeredText(sub, 550.f, 15, sf::Color(0, 200, 220, 200));
}

// ??????????????????????????????????????????????????????????????????????????????
//  WELCOME
// ??????????????????????????????????????????????????????????????????????????????
void Pong::drawWelcome() {
    float pulse = (std::sin(glowTimer * 1.8f) + 1.f) / 2.f;
    sf::Uint8 r = (sf::Uint8)(pulse * 60);
    centeredText("PONG", 260.f, 90, sf::Color(r, 240, 255), true);

    // Draw decorative paddles & ball
    sf::RectangleShape lp({ 14.f,80.f }); lp.setFillColor(sf::Color(0, 240, 255, 120));
    lp.setPosition(W / 2.f - 200.f, 420.f); window->draw(lp);
    sf::RectangleShape rp({ 14.f,80.f }); rp.setFillColor(sf::Color(255, 30, 180, 120));
    rp.setPosition(W / 2.f + 186.f, 420.f); window->draw(rp);
    sf::CircleShape b(12.f); b.setFillColor(sf::Color(255, 255, 255, 120));
    b.setOrigin(12.f, 12.f); b.setPosition(W / 2.f, 460.f); window->draw(b);

    float uw = 300.f + 60.f * pulse;
    sf::RectangleShape ul({ uw,3.f });
    ul.setFillColor(sf::Color(0, 240, 255, (sf::Uint8)(140 + 115 * pulse)));
    ul.setOrigin(uw / 2.f, 1.5f); ul.setPosition(W / 2.f, 370.f);
    window->draw(ul);

    centeredText("FIRST TO 5 POINTS WINS", 520.f, 16, sf::Color(180, 180, 220, 180));

    sf::Uint8 pa = (sf::Uint8)(140 + 115 * pulse);
    centeredText("PRESS  ENTER  TO  START", 610.f, 22, sf::Color(255, 220, 0, pa), true);

    // Controls hint
    centeredText("P1: W / S  KEYS        P2: UP / DOWN  ARROWS",
        720.f, 14, sf::Color(140, 140, 180, 160));
    centeredText("[ ESC ]  BACK TO HUB", 860.f, 13, sf::Color(100, 100, 140));
}

void Pong::handleWelcome(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Enter) {
            state = PongState::ENTER_P1;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  NAME ENTRY
// ??????????????????????????????????????????????????????????????????????????????
void Pong::drawEnterName(int player) {
    centeredText("PONG", 100.f, 42, neonCyan, true);
    if (player == 1) {
        centeredText("PLAYER 1", 230.f, 52, neonCyan, true);
        centeredText("Controls:  W  (up)   S  (down)", 300.f, 15, sf::Color(160, 160, 210));
        drawInputBox("ENTER YOUR NAME:", neonCyan);
    }
    else {
        centeredText("PLAYER 2", 230.f, 52, neonPink, true);
        centeredText("Controls:  UP  /  DOWN  arrows", 300.f, 15, sf::Color(160, 160, 210));
        char sub[48]; pCpy(sub, "P1: "); pCat(sub, p1Name); pCat(sub, " is ready!");
        drawInputBox("ENTER YOUR NAME:", neonPink, sub);
    }
}

void Pong::handleEnterP1(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            pCpy(p1Name, inputBuffer);
            for (int i = 0; p1Name[i]; i++) if (p1Name[i] >= 'a' && p1Name[i] <= 'z') p1Name[i] -= 32;
            state = PongState::ENTER_P2; inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}
void Pong::handleEnterP2(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            pCpy(p2Name, inputBuffer);
            for (int i = 0; p2Name[i]; i++) if (p2Name[i] >= 'a' && p2Name[i] <= 'z') p2Name[i] -= 32;
            p1Wins = 0; p2Wins = 0; totalMatches = 0;
            resetMatch();
            startCountdown();
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  PLAYING
// ??????????????????????????????????????????????????????????????????????????????
void Pong::handlePlaying(float dt) {
    // ?? P1 controls (W/S) ????????????????????????????????????????????????????
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        float ny = p1Paddle.getPosition().y - PADDLE_SPEED * dt;
        if (ny < 74.f) ny = 74.f;
        p1Paddle.setPosition(p1Paddle.getPosition().x, ny);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        float ny = p1Paddle.getPosition().y + PADDLE_SPEED * dt;
        if (ny + PADDLE_H > H) ny = H - PADDLE_H;
        p1Paddle.setPosition(p1Paddle.getPosition().x, ny);
    }

    // ?? P2 controls (Up/Down) ????????????????????????????????????????????????
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        float ny = p2Paddle.getPosition().y - PADDLE_SPEED * dt;
        if (ny < 74.f) ny = 74.f;
        p2Paddle.setPosition(p2Paddle.getPosition().x, ny);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        float ny = p2Paddle.getPosition().y + PADDLE_SPEED * dt;
        if (ny + PADDLE_H > H) ny = H - PADDLE_H;
        p2Paddle.setPosition(p2Paddle.getPosition().x, ny);
    }

    // ?? Ball movement ????????????????????????????????????????????????????????
    float bx = ball.getPosition().x + ballVx * dt;
    float by = ball.getPosition().y + ballVy * dt;

    // Top / bottom wall bounce
    if (by - BALL_RADIUS < 74.f) { by = 74.f + BALL_RADIUS; ballVy = -ballVy; }
    if (by + BALL_RADIUS > H) { by = H - BALL_RADIUS;    ballVy = -ballVy; }

    // P1 paddle collision
    sf::FloatRect p1r = p1Paddle.getGlobalBounds();
    if (ballVx<0 &&
        bx - BALL_RADIUS < p1r.left + p1r.width &&
        bx - BALL_RADIUS > p1r.left &&
        by > p1r.top && by < p1r.top + p1r.height) {
        bx = p1r.left + p1r.width + BALL_RADIUS;
        ballVx = -ballVx;
        ballSpeed += SPEED_INC;
        sndHit.play();
        // Angle based on where ball hits paddle
        float rel = (by - (p1r.top + p1r.height / 2.f)) / (p1r.height / 2.f);
        ballVy = ballSpeed * rel * 0.85f;
        ballVx = ballSpeed * (ballVx > 0 ? 1.f : -1.f);
        // Normalise
        float mag = std::sqrt(ballVx * ballVx + ballVy * ballVy);
        ballVx = ballVx / mag * ballSpeed;
        ballVy = ballVy / mag * ballSpeed;
    }

    // P2 paddle collision
    sf::FloatRect p2r = p2Paddle.getGlobalBounds();
    if (ballVx > 0 &&
        bx + BALL_RADIUS > p2r.left &&
        bx + BALL_RADIUS < p2r.left + p2r.width &&
        by > p2r.top && by < p2r.top + p2r.height) {
        bx = p2r.left - BALL_RADIUS;
        ballVx = -ballVx;
        ballSpeed += SPEED_INC;
        sndHit.play();
        float rel = (by - (p2r.top + p2r.height / 2.f)) / (p2r.height / 2.f);
        ballVy = ballSpeed * rel * 0.85f;
        ballVx = ballSpeed * (ballVx > 0 ? 1.f : -1.f);
        float mag = std::sqrt(ballVx * ballVx + ballVy * ballVy);
        ballVx = ballVx / mag * ballSpeed;
        ballVy = ballVy / mag * ballSpeed;
    }

    ball.setPosition(bx, by);

    // Trail update
    trail[trailIdx] = ball.getPosition();
    trailIdx = (trailIdx + 1) % TRAIL_LEN;

    // ?? Scoring ??????????????????????????????????????????????????????????????
    if (bx < 0.f) {
        p2Score++; lastScorer = 2;
        sndMiss.play();
        state = PongState::POINT_SCORED;
        pointFlashTimer = 0.f;
        if (p2Score >= WIN_SCORE) { p2Wins++; totalMatches++; sndWin.play(); state = PongState::MATCH_OVER; }
    }
    if (bx > W) {
        p1Score++; lastScorer = 1;
        sndMiss.play();
        state = PongState::POINT_SCORED;
        pointFlashTimer = 0.f;
        if (p1Score >= WIN_SCORE) { p1Wins++; totalMatches++; sndWin.play(); state = PongState::MATCH_OVER; }
    }
}

void Pong::drawPlaying() {
    drawField();
    drawPaddles();
    drawBall();
    drawScoreBar();
    centeredText("[ ESC ]  MENU", 870.f, 12, sf::Color(80, 80, 120));
}

// ??????????????????????????????????????????????????????????????????????????????
//  POINT SCORED
// ??????????????????????????????????????????????????????????????????????????????
void Pong::drawPointScored() {
    drawField();
    drawPaddles();
    drawScoreBar();

    // Flash overlay on scorer's side
    sf::Color flashCol = (lastScorer == 1) ? sf::Color(0, 240, 255, 30) : sf::Color(255, 30, 180, 30);
    sf::RectangleShape flash({ W / 2.f,H });
    flash.setFillColor(flashCol);
    flash.setPosition((lastScorer == 1) ? 0.f : W / 2.f, 0.f);
    window->draw(flash);

    char buf[64];
    pCpy(buf, (lastScorer == 1) ? p1Name : p2Name);
    pCat(buf, " SCORES!");
    sf::Color sc = (lastScorer == 1) ? neonCyan : neonPink;
    centeredText(buf, 420.f, 36, sc, true);
    centeredText("PRESS  ENTER  TO  CONTINUE", 520.f, 18, sf::Color(200, 200, 200, 200));
}

void Pong::handlePointScored(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter) {
        resetBall((lastScorer == 1) ? -1 : 1);
        startCountdown();
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  MATCH OVER
// ??????????????????????????????????????????????????????????????????????????????
void Pong::drawMatchOver() {
    drawField();
    drawPaddles();
    drawScoreBar();

    sf::RectangleShape ov({ W,H }); ov.setFillColor(sf::Color(0, 0, 0, 140)); window->draw(ov);

    sf::RectangleShape card({ 600.f,280.f });
    card.setFillColor(sf::Color(8, 8, 28, 245));
    card.setOutlineThickness(3.f); card.setOutlineColor(neonYellow);
    card.setOrigin(300.f, 140.f); card.setPosition(W / 2.f, 460.f);
    window->draw(card);

    char winBuf[64];
    pCpy(winBuf, (p1Score >= WIN_SCORE) ? p1Name : p2Name);
    pCat(winBuf, " WINS THE MATCH!");
    centeredText(winBuf, 360.f, 26, neonYellow, true);

    char scoreBuf[32];
    char sc1[8], sc2[8]; pIts(p1Score, sc1); pIts(p2Score, sc2);
    pCpy(scoreBuf, sc1); pCat(scoreBuf, " - "); pCat(scoreBuf, sc2);
    centeredText(scoreBuf, 415.f, 22, sf::Color(200, 200, 200));

    centeredText("[ ENTER ]  PLAY AGAIN", 475.f, 18, neonGreen, true);
    centeredText("[ F ]  FINISH  &  SEE RESULTS", 520.f, 18, sf::Color(255, 120, 80), true);
    centeredText("[ ESC ]  BACK TO HUB", 570.f, 13, sf::Color(100, 100, 140));
}

void Pong::handleMatchOver(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Enter) { resetMatch(); startCountdown(); }
        if (e.key.code == sf::Keyboard::F) { state = PongState::FINAL_RESULTS; }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  FINAL RESULTS
// ??????????????????????????????????????????????????????????????????????????????
void Pong::drawFinalResults() {
    float pulse = (std::sin(glowTimer * 2.f) + 1.f) / 2.f;

    sf::CircleShape glow(260.f);
    glow.setFillColor(sf::Color(255, 200, 0, (sf::Uint8)(12 + 10 * pulse)));
    glow.setOrigin(260.f, 260.f); glow.setPosition(W / 2.f, 420.f);
    window->draw(glow);

    centeredText("FINAL RESULTS", 100.f, 48, neonYellow, true);
    float uw = 380.f + 40.f * pulse;
    sf::RectangleShape ul({ uw,3.f });
    ul.setFillColor(neonYellow); ul.setOrigin(uw / 2.f, 1.5f);
    ul.setPosition(W / 2.f, 155.f); window->draw(ul);

    char tgBuf[48]; pCpy(tgBuf, "TOTAL MATCHES PLAYED: ");
    char tgn[8]; pIts(totalMatches, tgn); pCat(tgBuf, tgn);
    centeredText(tgBuf, 198.f, 16, sf::Color(160, 160, 200));

    // P1 card
    sf::RectangleShape c1({ 380.f,180.f });
    c1.setFillColor(sf::Color(0, 240, 255, 18));
    c1.setOutlineThickness(2.f); c1.setOutlineColor(neonCyan);
    c1.setOrigin(190.f, 90.f); c1.setPosition(340.f, 370.f);
    window->draw(c1);
    sf::Text n1; n1.setFont(font); n1.setCharacterSize(22);
    n1.setFillColor(neonCyan); n1.setStyle(sf::Text::Bold);
    n1.setString(p1Name);
    sf::FloatRect nb1 = n1.getLocalBounds();
    n1.setOrigin(nb1.width / 2.f, nb1.height / 2.f);
    n1.setPosition(340.f, 305.f); window->draw(n1);

    char w1[8]; pIts(p1Wins, w1);
    sf::Text wt1; wt1.setFont(font); wt1.setCharacterSize(64);
    wt1.setFillColor(neonCyan); wt1.setStyle(sf::Text::Bold);
    wt1.setString(w1);
    sf::FloatRect wb1 = wt1.getLocalBounds();
    wt1.setOrigin(wb1.width / 2.f, wb1.height / 2.f);
    wt1.setPosition(340.f, 378.f); window->draw(wt1);
    sf::Text wl1; wl1.setFont(font); wl1.setCharacterSize(14);
    wl1.setFillColor(sf::Color(0, 200, 220, 200)); wl1.setString("MATCHES WON");
    sf::FloatRect wlb = wl1.getLocalBounds();
    wl1.setOrigin(wlb.width / 2.f, wlb.height / 2.f);
    wl1.setPosition(340.f, 445.f); window->draw(wl1);

    // P2 card
    sf::RectangleShape c2({ 380.f,180.f });
    c2.setFillColor(sf::Color(255, 30, 180, 18));
    c2.setOutlineThickness(2.f); c2.setOutlineColor(neonPink);
    c2.setOrigin(190.f, 90.f); c2.setPosition(860.f, 370.f);
    window->draw(c2);
    sf::Text n2; n2.setFont(font); n2.setCharacterSize(22);
    n2.setFillColor(neonPink); n2.setStyle(sf::Text::Bold);
    n2.setString(p2Name);
    sf::FloatRect nb2 = n2.getLocalBounds();
    n2.setOrigin(nb2.width / 2.f, nb2.height / 2.f);
    n2.setPosition(860.f, 305.f); window->draw(n2);

    char w2[8]; pIts(p2Wins, w2);
    sf::Text wt2; wt2.setFont(font); wt2.setCharacterSize(64);
    wt2.setFillColor(neonPink); wt2.setStyle(sf::Text::Bold);
    wt2.setString(w2);
    sf::FloatRect wb2 = wt2.getLocalBounds();
    wt2.setOrigin(wb2.width / 2.f, wb2.height / 2.f);
    wt2.setPosition(860.f, 378.f); window->draw(wt2);
    sf::Text wl2; wl2.setFont(font); wl2.setCharacterSize(14);
    wl2.setFillColor(sf::Color(220, 0, 160, 200)); wl2.setString("MATCHES WON");
    sf::FloatRect wl2b = wl2.getLocalBounds();
    wl2.setOrigin(wl2b.width / 2.f, wl2b.height / 2.f);
    wl2.setPosition(860.f, 445.f); window->draw(wl2);

    // Champion
    char banBuf[64];
    if (p1Wins > p2Wins) { pCpy(banBuf, p1Name); pCat(banBuf, " IS THE CHAMPION!"); }
    else if (p2Wins > p1Wins) { pCpy(banBuf, p2Name); pCat(banBuf, " IS THE CHAMPION!"); }
    else pCpy(banBuf, "PERFECTLY BALANCED!");
    centeredText(banBuf, 535.f, 26, neonYellow, true);

    centeredText("[ ENTER ]  NEW SESSION", 635.f, 17, neonGreen);
    centeredText("[ ESC ]  BACK TO HUB", 685.f, 14, sf::Color(100, 100, 140));
}

void Pong::handleFinalResults(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter) {
        p1Wins = 0; p2Wins = 0; totalMatches = 0;
        inputBuffer[0] = '\0'; inputLen = 0;
        state = PongState::ENTER_P1;
    }
}

// ??? Main Run Loop ????????????????????????????????????????????????????????????
void Pong::run() {
    sf::Clock clock;

    while (window->isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f; // cap delta
        glowTimer += dt;

        sf::Event event;
        while (window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) window->close();

            // Global ESC ? back to hub
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape &&
                state != PongState::PLAYING)
                return;

            switch (state) {
            case PongState::WELCOME:       handleWelcome(event);      break;
            case PongState::ENTER_P1:      handleEnterP1(event);      break;
            case PongState::ENTER_P2:      handleEnterP2(event);      break;
            case PongState::POINT_SCORED:  handlePointScored(event);  break;
            case PongState::MATCH_OVER:    handleMatchOver(event);    break;
            case PongState::FINAL_RESULTS: handleFinalResults(event); break;
            default: break;
            }
        }

        // Countdown logic
        if (state == PongState::COUNTDOWN) {
            countdownTimer += dt;
            if (countdownTimer >= 1.f) {
                countdownTimer = 0.f;
                countdownNum--;
                if (countdownNum < 0) {
                    state = PongState::PLAYING;
                    resetBall((glowTimer > 0) ? 1 : -1);
                }
            }
        }

        // Physics update
        if (state == PongState::PLAYING)
            handlePlaying(dt);

        drawBackground();

        switch (state) {
        case PongState::WELCOME:       drawWelcome();       break;
        case PongState::ENTER_P1:      drawEnterName(1);    break;
        case PongState::ENTER_P2:      drawEnterName(2);    break;
        case PongState::COUNTDOWN:     drawCountdown();     break;
        case PongState::PLAYING:       drawPlaying();       break;
        case PongState::POINT_SCORED:  drawPointScored();   break;
        case PongState::MATCH_OVER:    drawMatchOver();     break;
        case PongState::FINAL_RESULTS: drawFinalResults();  break;
        }

        window->display();
    }
}

// ??? getResult ????????????????????????????????????????????????????????????????
void Pong::getResult(char* p1Out, int& p1WinsOut, int& p2WinsOut,
    char* p2Out, int& totalMatchesOut) const
{
    pCpy(p1Out, p1Name);
    pCpy(p2Out, p2Name);
    p1WinsOut = p1Wins;
    p2WinsOut = p2Wins;
    totalMatchesOut = totalMatches;
}