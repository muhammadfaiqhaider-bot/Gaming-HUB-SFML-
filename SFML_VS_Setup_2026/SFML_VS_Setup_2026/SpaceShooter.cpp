#include "SpaceShooter.h"
#include <cmath>

// ??? Constants ????????????????????????????????????????????????????????????????
const float SpaceShooter::W = 1200.f;
const float SpaceShooter::H = 900.f;

// ??? String helpers ???????????????????????????????????????????????????????????
static int  sLen(const char* s) { int i = 0; while (s[i])i++; return i; }
static void sCpy(char* d, const char* s) { int i = 0; while ((d[i] = s[i]))i++; }
static void sCat(char* d, const char* s) { int i = sLen(d), j = 0; while ((d[i++] = s[j++])); }
static void sIts(int v, char* b) {
    if (v == 0) { b[0] = '0'; b[1] = '\0'; return; }
    char t[12]; int i = 0;
    while (v > 0) { t[i++] = '0' + v % 10; v /= 10; }
    for (int j = 0; j < i; j++)b[j] = t[i - 1 - j]; b[i] = '\0';
}
static float sRand(float lo, float hi, float seed) {
    float f = std::sin(seed * 127.3f + 13.7f) * 43758.5f;
    f = f - (int)f; if (f < 0)f += 1.f;
    return lo + f * (hi - lo);
}

// ??? Struct constructors ??????????????????????????????????????????????????????
Enemy::Enemy() :vx(0), vy(0), alive(false), hp(1), type(0) {}
Bullet::Bullet() :vx(0), vy(0), alive(false), owner(1) {}
Particle::Particle() :vx(0), vy(0), life(0), maxLife(1), alive(false) {}
Ship::Ship() :x(0), y(0), speed(320.f), hp(3), maxHp(3),
shootCooldown(0), alive(true), score(0) {
    name[0] = '\0';
}

// ??? Constructor ??????????????????????????????????????????????????????????????
SpaceShooter::SpaceShooter(sf::RenderWindow* win)
    : window(win), glowTimer(0.f),
    state(SSState::WELCOME), mode(SSMode::NONE),
    wave(0), waveTimer(0.f), enemySpawnTimer(0.f),
    enemySpawnInterval(1.2f), enemiesThisWave(0),
    enemiesSpawned(0), waveClear(false),
    inputLen(0), countdownTimer(0.f), countdownNum(3),
    gameOverTimer(0.f)
{
    neonCyan = sf::Color(0, 240, 255);
    neonPink = sf::Color(255, 30, 180);
    neonYellow = sf::Color(255, 220, 0);
    neonGreen = sf::Color(0, 255, 120);
    neonOrange = sf::Color(255, 120, 0);
    darkBg = sf::Color(4, 4, 16);

    inputBuffer[0] = '\0';
    font.loadFromFile("Orbitron-VariableFont_wght.ttf");

    initScanlines();
    initStars();
    initShips();
}

// ??? Scanlines ????????????????????????????????????????????????????????????????
void SpaceShooter::initScanlines() {
    for (int i = 0; i < 120; i++) {
        scanlines[i].setSize({ W,1.f });
        scanlines[i].setPosition(0.f, i * 7.5f);
        scanlines[i].setFillColor(sf::Color(0, 0, 0, 15));
    }
}

// ??? Stars ????????????????????????????????????????????????????????????????????
void SpaceShooter::initStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].x = sRand(0, W, (float)i * 1.3f);
        stars[i].y = sRand(0, H, (float)i * 2.7f);
        stars[i].speed = sRand(30.f, 140.f, (float)i * 0.9f);
        stars[i].brightness = sRand(60.f, 200.f, (float)i * 3.1f);
    }
}

void SpaceShooter::updateStars(float dt) {
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].y += stars[i].speed * dt;
        if (stars[i].y > H) {
            stars[i].y = 0.f;
            stars[i].x = sRand(0, W, glowTimer * (i + 1));
        }
    }
}

// ??? Ships ????????????????????????????????????????????????????????????????????
void SpaceShooter::initShips() {
    // P1 ship shape (cyan)
    ships[0].body.setPointCount(5);
    ships[0].color = neonCyan;
    sCpy(ships[0].name, "PLAYER 1");

    // P2 ship shape (pink)
    ships[1].body.setPointCount(5);
    ships[1].color = neonPink;
    sCpy(ships[1].name, "PLAYER 2");
}

void SpaceShooter::resetGame() {
    wave = 1; waveTimer = 0.f; enemySpawnTimer = 0.f;
    enemySpawnInterval = 1.4f; enemiesThisWave = 8;
    enemiesSpawned = 0; waveClear = false;

    // Kill all entities
    for (int i = 0; i < MAX_ENEMIES; i++)  enemies[i].alive = false;
    for (int i = 0; i < MAX_BULLETS; i++)  bullets[i].alive = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].alive = false;

    // Reset ships
    ships[0].hp = 3; ships[0].maxHp = 3;
    ships[0].score = 0; ships[0].alive = true;
    ships[0].shootCooldown = 0.f;

    ships[1].hp = 3; ships[1].maxHp = 3;
    ships[1].score = 0; ships[1].alive = true;
    ships[1].shootCooldown = 0.f;

    bool twoPlayers = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);

    ships[0].x = twoPlayers ? W * 0.35f : W / 2.f;
    ships[0].y = H - 120.f;

    ships[1].x = W * 0.65f;
    ships[1].y = H - 120.f;
}

// ??? Build ship convex shape at position ?????????????????????????????????????
static void buildShip(sf::ConvexShape& body, sf::ConvexShape& flame,
    float cx, float cy, sf::Color col) {
    body.setPointCount(7);
    body.setPoint(0, { cx,      cy - 36.f }); // nose
    body.setPoint(1, { cx + 10.f, cy - 10.f });
    body.setPoint(2, { cx + 22.f, cy + 10.f });
    body.setPoint(3, { cx + 14.f, cy + 28.f });
    body.setPoint(4, { cx - 14.f, cy + 28.f });
    body.setPoint(5, { cx - 22.f, cy + 10.f });
    body.setPoint(6, { cx - 10.f, cy - 10.f });
    body.setFillColor(sf::Color(col.r / 6, col.g / 6, col.b / 6, 240));
    body.setOutlineThickness(2.f);
    body.setOutlineColor(col);

    flame.setPointCount(3);
    flame.setPoint(0, { cx,      cy + 46.f });
    flame.setPoint(1, { cx + 8.f,  cy + 28.f });
    flame.setPoint(2, { cx - 8.f,  cy + 28.f });
    float flicker = 0.6f + 0.4f * std::sin(cx * 0.3f);
    flame.setFillColor(sf::Color(255, (sf::Uint8)(100 + 80 * flicker), 0,
        (sf::Uint8)(180 + 75 * flicker)));
}

// ??? Spawning ?????????????????????????????????????????????????????????????????
int SpaceShooter::findFreeEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) if (!enemies[i].alive) return i;
    return -1;
}
int SpaceShooter::findFreeBullet() {
    for (int i = 0; i < MAX_BULLETS; i++) if (!bullets[i].alive) return i;
    return -1;
}
int SpaceShooter::findFreeParticle() {
    for (int i = 0; i < MAX_PARTICLES; i++) if (!particles[i].alive) return i;
    return -1;
}

void SpaceShooter::spawnEnemy() {
    int idx = findFreeEnemy(); if (idx < 0) return;
    Enemy& e = enemies[idx];
    e.alive = true;

    // Type based on wave
    float r = sRand(0, 1, glowTimer + (float)idx);
    if (wave >= 4 && r > 0.7f)      e.type = 2; // tank
    else if (wave >= 2 && r > 0.5f) e.type = 1; // fast
    else                        e.type = 0; // basic

    e.hp = (e.type == 2) ? 3 : (e.type == 1) ? 1 : 2;

    float spawnX = sRand(40.f, W - 40.f, glowTimer * 1.7f + (float)idx);
    e.shape.setPointCount(6);
    e.shape.setPoint(0, { 0,-20.f });
    e.shape.setPoint(1, { 16.f,-8.f });
    e.shape.setPoint(2, { 16.f,12.f });
    e.shape.setPoint(3, { 0,20.f });
    e.shape.setPoint(4, { -16.f,12.f });
    e.shape.setPoint(5, { -16.f,-8.f });

    sf::Color ec;
    if (e.type == 0) ec = sf::Color(255, 60, 60);
    else if (e.type == 1) ec = sf::Color(255, 160, 0);
    else ec = sf::Color(180, 0, 255);
    e.shape.setFillColor(sf::Color(ec.r / 5, ec.g / 5, ec.b / 5, 220));
    e.shape.setOutlineThickness(2.f);
    e.shape.setOutlineColor(ec);
    e.shape.setPosition(spawnX, -20.f);

    float spd = (e.type == 1) ? 180.f : (e.type == 2) ? 60.f : 100.f;
    spd += wave * 12.f;
    e.vy = spd;
    e.vx = sRand(-30.f, 30.f, glowTimer + (float)idx * 0.5f);
}

void SpaceShooter::spawnExplosion(float x, float y, sf::Color col, int count) {
    for (int i = 0; i < count; i++) {
        int idx = findFreeParticle(); if (idx < 0) break;
        Particle& p = particles[idx];
        p.alive = true;
        p.maxLife = sRand(0.3f, 0.8f, glowTimer + (float)i);
        p.life = p.maxLife;
        float angle = sRand(0, 6.28f, glowTimer + (float)i * 0.7f);
        float spd = sRand(60.f, 220.f, glowTimer + (float)i * 1.3f);
        p.vx = std::cos(angle) * spd;
        p.vy = std::sin(angle) * spd;
        float r = sRand(2.f, 6.f, glowTimer + (float)i * 2.1f);
        p.shape.setRadius(r);
        p.shape.setOrigin(r, r);
        p.shape.setPosition(x, y);
        p.shape.setFillColor(col);
    }
}

void SpaceShooter::fireBullet(int pi) {
    if (!ships[pi].alive) return;
    if (ships[pi].shootCooldown > 0.f) return;
    int idx = findFreeBullet(); if (idx < 0) return;

    Bullet& b = bullets[idx];
    b.alive = true; b.owner = pi + 1;
    float r = 5.f;
    b.shape.setRadius(r); b.shape.setOrigin(r, r);
    b.shape.setPosition(ships[pi].x, ships[pi].y - 36.f);
    b.shape.setFillColor(pi == 0 ? neonCyan : neonPink);
    b.vx = 0.f; b.vy = -700.f;

    ships[pi].shootCooldown = 0.18f;

    // Muzzle flash
    spawnExplosion(ships[pi].x, ships[pi].y - 36.f,
        pi == 0 ? neonCyan : neonPink, 4);
}

// ??? Update ???????????????????????????????????????????????????????????????????
void SpaceShooter::updateShips(float dt) {
    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);

    // P1 controls: WASD + Space
    if (ships[0].alive) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            ships[0].x -= ships[0].speed * dt;
            if (ships[0].x < 22.f) ships[0].x = 22.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            ships[0].x += ships[0].speed * dt;
            if (ships[0].x > W - 22.f) ships[0].x = W - 22.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            ships[0].y -= ships[0].speed * dt;
            if (ships[0].y < 80.f) ships[0].y = 80.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            ships[0].y += ships[0].speed * dt;
            if (ships[0].y > H - 30.f) ships[0].y = H - 30.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            fireBullet(0);
        ships[0].shootCooldown -= dt;
    }

    // P2 controls: Arrows + Enter (only in 2P modes)
    if (two && ships[1].alive) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            ships[1].x -= ships[1].speed * dt;
            if (ships[1].x < 22.f) ships[1].x = 22.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            ships[1].x += ships[1].speed * dt;
            if (ships[1].x > W - 22.f) ships[1].x = W - 22.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            ships[1].y -= ships[1].speed * dt;
            if (ships[1].y < 80.f) ships[1].y = 80.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            ships[1].y += ships[1].speed * dt;
            if (ships[1].y > H - 30.f) ships[1].y = H - 30.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
            fireBullet(1);
        ships[1].shootCooldown -= dt;
    }
}

void SpaceShooter::updateBullets(float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].alive) continue;
        sf::Vector2f pos = bullets[i].shape.getPosition();
        pos.x += bullets[i].vx * dt;
        pos.y += bullets[i].vy * dt;
        bullets[i].shape.setPosition(pos);
        if (pos.y<-20.f || pos.y>H + 20.f) bullets[i].alive = false;
    }
}

void SpaceShooter::updateEnemies(float dt) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        sf::Vector2f pos = enemies[i].shape.getPosition();
        pos.x += enemies[i].vx * dt;
        pos.y += enemies[i].vy * dt;
        // Bounce off walls
        if (pos.x<20.f || pos.x>W - 20.f) enemies[i].vx = -enemies[i].vx;
        enemies[i].shape.setPosition(pos);
        if (pos.y > H + 40.f) enemies[i].alive = false;
    }
}

void SpaceShooter::updateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].alive) continue;
        particles[i].life -= dt;
        if (particles[i].life <= 0.f) { particles[i].alive = false; continue; }
        sf::Vector2f pos = particles[i].shape.getPosition();
        pos.x += particles[i].vx * dt;
        pos.y += particles[i].vy * dt;
        particles[i].vy += 80.f * dt; // gravity
        particles[i].shape.setPosition(pos);
        float a = particles[i].life / particles[i].maxLife;
        sf::Color c = particles[i].shape.getFillColor();
        c.a = (sf::Uint8)(a * 220.f);
        particles[i].shape.setFillColor(c);
    }
}

void SpaceShooter::checkCollisions() {
    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);

    for (int bi = 0; bi < MAX_BULLETS; bi++) {
        if (!bullets[bi].alive) continue;
        sf::Vector2f bp = bullets[bi].shape.getPosition();

        for (int ei = 0; ei < MAX_ENEMIES; ei++) {
            if (!enemies[ei].alive) continue;
            sf::Vector2f ep = enemies[ei].shape.getPosition();
            float dx = bp.x - ep.x, dy = bp.y - ep.y;
            if (std::sqrt(dx * dx + dy * dy) < 22.f) {
                bullets[bi].alive = false;
                enemies[ei].hp--;

                sf::Color ec = enemies[ei].shape.getOutlineColor();
                spawnExplosion(ep.x, ep.y, ec, 8);

                if (enemies[ei].hp <= 0) {
                    enemies[ei].alive = false;
                    int pts = (enemies[ei].type == 2) ? 30 : (enemies[ei].type == 1) ? 15 : 10;
                    // Award points
                    int owner = bullets[bi].owner - 1;
                    if (owner >= 0 && owner < 2) ships[owner].score += pts;
                    else ships[0].score += pts;
                    spawnExplosion(ep.x, ep.y, ec, 16);
                }
                break;
            }
        }
    }

    // Enemy hits ship
    for (int ei = 0; ei < MAX_ENEMIES; ei++) {
        if (!enemies[ei].alive) continue;
        sf::Vector2f ep = enemies[ei].shape.getPosition();

        for (int pi = 0; pi < (two ? 2 : 1); pi++) {
            if (!ships[pi].alive) continue;
            float dx = ep.x - ships[pi].x, dy = ep.y - ships[pi].y;
            if (std::sqrt(dx * dx + dy * dy) < 30.f) {
                enemies[ei].alive = false;
                ships[pi].hp--;
                spawnExplosion(ships[pi].x, ships[pi].y,
                    ships[pi].color, 12);
                if (ships[pi].hp <= 0) {
                    ships[pi].alive = false;
                    spawnExplosion(ships[pi].x, ships[pi].y,
                        ships[pi].color, 30);
                }
            }
        }
    }
}

void SpaceShooter::updateWave(float dt) {
    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);

    // Check game over
    bool anyAlive = ships[0].alive || (two && ships[1].alive);
    if (!anyAlive) {
        gameOverTimer += dt;
        if (gameOverTimer > 2.f) state = SSState::GAME_OVER;
        return;
    }

    // Spawn enemies
    if (enemiesSpawned < enemiesThisWave) {
        enemySpawnTimer += dt;
        if (enemySpawnTimer >= enemySpawnInterval) {
            enemySpawnTimer = 0.f;
            spawnEnemy();
            enemiesSpawned++;
        }
    }

    // Check wave clear
    bool allDead = true;
    for (int i = 0; i < MAX_ENEMIES; i++) if (enemies[i].alive) { allDead = false; break; }
    if (allDead && enemiesSpawned >= enemiesThisWave) {
        waveTimer += dt;
        if (waveTimer > 2.5f) {
            wave++;
            waveTimer = 0.f;
            enemiesSpawned = 0;
            enemiesThisWave = 6 + wave * 3;
            enemySpawnInterval = 1.4f - wave * 0.08f;
            if (enemySpawnInterval < 0.3f) enemySpawnInterval = 0.3f;
            // Bonus HP on wave clear
            for (int p = 0; p < (two ? 2 : 1); p++)
                if (ships[p].alive && ships[p].hp < ships[p].maxHp)
                    ships[p].hp++;
        }
    }
}

// ??? Drawing ??????????????????????????????????????????????????????????????????
void SpaceShooter::drawBackground() {
    window->clear(darkBg);
    for (int i = 0; i < 120; i++) window->draw(scanlines[i]);
}

void SpaceShooter::drawStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
        sf::CircleShape s(stars[i].speed > 100.f ? 1.5f : 1.f);
        s.setFillColor(sf::Color(255, 255, 255, (sf::Uint8)stars[i].brightness));
        s.setPosition(stars[i].x, stars[i].y);
        window->draw(s);
    }
}

void SpaceShooter::drawShip(int idx) {
    if (!ships[idx].alive) return;
    float cx = ships[idx].x, cy = ships[idx].y;
    buildShip(ships[idx].body, ships[idx].flame, cx, cy, ships[idx].color);
    window->draw(ships[idx].flame);
    window->draw(ships[idx].body);
}

void SpaceShooter::drawEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        float pulse = (std::sin(glowTimer * 5.f + (float)i) + 1.f) / 2.f;
        sf::Color oc = enemies[i].shape.getOutlineColor();
        oc.a = (sf::Uint8)(160 + 95 * pulse);
        enemies[i].shape.setOutlineColor(oc);
        window->draw(enemies[i].shape);
    }
}

void SpaceShooter::drawBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].alive) continue;
        // Glow
        sf::CircleShape glow(10.f);
        glow.setOrigin(10.f, 10.f);
        glow.setPosition(bullets[i].shape.getPosition());
        sf::Color gc = bullets[i].shape.getFillColor(); gc.a = 40;
        glow.setFillColor(gc);
        window->draw(glow);
        window->draw(bullets[i].shape);
    }
}

void SpaceShooter::drawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (particles[i].alive) window->draw(particles[i].shape);
}

void SpaceShooter::drawHUD() {
    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);

    // Top bar
    sf::RectangleShape bar({ W,68.f });
    bar.setFillColor(sf::Color(6, 6, 22, 220));
    bar.setPosition(0, 0); window->draw(bar);
    sf::RectangleShape barLine({ W,2.f });
    barLine.setFillColor(sf::Color(0, 240, 255, 50));
    barLine.setPosition(0, 68.f); window->draw(barLine);

    // Wave info
    char wbuf[32]; sCpy(wbuf, "WAVE "); char wn[8]; sIts(wave, wn); sCat(wbuf, wn);
    sf::Text wt; wt.setFont(font); wt.setCharacterSize(16);
    wt.setFillColor(sf::Color(200, 200, 220)); wt.setStyle(sf::Text::Bold);
    wt.setString(wbuf);
    sf::FloatRect wb = wt.getLocalBounds();
    wt.setOrigin(wb.width / 2.f, wb.height / 2.f);
    wt.setPosition(W / 2.f, 26.f); window->draw(wt);

    // Mode label
    const char* ml = (mode == SSMode::SINGLE_PLAYER) ? "SOLO" : (mode == SSMode::COMPETITIVE) ? "VS" : "CO-OP";
    sf::Text mt; mt.setFont(font); mt.setCharacterSize(11);
    mt.setFillColor(sf::Color(140, 140, 180, 180)); mt.setString(ml);
    sf::FloatRect mb = mt.getLocalBounds();
    mt.setOrigin(mb.width / 2.f, 0.f);
    mt.setPosition(W / 2.f, 46.f); window->draw(mt);

    // P1 HUD (left)
    auto drawPlayerHud = [&](int pi, float startX) {
        sf::Text nt; nt.setFont(font); nt.setCharacterSize(15);
        nt.setFillColor(ships[pi].color); nt.setStyle(sf::Text::Bold);
        nt.setString(ships[pi].name);
        nt.setPosition(startX, 8.f); window->draw(nt);

        // HP hearts
        for (int h = 0; h < ships[pi].maxHp; h++) {
            sf::CircleShape heart(7.f);
            heart.setPosition(startX + h * 22.f, 32.f);
            bool filled = (h < ships[pi].hp && ships[pi].alive);
            heart.setFillColor(filled ? ships[pi].color : sf::Color(40, 40, 60));
            heart.setOutlineThickness(1.5f);
            heart.setOutlineColor(ships[pi].color);
            window->draw(heart);
        }

        // Score
        char sbuf[32]; sCpy(sbuf, "SCORE: ");
        char sn[12]; sIts(ships[pi].score, sn); sCat(sbuf, sn);
        sf::Text st; st.setFont(font); st.setCharacterSize(12);
        st.setFillColor(sf::Color(180, 180, 200));
        st.setString(sbuf);
        st.setPosition(startX, 50.f); window->draw(st);
        };

    drawPlayerHud(0, 30.f);
    if (two) drawPlayerHud(1, W - 280.f);

    // ESC hint
    sf::Text esc; esc.setFont(font); esc.setCharacterSize(11);
    esc.setFillColor(sf::Color(80, 80, 110));
    esc.setString("[ ESC ] MENU");
    sf::FloatRect eb = esc.getLocalBounds();
    esc.setOrigin(eb.width / 2.f, 0.f);
    esc.setPosition(W / 2.f, 876.f); window->draw(esc);
}

void SpaceShooter::drawWaveBanner() {
    bool allDead = true;
    for (int i = 0; i < MAX_ENEMIES; i++) if (enemies[i].alive) { allDead = false; break; }
    if (allDead && enemiesSpawned >= enemiesThisWave && waveTimer < 2.5f) {
        float alpha = 1.f;
        if (waveTimer > 1.8f) alpha = (2.5f - waveTimer) / 0.7f;
        char buf[32]; sCpy(buf, "WAVE "); char wn[8]; sIts(wave, wn); sCat(buf, wn);
        sCat(buf, " CLEAR!");
        sf::Text bt; bt.setFont(font); bt.setCharacterSize(48);
        bt.setFillColor(sf::Color(0, 255, 120, (sf::Uint8)(alpha * 220)));
        bt.setStyle(sf::Text::Bold); bt.setString(buf);
        sf::FloatRect bb = bt.getLocalBounds();
        bt.setOrigin(bb.width / 2.f, bb.height / 2.f);
        bt.setPosition(W / 2.f, H / 2.f); window->draw(bt);

        char nb[32]; sCpy(nb, "NEXT: WAVE "); sIts(wave + 1, wn); sCat(nb, wn);
        sf::Text nt; nt.setFont(font); nt.setCharacterSize(20);
        nt.setFillColor(sf::Color(200, 200, 100, (sf::Uint8)(alpha * 180)));
        nt.setString(nb);
        sf::FloatRect nbb = nt.getLocalBounds();
        nt.setOrigin(nbb.width / 2.f, nbb.height / 2.f);
        nt.setPosition(W / 2.f, H / 2.f + 60.f); window->draw(nt);
    }
}

// ??? Centered text ????????????????????????????????????????????????????????????
void SpaceShooter::centeredText(const char* s, float y, unsigned size, sf::Color col, bool bold) {
    sf::Text t; t.setFont(font); t.setString(s);
    t.setCharacterSize(size); t.setFillColor(col);
    if (bold) t.setStyle(sf::Text::Bold);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.width / 2.f, b.height / 2.f);
    t.setPosition(W / 2.f, y); window->draw(t);
}

// ??? Input box ????????????????????????????????????????????????????????????????
void SpaceShooter::drawInputBox(const char* prompt, sf::Color col, const char* sub) {
    sf::RectangleShape panel({ 640.f,sub ? 280.f : 240.f });
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
    char display[36]; sCpy(display, inputBuffer);
    float pulse = (std::sin(glowTimer * 4.f) + 1.f) / 2.f;
    if (pulse > 0.5f) sCat(display, "|");
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
void SpaceShooter::drawWelcome() {
    float pulse = (std::sin(glowTimer * 1.8f) + 1.f) / 2.f;
    sf::Uint8 r = (sf::Uint8)(pulse * 50);
    centeredText("SPACE SHOOTER", 260.f, 68, sf::Color(r, 255, 120), true);

    // Decorative rocket
    sf::ConvexShape rk; rk.setPointCount(5);
    float cx = W / 2.f, cy = 470.f;
    rk.setPoint(0, { cx,cy - 50.f }); rk.setPoint(1, { cx + 18.f,cy });
    rk.setPoint(2, { cx + 12.f,cy + 38.f }); rk.setPoint(3, { cx - 12.f,cy + 38.f });
    rk.setPoint(4, { cx - 18.f,cy });
    rk.setFillColor(sf::Color(0, 30, 20, 200));
    rk.setOutlineThickness(2.5f); rk.setOutlineColor(neonGreen);
    window->draw(rk);
    sf::ConvexShape fl; fl.setPointCount(3);
    fl.setPoint(0, { cx,cy + 60.f }); fl.setPoint(1, { cx + 10.f,cy + 38.f });
    fl.setPoint(2, { cx - 10.f,cy + 38.f });
    float fp = (std::sin(glowTimer * 8.f) + 1.f) / 2.f;
    fl.setFillColor(sf::Color(255, (sf::Uint8)(80 + 80 * fp), 0, 200));
    window->draw(fl);

    float uw = 300.f + 60.f * pulse;
    sf::RectangleShape ul({ uw,3.f });
    ul.setFillColor(sf::Color(0, 255, 120, (sf::Uint8)(130 + 125 * pulse)));
    ul.setOrigin(uw / 2.f, 1.5f); ul.setPosition(W / 2.f, 370.f);
    window->draw(ul);

    centeredText("SURVIVE THE WAVES — DESTROY EVERYTHING", 560.f, 15,
        sf::Color(160, 200, 160, 180));
    sf::Uint8 pa = (sf::Uint8)(140 + 115 * pulse);
    centeredText("PRESS  ENTER  TO  START", 640.f, 22, sf::Color(255, 220, 0, pa), true);
    centeredText("[ ESC ]  BACK TO HUB", 860.f, 13, sf::Color(100, 100, 140));
}

void SpaceShooter::handleWelcome(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter) {
        state = SSState::ENTER_P1;
        inputBuffer[0] = '\0'; inputLen = 0;
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  NAME ENTRY
// ??????????????????????????????????????????????????????????????????????????????
void SpaceShooter::drawEnterName(int player) {
    centeredText("SPACE SHOOTER", 100.f, 38, neonGreen, true);
    if (player == 1) {
        centeredText("PILOT 1", 230.f, 52, neonCyan, true);
        centeredText("Controls:  A D W S  to move   SPACE  to fire",
            300.f, 14, sf::Color(160, 160, 210));
        drawInputBox("ENTER YOUR CALLSIGN:", neonCyan);
    }
    else {
        centeredText("PILOT 2", 230.f, 52, neonPink, true);
        centeredText("Controls:  ARROW KEYS  to move   R-CTRL  to fire",
            300.f, 14, sf::Color(160, 160, 210));
        char sub[48]; sCpy(sub, "P1: "); sCat(sub, ships[0].name); sCat(sub, " ready!");
        drawInputBox("ENTER YOUR CALLSIGN:", neonPink, sub);
    }
}

void SpaceShooter::handleEnterP1(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            sCpy(ships[0].name, inputBuffer);
            for (int i = 0; ships[0].name[i]; i++)
                if (ships[0].name[i] >= 'a' && ships[0].name[i] <= 'z') ships[0].name[i] -= 32;
            state = SSState::MODE_SELECT;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}

void SpaceShooter::handleEnterP2(sf::Event& e) {
    if (e.type == sf::Event::TextEntered) {
        unsigned c = e.text.unicode;
        if (c == 13 && inputLen > 0) {
            sCpy(ships[1].name, inputBuffer);
            for (int i = 0; ships[1].name[i]; i++)
                if (ships[1].name[i] >= 'a' && ships[1].name[i] <= 'z') ships[1].name[i] -= 32;
            resetGame();
            state = SSState::COUNTDOWN;
            countdownTimer = 0.f; countdownNum = 3;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (c == 8 && inputLen > 0) { inputBuffer[--inputLen] = '\0'; }
        else if (c >= 32 && c < 127 && inputLen < 14) { inputBuffer[inputLen++] = (char)c; inputBuffer[inputLen] = '\0'; }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  MODE SELECT
// ??????????????????????????????????????????????????????????????????????????????
void SpaceShooter::drawModeSelect() {
    centeredText("SPACE SHOOTER", 90.f, 38, neonGreen, true);
    centeredText("SELECT  GAME  MODE", 160.f, 20, sf::Color(160, 160, 210, 200));

    float uw = 300.f;
    sf::RectangleShape ul({ uw,2.f });
    ul.setFillColor(neonGreen); ul.setOrigin(uw / 2.f, 1.f);
    ul.setPosition(W / 2.f, 185.f); window->draw(ul);

    struct ModeCard { const char* key; const char* title; const char* desc1; const char* desc2; sf::Color col; };
    ModeCard cards[3] = {
        {"[ 1 ]","SOLO  MISSION","1 Player vs endless waves",
         "Survive as long as possible",neonCyan},
        {"[ 2 ]","HEAD  TO  HEAD","2 Players compete for score",
         "Most kills wins — same keyboard",neonPink},
        {"[ 3 ]","CO-OP  MODE","2 Players survive together",
         "Team up — share the glory",neonGreen}
    };

    float cardW = 300.f, cardH = 200.f;
    float startX = 90.f, gapX = 60.f;
    float cardY = 300.f;

    for (int i = 0; i < 3; i++) {
        float cx = startX + i * (cardW + gapX);
        sf::RectangleShape box({ cardW,cardH });
        box.setFillColor(sf::Color(10, 10, 30, 220));
        box.setOutlineThickness(2.f);
        box.setOutlineColor(sf::Color(cards[i].col.r, cards[i].col.g, cards[i].col.b, 160));
        box.setPosition(cx, cardY); window->draw(box);

        // Left accent
        sf::RectangleShape acc({ 5.f,cardH });
        acc.setFillColor(cards[i].col);
        acc.setPosition(cx, cardY); window->draw(acc);

        // Key
        sf::Text kt; kt.setFont(font); kt.setCharacterSize(22);
        kt.setFillColor(cards[i].col); kt.setStyle(sf::Text::Bold);
        kt.setString(cards[i].key);
        sf::FloatRect kb = kt.getLocalBounds();
        kt.setOrigin(kb.width / 2.f, 0.f);
        kt.setPosition(cx + cardW / 2.f, cardY + 18.f); window->draw(kt);

        // Title
        sf::Text tt; tt.setFont(font); tt.setCharacterSize(16);
        tt.setFillColor(sf::Color::White); tt.setStyle(sf::Text::Bold);
        tt.setString(cards[i].title);
        sf::FloatRect tb = tt.getLocalBounds();
        tt.setOrigin(tb.width / 2.f, 0.f);
        tt.setPosition(cx + cardW / 2.f, cardY + 62.f); window->draw(tt);

        // Desc
        sf::Text d1; d1.setFont(font); d1.setCharacterSize(12);
        d1.setFillColor(sf::Color(180, 180, 200, 180));
        d1.setString(cards[i].desc1);
        sf::FloatRect d1b = d1.getLocalBounds();
        d1.setOrigin(d1b.width / 2.f, 0.f);
        d1.setPosition(cx + cardW / 2.f, cardY + 110.f); window->draw(d1);

        sf::Text d2; d2.setFont(font); d2.setCharacterSize(12);
        d2.setFillColor(sf::Color(140, 140, 160, 160));
        d2.setString(cards[i].desc2);
        sf::FloatRect d2b = d2.getLocalBounds();
        d2.setOrigin(d2b.width / 2.f, 0.f);
        d2.setPosition(cx + cardW / 2.f, cardY + 138.f); window->draw(d2);
    }

    centeredText("[ ESC ]  BACK", 860.f, 13, sf::Color(100, 100, 140));
}

void SpaceShooter::handleModeSelect(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Num1 || e.key.code == sf::Keyboard::Numpad1) {
            mode = SSMode::SINGLE_PLAYER;
            resetGame(); state = SSState::COUNTDOWN;
            countdownTimer = 0.f; countdownNum = 3;
        }
        else if (e.key.code == sf::Keyboard::Num2 || e.key.code == sf::Keyboard::Numpad2) {
            mode = SSMode::COMPETITIVE;
            state = SSState::ENTER_P2;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
        else if (e.key.code == sf::Keyboard::Num3 || e.key.code == sf::Keyboard::Numpad3) {
            mode = SSMode::COOP;
            state = SSState::ENTER_P2;
            inputBuffer[0] = '\0'; inputLen = 0;
        }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  COUNTDOWN
// ??????????????????????????????????????????????????????????????????????????????
void SpaceShooter::drawCountdown() {
    drawStars();
    drawShip(0);
    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);
    if (two) drawShip(1);
    drawHUD();

    char buf[4];
    if (countdownNum > 0) sIts(countdownNum, buf);
    else { buf[0] = 'G'; buf[1] = 'O'; buf[2] = '!'; buf[3] = '\0'; }

    float pulse = (std::sin(glowTimer * 8.f) + 1.f) / 2.f;
    sf::Text ct; ct.setFont(font); ct.setCharacterSize(110);
    ct.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(180 + 75 * pulse)));
    ct.setStyle(sf::Text::Bold); ct.setString(buf);
    sf::FloatRect fb = ct.getLocalBounds();
    ct.setOrigin(fb.width / 2.f, fb.height / 2.f);
    ct.setPosition(W / 2.f, H / 2.f); window->draw(ct);
}

// ??????????????????????????????????????????????????????????????????????????????
//  PLAYING
// ??????????????????????????????????????????????????????????????????????????????
void SpaceShooter::drawPlaying() {
    drawStars();
    drawParticles();
    drawEnemies();
    drawBullets();
    drawShip(0);
    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);
    if (two) drawShip(1);
    drawHUD();
    drawWaveBanner();
}

// ??????????????????????????????????????????????????????????????????????????????
//  GAME OVER
// ??????????????????????????????????????????????????????????????????????????????
void SpaceShooter::drawGameOver() {
    drawStars();
    drawParticles();
    drawHUD();

    sf::RectangleShape ov({ W,H });
    ov.setFillColor(sf::Color(0, 0, 0, 160)); window->draw(ov);

    sf::RectangleShape card({ 640.f,300.f });
    card.setFillColor(sf::Color(8, 8, 28, 245));
    card.setOutlineThickness(3.f); card.setOutlineColor(neonYellow);
    card.setOrigin(320.f, 150.f); card.setPosition(W / 2.f, 440.f);
    window->draw(card);

    centeredText("GAME OVER", 330.f, 52, sf::Color(255, 60, 60), true);

    char waveBuf[32]; sCpy(waveBuf, "SURVIVED TO WAVE "); char wn[8]; sIts(wave, wn); sCat(waveBuf, wn);
    centeredText(waveBuf, 400.f, 18, sf::Color(200, 200, 200));

    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);
    if (two && mode == SSMode::COMPETITIVE) {
        // Show winner
        char winBuf[64];
        if (ships[0].score > ships[1].score) { sCpy(winBuf, ships[0].name); sCat(winBuf, " WINS!"); }
        else if (ships[1].score > ships[0].score) { sCpy(winBuf, ships[1].name); sCat(winBuf, " WINS!"); }
        else sCpy(winBuf, "IT'S A TIE!");
        centeredText(winBuf, 445.f, 22, neonYellow, true);
    }

    centeredText("[ ENTER ]  PLAY AGAIN", 490.f, 18, neonGreen, true);
    centeredText("[ F ]  FINAL RESULTS", 535.f, 18, sf::Color(255, 120, 80), true);
    centeredText("[ ESC ]  BACK TO HUB", 580.f, 13, sf::Color(100, 100, 140));
}

void SpaceShooter::handleGameOver(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Enter) {
            resetGame();
            state = SSState::COUNTDOWN;
            countdownTimer = 0.f; countdownNum = 3;
            gameOverTimer = 0.f;
        }
        if (e.key.code == sf::Keyboard::F) state = SSState::FINAL_RESULTS;
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  FINAL RESULTS
// ??????????????????????????????????????????????????????????????????????????????
void SpaceShooter::drawFinalResults() {
    float pulse = (std::sin(glowTimer * 2.f) + 1.f) / 2.f;
    sf::CircleShape glow(260.f);
    glow.setFillColor(sf::Color(0, 200, 100, (sf::Uint8)(10 + 10 * pulse)));
    glow.setOrigin(260.f, 260.f); glow.setPosition(W / 2.f, 420.f);
    window->draw(glow);

    centeredText("MISSION RESULTS", 95.f, 46, neonGreen, true);
    float uw = 380.f + 40.f * pulse;
    sf::RectangleShape ul({ uw,3.f });
    ul.setFillColor(neonGreen); ul.setOrigin(uw / 2.f, 1.5f);
    ul.setPosition(W / 2.f, 150.f); window->draw(ul);

    bool two = (mode == SSMode::COMPETITIVE || mode == SSMode::COOP);
    char waveBuf[32]; sCpy(waveBuf, "FURTHEST WAVE: "); char wn[8]; sIts(wave, wn); sCat(waveBuf, wn);
    centeredText(waveBuf, 195.f, 16, sf::Color(160, 160, 200));

    auto drawCard = [&](int pi, float posX) {
        sf::RectangleShape c({ 380.f,200.f });
        c.setFillColor(sf::Color(ships[pi].color.r / 12, ships[pi].color.g / 12,
            ships[pi].color.b / 12, 200));
        c.setOutlineThickness(2.f); c.setOutlineColor(ships[pi].color);
        c.setOrigin(190.f, 100.f); c.setPosition(posX, 390.f);
        window->draw(c);

        sf::Text nt; nt.setFont(font); nt.setCharacterSize(20);
        nt.setFillColor(ships[pi].color); nt.setStyle(sf::Text::Bold);
        nt.setString(ships[pi].name);
        sf::FloatRect nb = nt.getLocalBounds();
        nt.setOrigin(nb.width / 2.f, nb.height / 2.f);
        nt.setPosition(posX, 310.f); window->draw(nt);

        char sbuf[16]; sIts(ships[pi].score, sbuf);
        sf::Text sc; sc.setFont(font); sc.setCharacterSize(58);
        sc.setFillColor(ships[pi].color); sc.setStyle(sf::Text::Bold);
        sc.setString(sbuf);
        sf::FloatRect sb = sc.getLocalBounds();
        sc.setOrigin(sb.width / 2.f, sb.height / 2.f);
        sc.setPosition(posX, 388.f); window->draw(sc);

        sf::Text sl; sl.setFont(font); sl.setCharacterSize(13);
        sl.setFillColor(sf::Color(180, 180, 200, 180)); sl.setString("SCORE");
        sf::FloatRect slb = sl.getLocalBounds();
        sl.setOrigin(slb.width / 2.f, slb.height / 2.f);
        sl.setPosition(posX, 453.f); window->draw(sl);
        };

    if (!two) {
        drawCard(0, W / 2.f);
    }
    else {
        drawCard(0, 340.f);
        drawCard(1, 860.f);

        // Winner
        char banBuf[64];
        if (ships[0].score > ships[1].score) { sCpy(banBuf, ships[0].name); sCat(banBuf, " IS THE CHAMPION!"); }
        else if (ships[1].score > ships[0].score) { sCpy(banBuf, ships[1].name); sCat(banBuf, " IS THE CHAMPION!"); }
        else sCpy(banBuf, "EQUALLY LEGENDARY!");
        centeredText(banBuf, 540.f, 24, neonYellow, true);
    }

    centeredText("[ ENTER ]  PLAY AGAIN", 640.f, 17, neonGreen);
    centeredText("[ ESC ]  BACK TO HUB", 690.f, 14, sf::Color(100, 100, 140));
}

void SpaceShooter::handleFinalResults(sf::Event& e) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter) {
        inputBuffer[0] = '\0'; inputLen = 0;
        state = SSState::ENTER_P1;
    }
}

// ??? Main Run Loop ????????????????????????????????????????????????????????????
void SpaceShooter::run() {
    sf::Clock clock;

    while (window->isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;
        glowTimer += dt;

        sf::Event event;
        while (window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) window->close();

            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape &&
                state != SSState::PLAYING)
                return;

            switch (state) {
            case SSState::WELCOME:       handleWelcome(event);      break;
            case SSState::ENTER_P1:      handleEnterP1(event);      break;
            case SSState::ENTER_P2:      handleEnterP2(event);      break;
            case SSState::MODE_SELECT:   handleModeSelect(event);   break;
            case SSState::GAME_OVER:     handleGameOver(event);     break;
            case SSState::FINAL_RESULTS: handleFinalResults(event); break;
            default: break;
            }
        }

        // Countdown
        if (state == SSState::COUNTDOWN) {
            countdownTimer += dt;
            if (countdownTimer >= 1.f) {
                countdownTimer = 0.f; countdownNum--;
                if (countdownNum < 0) { state = SSState::PLAYING; gameOverTimer = 0.f; }
            }
        }

        // Physics
        if (state == SSState::PLAYING) {
            updateStars(dt);
            updateShips(dt);
            updateBullets(dt);
            updateEnemies(dt);
            updateParticles(dt);
            checkCollisions();
            updateWave(dt);
        }
        else {
            updateStars(dt);
            updateParticles(dt);
        }

        drawBackground();

        switch (state) {
        case SSState::WELCOME:       drawWelcome();       break;
        case SSState::ENTER_P1:      drawEnterName(1);    break;
        case SSState::ENTER_P2:      drawEnterName(2);    break;
        case SSState::MODE_SELECT:   drawModeSelect();    break;
        case SSState::COUNTDOWN:     drawCountdown();     break;
        case SSState::PLAYING:       drawPlaying();       break;
        case SSState::GAME_OVER:     drawGameOver();      break;
        case SSState::FINAL_RESULTS: drawFinalResults();  break;
        }

        window->display();
    }
}

// ??? getResult ????????????????????????????????????????????????????????????????
void SpaceShooter::getResult(char* p1Out, int& p1ScoreOut,
    char* p2Out, int& p2ScoreOut,
    int& waveReachedOut) const
{
    sCpy(p1Out, ships[0].name);
    sCpy(p2Out, ships[1].name);
    p1ScoreOut = ships[0].score;
    p2ScoreOut = ships[1].score;
    waveReachedOut = wave;
}