#include "Hub.h"
#include <cmath>

const float Hub::MORPH_DURATION = 0.9f;

// ??? GameCard constructor ??????????????????????????????????????????????????????
GameCard::GameCard()
    : baseX(0), baseY(0), cardW(0), cardH(0),
    hoverScale(1.f), sweepOffset(-300.f), isHovered(false) {
}

// ??? Ease ?????????????????????????????????????????????????????????????????????
float Hub::easeOutCubic(float t) { float f = 1.f - t; return 1.f - f * f * f; }

// ??? Constructor ??????????????????????????????????????????????????????????????
Hub::Hub(sf::RenderWindow* win)
    : window(win), glowTimer(0.f), morphTimer(0.f),
    cardRevealTimer(0.f), cardsVisible(false),
    state(HubState::INTRO)
{
    neonCyan = sf::Color(0, 240, 255);
    neonPink = sf::Color(255, 30, 180);
    neonYellow = sf::Color(255, 220, 0);
    neonGreen = sf::Color(0, 255, 120);
    neonOrange = sf::Color(255, 140, 0);
    darkBg = sf::Color(4, 4, 16);

    titleStartY = 390.f;   // CY(430) - 40 = 390
    titleEndY = 62.f;
    titleStartSize = 72;
    titleEndSize = 46;

    font.loadFromFile("Orbitron-VariableFont_wght.ttf");
    initTitle();
    initCards();
    initTextures();
    initParticles();
    initCornerBrackets();

    if (!bgMusic.openFromFile("theme.ogg"))
        bgMusic.openFromFile("x64/Debug/theme.ogg");

    bgMusic.setLoop(true);
    bgMusic.setVolume(55.f);
    bgMusic.play();

    if (bufClick.loadFromFile("click.wav"))
        sndClick.setBuffer(bufClick);
    sndClick.setVolume(80.f);
}

// ??????????????????????????????????????????????????????????????????????????????
//  INIT
// ??????????????????????????????????????????????????????????????????????????????
void Hub::initTitle() {
    titleText.setFont(font);
    titleText.setString("FAIQ'S GAMING HUB");
    titleText.setCharacterSize((unsigned)titleStartSize);
    titleText.setFillColor(neonCyan);
    titleText.setStyle(sf::Text::Bold);

    subtitleText.setFont(font);
    subtitleText.setString("- SELECT YOUR GAME -");
    subtitleText.setCharacterSize(13);
    subtitleText.setFillColor(sf::Color(0, 0, 0, 0));
    subtitleText.setLetterSpacing(5.f);
    sf::FloatRect sb = subtitleText.getLocalBounds();
    subtitleText.setOrigin(sb.width / 2.f, sb.height / 2.f);
    subtitleText.setPosition(600.f, 118.f);

    titleUnderline.setSize({ 500.f, 2.f });
    titleUnderline.setFillColor(sf::Color(0, 240, 255, 0));
    titleUnderline.setOrigin(250.f, 1.f);
    titleUnderline.setPosition(600.f, 142.f);
}

void Hub::initCards() {
    // Layout: 2x2 grid for games (top), leaderboard wide card (bottom)
    // Grid: two columns, two rows
    float cW = 530.f, cH = 270.f;
    float padX = 45.f, padY = 160.f;
    float gapX = 55.f, gapY = 30.f;

    // Game cards 0-3
    float posX[4] = { padX, padX + cW + gapX, padX, padX + cW + gapX };
    float posY[4] = { padY, padY, padY + cH + gapY, padY + cH + gapY };

    sf::Color cols[4] = { neonCyan, neonPink, neonYellow, neonGreen };

    for (int i = 0; i < 4; i++) {
        cards[i].baseX = posX[i]; cards[i].baseY = posY[i];
        cards[i].cardW = cW;      cards[i].cardH = cH;
        cards[i].accentColor = cols[i];
        cards[i].sweepOffset = -300.f;
    }

    // Leaderboard card (index 4) — full width minus margins, below grid
    float lbY = padY + 2.f * (cH + gapY) + 10.f;
    cards[4].baseX = padX;
    cards[4].baseY = lbY;
    cards[4].cardW = 1200.f - 2.f * padX;
    cards[4].cardH = 80.f;
    cards[4].accentColor = neonYellow;
    cards[4].sweepOffset = -300.f;
}

void Hub::initTextures() {
    const char* files[4] = {
        "tictactoe.png","pong.png","snakeladder.png","spaceshooter.png"
    };
    for (int i = 0; i < 4; i++) {
        textureLoaded[i] = gameTextures[i].loadFromFile(files[i]);
        if (textureLoaded[i]) {
            gameTextures[i].setSmooth(true);
            gameSprites[i].setTexture(gameTextures[i]);
        }
    }
}

void Hub::initParticles() {
    sf::Color pCols[5] = { neonCyan, neonPink, neonYellow, neonGreen, neonOrange };
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        float seed = (float)i * 137.508f;
        float sx = std::sin(seed) * 43758.5f; sx -= (int)sx; if (sx < 0)sx += 1.f;
        float sy = std::sin(seed + 1.f) * 43758.5f; sy -= (int)sy; if (sy < 0)sy += 1.f;
        float sv = std::sin(seed + 2.f) * 43758.5f; sv -= (int)sv; if (sv < 0)sv += 1.f;
        float sa = std::sin(seed + 3.f) * 43758.5f; sa -= (int)sa; if (sa < 0)sa += 1.f;
        float ss = std::sin(seed + 4.f) * 43758.5f; ss -= (int)ss; if (ss < 0)ss += 1.f;

        HubParticle& p = particles[i];
        p.x = sx * 1200.f;
        p.y = sy * 900.f;
        p.vx = (sv - 0.5f) * 18.f;
        p.vy = (sa - 0.5f) * 18.f - 5.f;
        p.size = 1.f + ss * 2.5f;
        p.maxAlpha = 30.f + sa * 60.f;
        p.alpha = p.maxAlpha;
        p.color = pCols[i % 5];
    }
}

void Hub::initCornerBrackets() {
    float bLen = 24.f, bThick = 2.f, margin = 20.f;
    float W = 1200.f, H = 900.f;
    float cx[4] = { margin, W - margin - bLen, margin,        W - margin - bLen };
    float cy[4] = { margin, margin,        H - margin - bLen, H - margin - bLen };
    for (int i = 0; i < 4; i++) {
        bracketsH[i].setSize({ bLen, bThick });
        bracketsH[i].setPosition(cx[i], cy[i]);
        bracketsH[i].setFillColor(sf::Color(0, 240, 255, 130));
        bracketsV[i].setSize({ bThick, bLen });
        bracketsV[i].setPosition(cx[i], cy[i]);
        bracketsV[i].setFillColor(sf::Color(0, 240, 255, 130));
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  UPDATE
// ??????????????????????????????????????????????????????????????????????????????
void Hub::updateParticles(float dt) {
    sf::Color pCols[5] = { neonCyan, neonPink, neonYellow, neonGreen, neonOrange };
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
        // Wrap
        if (particles[i].x < -10.f)  particles[i].x = 1210.f;
        if (particles[i].x > 1210.f) particles[i].x = -10.f;
        if (particles[i].y < -10.f)  particles[i].y = 910.f;
        if (particles[i].y > 910.f)  particles[i].y = -10.f;
        // Gentle pulse
        particles[i].alpha = particles[i].maxAlpha *
            (0.5f + 0.5f * std::sin(glowTimer * 1.2f + i * 0.4f));
    }
}

void Hub::updateHover(sf::Vector2i mp, float dt) {
    for (int i = 0; i < TOTAL_CARDS; i++) {
        bool h = sf::FloatRect(cards[i].baseX, cards[i].baseY,
            cards[i].cardW, cards[i].cardH)
            .contains((float)mp.x, (float)mp.y);
        cards[i].isHovered = h;

        // Smooth scale
        float target = h ? 0.94f : 1.0f;
        cards[i].hoverScale += (target - cards[i].hoverScale) * 9.f * dt;

        // Sweep: when hovered, advance sweep; when not, reset
        if (h) {
            cards[i].sweepOffset += 900.f * dt;
            if (cards[i].sweepOffset > cards[i].cardW + 250.f)
                cards[i].sweepOffset = -250.f;  // loop
        }
        else {
            cards[i].sweepOffset = -250.f;
        }
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  DRAW HELPERS
// ??????????????????????????????????????????????????????????????????????????????
void Hub::drawThickLine(float x1, float y1, float x2, float y2, float t, sf::Color c) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.1f)return;
    float angle = std::atan2(dy, dx) * 180.f / 3.14159f;
    sf::RectangleShape line({ len,t });
    line.setOrigin(0.f, t / 2.f);
    line.setPosition(x1, y1);
    line.setRotation(angle);
    line.setFillColor(c);
    window->draw(line);
}

// HUD-style targeting brackets on card corners
void Hub::drawCardBrackets(float x, float y, float w, float h, sf::Color c, float alpha) {
    float bL = 18.f, bT = 2.5f;
    sf::Uint8 a = (sf::Uint8)(alpha);
    sf::Color bc(c.r, c.g, c.b, a);

    // Top-left
    sf::RectangleShape r1({ bL,bT }); r1.setFillColor(bc);
    r1.setPosition(x, y); window->draw(r1);
    sf::RectangleShape r2({ bT,bL }); r2.setFillColor(bc);
    r2.setPosition(x, y); window->draw(r2);

    // Top-right
    sf::RectangleShape r3({ bL,bT }); r3.setFillColor(bc);
    r3.setPosition(x + w - bL, y); window->draw(r3);
    sf::RectangleShape r4({ bT,bL }); r4.setFillColor(bc);
    r4.setPosition(x + w - bT, y); window->draw(r4);

    // Bottom-left
    sf::RectangleShape r5({ bL,bT }); r5.setFillColor(bc);
    r5.setPosition(x, y + h - bT); window->draw(r5);
    sf::RectangleShape r6({ bT,bL }); r6.setFillColor(bc);
    r6.setPosition(x, y + h - bL); window->draw(r6);

    // Bottom-right
    sf::RectangleShape r7({ bL,bT }); r7.setFillColor(bc);
    r7.setPosition(x + w - bL, y + h - bT); window->draw(r7);
    sf::RectangleShape r8({ bT,bL }); r8.setFillColor(bc);
    r8.setPosition(x + w - bT, y + h - bL); window->draw(r8);
}

// ??????????????????????????????????????????????????????????????????????????????
//  BACKGROUND
// ??????????????????????????????????????????????????????????????????????????????
void Hub::drawBackground() {
    window->clear(darkBg);

    // Radial ambient glow in center
    for (int r = 400; r > 0; r -= 40) {
        sf::CircleShape g((float)r);
        g.setOrigin((float)r, (float)r);
        g.setPosition(600.f, 480.f);
        float a = (1.f - (float)r / 400.f) * 8.f;
        g.setFillColor(sf::Color(0, 30, 80, (sf::Uint8)a));
        window->draw(g);
    }

    // Fine dot grid
    sf::CircleShape dot(0.9f);
    dot.setFillColor(sf::Color(255, 255, 255, 10));
    for (int x = 30; x < 1200; x += 40)
        for (int y = 30; y < 900; y += 40) {
            dot.setPosition((float)x, (float)y);
            window->draw(dot);
        }

    // Subtle horizontal scan lines
    for (int i = 0; i < 150; i++) {
        sf::RectangleShape sl({ 1200.f,1.f });
        sl.setPosition(0.f, (float)(i * 6));
        sl.setFillColor(sf::Color(0, 0, 0, 16));
        window->draw(sl);
    }
}

void Hub::drawParticles() {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        sf::CircleShape p(particles[i].size);
        p.setOrigin(particles[i].size, particles[i].size);
        p.setPosition(particles[i].x, particles[i].y);
        sf::Color c = particles[i].color;
        p.setFillColor(sf::Color(c.r, c.g, c.b, (sf::Uint8)particles[i].alpha));
        window->draw(p);
    }
}

void Hub::drawCornerBrackets() {
    float pulse = (std::sin(glowTimer * 1.5f) + 1.f) / 2.f;
    for (int i = 0; i < 4; i++) {
        sf::Color bc = bracketsH[i].getFillColor();
        bc.a = (sf::Uint8)(100 + 80 * pulse);
        bracketsH[i].setFillColor(bc);
        bracketsV[i].setFillColor(bc);
        window->draw(bracketsH[i]);
        window->draw(bracketsV[i]);
    }
}

// ??? Rounded rectangle fill ???????????????????????????????????????????????????
void Hub::drawRoundedRect(float x, float y, float w, float h, float r,
    sf::Color fill, sf::RenderStates rs)
{
    // Centre cross
    sf::RectangleShape cH({ w - 2.f * r, h });
    cH.setPosition(x + r, y); cH.setFillColor(fill);
    window->draw(cH, rs);
    sf::RectangleShape cV({ w, h - 2.f * r });
    cV.setPosition(x, y + r); cV.setFillColor(fill);
    window->draw(cV, rs);
    // Four corner circles
    float cx[4] = { x + r,     x + w - r,  x + r,    x + w - r };
    float cy[4] = { y + r,     y + r,    y + h - r,  y + h - r };
    for (int k = 0; k < 4; k++) {
        sf::CircleShape c(r, 16);
        c.setOrigin(r, r); c.setPosition(cx[k], cy[k]);
        c.setFillColor(fill);
        window->draw(c, rs);
    }
}

// ??? Rounded rectangle border (outline only) ??????????????????????????????????
void Hub::drawRoundedBorder(float x, float y, float w, float h, float r,
    float thick, sf::Color col, sf::RenderStates rs)
{
    // Top / bottom / left / right edges — extend fully to corners, no arcs
    sf::RectangleShape top({ w, thick });
    top.setPosition(x, y); top.setFillColor(col); window->draw(top, rs);
    sf::RectangleShape bot({ w, thick });
    bot.setPosition(x, y + h - thick); bot.setFillColor(col); window->draw(bot, rs);
    sf::RectangleShape lft({ thick, h });
    lft.setPosition(x, y); lft.setFillColor(col); window->draw(lft, rs);
    sf::RectangleShape rgt({ thick, h });
    rgt.setPosition(x + w - thick, y); rgt.setFillColor(col); window->draw(rgt, rs);
}

// ??????????????????????????????????????????????????????????????????????????????
//  ICON (thumbnail or fallback drawing)
// ??????????????????????????????????????????????????????????????????????????????
void Hub::drawIconForGame(int idx, float px, float py, float pw, float ph) {
    if (textureLoaded[idx]) {
        sf::Texture& tex = gameTextures[idx];
        sf::Sprite& spr = gameSprites[idx];
        float tw = (float)tex.getSize().x, th = (float)tex.getSize().y;
        float sx = pw / tw, sy = ph / th;
        float sc = (sx < sy) ? sx : sy;
        spr.setScale(sc, sc);
        spr.setPosition(px + (pw - tw * sc) / 2.f, py + (ph - th * sc) / 2.f);
        spr.setColor(cards[idx].isHovered ?
            sf::Color(255, 255, 255, 255) : sf::Color(210, 210, 210, 220));
        window->draw(spr);
        return;
    }
    // Fallback minimal icon
    sf::Color c = cards[idx].accentColor;
    float cx = px + pw / 2.f, cy2 = py + ph / 2.f;
    sf::CircleShape ico(28.f, 6);
    ico.setOrigin(28.f, 28.f); ico.setPosition(cx, cy2);
    ico.setFillColor(sf::Color(c.r / 8, c.g / 8, c.b / 8, 180));
    ico.setOutlineThickness(2.5f); ico.setOutlineColor(c);
    window->draw(ico);
    sf::Text t; t.setFont(font); t.setString("?");
    t.setCharacterSize(28); t.setFillColor(c);
    sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin(tb.width / 2.f, tb.height / 2.f);
    t.setPosition(cx, cy2 - 4.f); window->draw(t);
}

// ??????????????????????????????????????????????????????????????????????????????
//  DRAW GAME CARD  (index 0-3)
// ??????????????????????????????????????????????????????????????????????????????
void Hub::drawGameCard(int i, float alpha) {
    if (alpha <= 0.f) return;

    const char* names[4] = { "TIC TAC TOE","PONG","SNAKE & LADDER","SPACE SHOOTER" };
    const char* descs[4] = { "Classic 3x3 strategy duel","Retro paddle battle",
                           "Roll the dice, race to win","Defend the galaxy" };
    const char* nums[4] = { "01","02","03","04" };

    float s = cards[i].hoverScale;
    float bx = cards[i].baseX, by = cards[i].baseY;
    float cW = cards[i].cardW, cH = cards[i].cardH;
    float pvx = bx + cW / 2.f, pvy = by + cH / 2.f;
    sf::Color ac = cards[i].accentColor;
    float pulse = (std::sin(glowTimer * 2.5f + i * 0.7f) + 1.f) / 2.f;
    bool hov = cards[i].isHovered;

    sf::Transform tf;
    tf.translate(pvx, pvy); tf.scale(s, s); tf.translate(-pvx, -pvy);
    sf::RenderStates rs(tf);

    // ?? Shadow beneath card ??????????????????????????????????????????????????
    drawRoundedRect(bx + 5.f, by + 8.f, cW - 10.f, cH - 10.f, 8.f,
        sf::Color(0, 0, 0, (sf::Uint8)(60 * alpha / 255.f)), rs);

    // ?? Card body (rounded) ???????????????????????????????????????????????????
    sf::Color bodyFill = hov ?
        sf::Color(ac.r / 10, ac.g / 10, ac.b / 10, (sf::Uint8)(240 * alpha / 255.f)) :
        sf::Color(8, 8, 26, (sf::Uint8)(230 * alpha / 255.f));
    drawRoundedRect(bx, by, cW, cH, 8.f, bodyFill, rs);

    // ?? Rounded border ????????????????????????????????????????????????????????
    sf::Uint8 borderA = hov ? 255 : (sf::Uint8)(120 * alpha / 255.f);
    float bthick = hov ? 2.f : 1.5f;
    drawRoundedBorder(bx, by, cW, cH, 8.f, bthick,
        sf::Color(ac.r, ac.g, ac.b, borderA), rs);

    // ?? Diagonal accent stripe (top-left) ?????????????????????????????????????
    for (int d = 0; d < 3; d++) {
        float off = (float)d * 10.f;
        sf::Uint8 sa = (sf::Uint8)((hov ? 80.f : 40.f) * (alpha / 255.f));
        drawThickLine(bx + off, by, bx, by + off, 1.5f,
            sf::Color(ac.r, ac.g, ac.b, sa));
    }

    // ?? Left colored sidebar (inset from corners so it respects rounding) ????
    float sbW = 5.f, sbR = 14.f;
    sf::RectangleShape sidebar({ sbW, cH - 2.f * sbR });
    sidebar.setPosition(bx, by + sbR);
    sidebar.setFillColor(sf::Color(ac.r, ac.g, ac.b, (sf::Uint8)(200 * alpha / 255.f)));
    window->draw(sidebar, rs);

    // ?? Glow sweep (diagonal light beam on hover) ????????????????????????????
    if (hov && cards[i].sweepOffset > -200.f) {
        float sw = cards[i].sweepOffset;
        // Draw a narrow diagonal bright stripe across card
        for (int sl = 0; sl < 5; sl++) {
            float off = (float)sl * 8.f - 20.f;
            float x1 = bx + sw + off, y1 = by;
            float x2 = bx + sw + off - cH * 0.4f, y2 = by + cH;
            sf::Uint8 sweepA = (sf::Uint8)(
                (1.f - fabs2((float)(sl - 2) / 3.f)) * 28.f * (alpha / 255.f));
            drawThickLine(x1, y1, x2, y2, 18.f,
                sf::Color(255, 255, 255, sweepA));
        }
    }

    // ?? Thumbnail image panel ?????????????????????????????????????????????????
    float imgX = bx + 14.f, imgY = by + 14.f;
    float imgW = 220.f, imgH = cH - 28.f;

    // Image background
    sf::RectangleShape imgBg({ imgW,imgH });
    imgBg.setPosition(imgX, imgY);
    imgBg.setFillColor(sf::Color(ac.r / 12, ac.g / 12, ac.b / 12, 200));
    window->draw(imgBg, rs);

    drawIconForGame(i, imgX, imgY, imgW, imgH);

    // Image inner border
    sf::RectangleShape imgB({ imgW,imgH });
    imgB.setPosition(imgX, imgY);
    imgB.setFillColor(sf::Color::Transparent);
    imgB.setOutlineThickness(1.f);
    imgB.setOutlineColor(sf::Color(ac.r, ac.g, ac.b, (sf::Uint8)(60 + 40 * pulse)));
    window->draw(imgB, rs);

    // ?? Number badge (top-right corner of image) ??????????????????????????????
    float badgeX = imgX + imgW - 38.f, badgeY = imgY + 8.f;
    sf::RectangleShape badge({ 32.f,20.f });
    badge.setPosition(badgeX, badgeY);
    badge.setFillColor(sf::Color(ac.r / 5, ac.g / 5, ac.b / 5, 220));
    window->draw(badge, rs);
    sf::Text numT; numT.setFont(font); numT.setString(nums[i]);
    numT.setCharacterSize(11); numT.setFillColor(ac);
    numT.setStyle(sf::Text::Bold);
    sf::FloatRect nb = numT.getLocalBounds();
    numT.setOrigin(nb.width / 2.f, nb.height / 2.f);
    numT.setPosition(badgeX + 16.f, badgeY + 10.f);
    window->draw(numT, rs);

    // ?? Game name ?????????????????????????????????????????????????????????????
    float textX = bx + imgW + 28.f;
    sf::Text name; name.setFont(font); name.setString(names[i]);
    sf::Uint8 nameA = (sf::Uint8)(255 * alpha / 255.f);
    name.setCharacterSize(hov ? 24 : 22);
    name.setFillColor(hov ? sf::Color::White : sf::Color(ac.r, ac.g, ac.b, nameA));
    name.setStyle(sf::Text::Bold);
    name.setPosition(textX, by + cH / 2.f - 42.f);
    window->draw(name, rs);

    // ?? Thin accent line under name ???????????????????????????????????????????
    float lineW = hov ? 160.f : 80.f + 40.f * pulse;
    sf::RectangleShape nameLine({ lineW, 2.f });
    nameLine.setPosition(textX, by + cH / 2.f - 10.f);
    nameLine.setFillColor(sf::Color(ac.r, ac.g, ac.b, (sf::Uint8)(hov ? 200 : 80)));
    window->draw(nameLine, rs);

    // ?? Description ???????????????????????????????????????????????????????????
    sf::Text desc; desc.setFont(font); desc.setString(descs[i]);
    desc.setCharacterSize(13);
    desc.setFillColor(sf::Color(180, 180, 210, (sf::Uint8)(160 * alpha / 255.f)));
    desc.setPosition(textX, by + cH / 2.f + 8.f);
    window->draw(desc, rs);

    // ?? "CLICK TO PLAY" hint (only on hover) ??????????????????????????????????
    if (hov) {
        sf::Text play; play.setFont(font);
        play.setString("CLICK TO PLAY >");
        play.setCharacterSize(11);
        play.setFillColor(sf::Color(ac.r, ac.g, ac.b, (sf::Uint8)(140 + 100 * pulse)));
        play.setPosition(textX, by + cH - 32.f);
        window->draw(play, rs);
    }

    // ?? Bottom glow line (on hover) ???????????????????????????????????????????
    if (hov) {
        sf::RectangleShape glow({ cW,4.f });
        glow.setPosition(bx, by + cH - 3.f);
        glow.setFillColor(sf::Color(ac.r, ac.g, ac.b, (sf::Uint8)(80 + 60 * pulse)));
        window->draw(glow, rs);
    }
}

// ??????????????????????????????????????????????????????????????????????????????
//  LEADERBOARD CARD  (index 4, wide bottom card)
// ??????????????????????????????????????????????????????????????????????????????
void Hub::drawLBCard(float alpha) {
    if (alpha <= 0.f) return;

    float bx = cards[4].baseX, by = cards[4].baseY;
    float cW = cards[4].cardW, cH = cards[4].cardH;
    float s = cards[4].hoverScale;
    bool hov = cards[4].isHovered;
    float pulse = (std::sin(glowTimer * 3.f) + 1.f) / 2.f;

    sf::Transform tf;
    tf.translate(bx + cW / 2.f, by + cH / 2.f);
    tf.scale(s, s);
    tf.translate(-(bx + cW / 2.f), -(by + cH / 2.f));
    sf::RenderStates rs(tf);

    // Body (rounded)
    sf::Color bodyFill = hov ?
        sf::Color(50, 42, 0, (sf::Uint8)(240 * alpha / 255.f)) :
        sf::Color(14, 12, 4, (sf::Uint8)(210 * alpha / 255.f));
    drawRoundedRect(bx, by, cW, cH, 14.f, bodyFill, rs);

    // Border (rounded)
    sf::Uint8 borderA = hov ? (sf::Uint8)(255) : (sf::Uint8)(100 * alpha / 255.f);
    float bthick = hov ? 2.5f : 1.5f;
    drawRoundedBorder(bx, by, cW, cH, 14.f, bthick,
        sf::Color(255, 220, 0, borderA), rs);

    // HUD corner brackets
    drawCardBrackets(bx, by, cW, cH, neonYellow, hov ? 255.f : 140.f * (alpha / 255.f));

    // Sweep glow
    if (hov && cards[4].sweepOffset > -200.f) {
        float sw = cards[4].sweepOffset;
        for (int sl = 0; sl < 5; sl++) {
            float off = (float)sl * 8.f - 20.f;
            sf::Uint8 sweepA = (sf::Uint8)(
                (1.f - fabs2((float)(sl - 2) / 3.f)) * 22.f * (alpha / 255.f));
            drawThickLine(bx + sw + off, by, bx + sw + off - cH * 0.4f, by + cH,
                18.f, sf::Color(255, 220, 0, sweepA));
        }
    }

    // Trophy icon (left side)
    float iconCX = bx + 50.f, iconCY = by + cH / 2.f;
    // Cup body
    sf::RectangleShape cup({ 28.f,22.f });
    cup.setFillColor(sf::Color(50, 42, 0, 220));
    cup.setOutlineThickness(2.f);
    cup.setOutlineColor(sf::Color(255, 220, 0, (sf::Uint8)(180 + 75 * pulse)));
    cup.setPosition(iconCX - 14.f, iconCY - 14.f);
    window->draw(cup, rs);
    // Handles
    sf::RectangleShape lh({ 6.f,12.f }); lh.setFillColor(sf::Color(255, 220, 0, 140));
    lh.setPosition(iconCX - 20.f, iconCY - 8.f); window->draw(lh, rs);
    sf::RectangleShape rh({ 6.f,12.f }); rh.setFillColor(sf::Color(255, 220, 0, 140));
    rh.setPosition(iconCX + 14.f, iconCY - 8.f); window->draw(rh, rs);
    // Base
    sf::RectangleShape base({ 22.f,5.f }); base.setFillColor(sf::Color(255, 220, 0, 160));
    base.setPosition(iconCX - 11.f, iconCY + 8.f); window->draw(base, rs);
    // Spinning star in cup
    sf::CircleShape star(6.f, 5);
    star.setOrigin(6.f, 6.f); star.setPosition(iconCX, iconCY - 2.f);
    star.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(160 + 80 * pulse)));
    star.setRotation(glowTimer * 60.f);
    window->draw(star, rs);

    // "LEADERBOARD" text center
    float textCX = bx + cW / 2.f;
    sf::Text title; title.setFont(font);
    title.setString("LEADERBOARD");
    title.setCharacterSize(hov ? 30 : 26);
    title.setFillColor(hov ? sf::Color(255, 220, 0, 255) :
        sf::Color(255, 220, 0, (sf::Uint8)(180 * alpha / 255.f)));
    title.setStyle(sf::Text::Bold);
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.width / 2.f, tb.height / 2.f);
    title.setPosition(textCX, by + cH / 2.f - 10.f);
    window->draw(title, rs);

    // Subtitle
    sf::Text sub; sub.setFont(font);
    sub.setString(hov ? "CLICK TO VIEW RANKINGS" : "[ L ]  VIEW RANKINGS");
    sub.setCharacterSize(12);
    sub.setFillColor(sf::Color(200, 170, 0, (sf::Uint8)((hov ? 180 : 100) * alpha / 255.f)));
    sub.setLetterSpacing(2.f);
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setOrigin(sb.width / 2.f, sb.height / 2.f);
    sub.setPosition(textCX, by + cH / 2.f + 16.f);
    window->draw(sub, rs);

    // Pulsing side lines
    sf::Uint8 lineA = (sf::Uint8)(60 + 50 * pulse);
    sf::RectangleShape ll({ 3.f,cH * 0.6f });
    ll.setPosition(bx + 90.f, by + cH * 0.2f);
    ll.setFillColor(sf::Color(255, 220, 0, lineA));
    window->draw(ll, rs);
    sf::RectangleShape rl({ 3.f,cH * 0.6f });
    rl.setPosition(bx + cW - 93.f, by + cH * 0.2f);
    rl.setFillColor(sf::Color(255, 220, 0, lineA));
    window->draw(rl, rs);
}

// ??????????????????????????????????????????????????????????????????????????????
//  INTRO SCREEN  — clean cinematic
// ??????????????????????????????????????????????????????????????????????????????
void Hub::drawIntro() {
    float pulse = (std::sin(glowTimer * 1.8f) + 1.f) / 2.f;
    float pulse2 = (std::sin(glowTimer * 3.2f) + 1.f) / 2.f;
    float pulse3 = (std::sin(glowTimer * 0.7f) + 1.f) / 2.f;

    const float CX = 600.f, CY = 450.f;

    // ?? 1. Deep radial glow layers ????????????????????????????????????????????
    for (int ring = 5; ring >= 1; ring--) {
        float rad = 110.f * ring;
        sf::CircleShape g(rad, 40);
        g.setOrigin(rad, rad); g.setPosition(CX, CY);
        float a = (1.f - (float)ring / 6.f) * (10.f + 6.f * pulse3);
        g.setFillColor(sf::Color(0, 20, 60, (sf::Uint8)a));
        window->draw(g);
    }

    // ?? 2. Rotating hex rings ?????????????????????????????????????????????????
    float hexSpeeds[3] = { 10.f, -7.f, 16.f };
    float hexRadii[3] = { 320.f, 230.f, 150.f };
    sf::Color hexCols[3] = {
        sf::Color(0,  240, 255, 20),
        sf::Color(255, 30, 180, 14),
        sf::Color(0,  240, 255, 26)
    };
    for (int h = 0; h < 3; h++) {
        float angle = glowTimer * hexSpeeds[h];
        float rad = hexRadii[h];
        sf::ConvexShape hex; hex.setPointCount(6);
        for (int p = 0; p < 6; p++) {
            float a = (angle + p * 60.f) * 3.14159f / 180.f;
            hex.setPoint(p, { CX + rad * std::cos(a), CY + rad * std::sin(a) });
        }
        hex.setFillColor(sf::Color::Transparent);
        hex.setOutlineThickness(1.f);
        hex.setOutlineColor(hexCols[h]);
        window->draw(hex);
    }

    // ?? 3. Horizontal scan beam ???????????????????????????????????????????????
    float beamY = std::fmod(glowTimer * 160.f, 900.f);
    for (int b = 0; b < 6; b++) {
        float by2 = beamY - b * 14.f;
        if (by2 < 0.f || by2 > 900.f) continue;
        sf::RectangleShape beam({ 1200.f, 2.f });
        beam.setPosition(0.f, by2);
        beam.setFillColor(sf::Color(0, 240, 255, (sf::Uint8)((1.f - (float)b / 6.f) * 10.f)));
        window->draw(beam);
    }

    // ?? 4. Title glow shadow layers ???????????????????????????????????????????
    titleText.setCharacterSize((unsigned)titleStartSize);
    sf::FloatRect tb = titleText.getLocalBounds();
    titleText.setOrigin(tb.width / 2.f, tb.height / 2.f);
    titleText.setPosition(CX, CY - 80.f);
    for (int gsh = 4; gsh >= 1; gsh--) {
        titleText.setFillColor(sf::Color(0, 180, 220, (sf::Uint8)(12 * gsh)));
        window->draw(titleText);
    }

    // ?? 5. Main title ?????????????????????????????????????????????????????????
    sf::Uint8 r2 = (sf::Uint8)(pulse2 * 80);
    titleText.setFillColor(sf::Color(r2, 240, 255));
    window->draw(titleText);

    // ?? 6. Double underline ???????????????????????????????????????????????????
    float uw = 560.f + 80.f * pulse;
    sf::RectangleShape ul({ uw, 3.f });
    ul.setFillColor(sf::Color(0, 240, 255, (sf::Uint8)(120 + 135 * pulse)));
    ul.setOrigin(uw / 2.f, 1.5f); ul.setPosition(CX, CY + 8.f);
    window->draw(ul);
    float uw2 = uw - 50.f;
    sf::RectangleShape ul2({ uw2, 1.f });
    ul2.setFillColor(sf::Color(0, 240, 255, (sf::Uint8)(50 + 50 * pulse)));
    ul2.setOrigin(uw2 / 2.f, 0.5f); ul2.setPosition(CX, CY + 14.f);
    window->draw(ul2);

    // ?? 7. Tagline ????????????????????????????????????????????????????????????
    sf::Text tag; tag.setFont(font);
    tag.setString("YOUR ULTIMATE GAMING DESTINATION");
    tag.setCharacterSize(12);
    tag.setFillColor(sf::Color(130, 130, 195, (sf::Uint8)(90 + 70 * pulse)));
    tag.setLetterSpacing(5.f);
    sf::FloatRect tgb = tag.getLocalBounds();
    tag.setOrigin(tgb.width / 2.f, tgb.height / 2.f);
    tag.setPosition(CX, CY + 22.f);
    window->draw(tag);

    // ?? 8. Styled ENTER button ????????????????????????????????????????????????
    float btnW = 340.f, btnH = 54.f;
    float btnX = CX - btnW / 2.f, btnY = CY + 95.f;

    // Simple clean glow
    for (int g2 = 3; g2 >= 1; g2--) {
        sf::RectangleShape glow({ btnW + g2 * 8.f, btnH + g2 * 8.f });
        glow.setOrigin((btnW + g2 * 8.f) / 2.f, (btnH + g2 * 8.f) / 2.f);
        glow.setPosition(CX, btnY + btnH / 2.f);
        glow.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(5 * g2 * pulse)));
        window->draw(glow);
    }
    // Body
    drawRoundedRect(btnX, btnY, btnW, btnH, 10.f,
        sf::Color(38, 32, 0, (sf::Uint8)(210 + 40 * pulse)),
        sf::RenderStates::Default);
    // Border
    drawRoundedBorder(btnX, btnY, btnW, btnH, 10.f, 2.f,
        sf::Color(255, 220, 0, (sf::Uint8)(190 + 65 * pulse)),
        sf::RenderStates::Default);
    // Button text
    sf::Text pr; pr.setFont(font);
    pr.setString("PRESS  ENTER  TO  START");
    pr.setCharacterSize(17);
    pr.setFillColor(sf::Color(255, 220, 0, (sf::Uint8)(200 + 55 * pulse2)));
    pr.setStyle(sf::Text::Bold);
    sf::FloatRect pb = pr.getLocalBounds();
    pr.setOrigin(pb.width / 2.f, pb.height / 2.f);
    pr.setPosition(CX, btnY + btnH / 2.f + 1.f);
    window->draw(pr);

    // ?? 9. Corner HUD brackets ????????????????????????????????????????????????
    drawCornerBrackets();

    // ?? 10. Bottom credit line ????????????????????????????????????????????????
    sf::Text cred; cred.setFont(font);
    cred.setString("CS-1004  |  FAST-NUCES  |  SPRING 2026");
    cred.setCharacterSize(10);
    cred.setFillColor(sf::Color(55, 55, 95, (sf::Uint8)(70 + 40 * pulse)));
    cred.setLetterSpacing(3.f);
    sf::FloatRect cb = cred.getLocalBounds();
    cred.setOrigin(cb.width / 2.f, cb.height / 2.f);
    cred.setPosition(CX, 872.f);
    window->draw(cred);
}

// ??????????????????????????????????????????????????????????????????????????????
//  MENU
// ??????????????????????????????????????????????????????????????????????????????
void Hub::drawMenu(float alpha) {
    float pulse = (std::sin(glowTimer * 2.f) + 1.f) / 2.f;

    drawCornerBrackets();

    // Title
    titleText.setCharacterSize((unsigned)titleEndSize);
    sf::FloatRect tb = titleText.getLocalBounds();
    titleText.setOrigin(tb.width / 2.f, tb.height / 2.f);
    titleText.setPosition(600.f, titleEndY);
    sf::Uint8 r2 = (sf::Uint8)(pulse * 70);
    titleText.setFillColor(sf::Color(r2, 240, 255));
    window->draw(titleText);

    // Underline
    float uw = 400.f + 60.f * pulse;
    titleUnderline.setSize({ uw,2.f });
    titleUnderline.setOrigin(uw / 2.f, 1.f);
    titleUnderline.setFillColor(sf::Color(0, 240, 255, (sf::Uint8)(alpha)));
    window->draw(titleUnderline);

    // Subtitle
    subtitleText.setFillColor(sf::Color(130, 130, 200, (sf::Uint8)(alpha * 0.65f)));
    window->draw(subtitleText);

    // Game cards
    for (int i = 0; i < 4; i++) drawGameCard(i, alpha);

    // Leaderboard card
    drawLBCard(alpha);
}

// ??????????????????????????????????????????????????????????????????????????????
//  RUN LOOP
// ??????????????????????????????????????????????????????????????????????????????
int Hub::run() {
    sf::Clock clock;

    while (window->isOpen()) {
        float dt = clock.restart().asSeconds();
        glowTimer += dt;

        sf::Event ev;
        while (window->pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) {
                bgMusic.stop();
                window->close();
            }

            if (state == HubState::INTRO) {
                if (ev.type == sf::Event::KeyPressed &&
                    ev.key.code == sf::Keyboard::Enter) {
                    state = HubState::MORPHING;
                    morphTimer = 0.f;
                }
            }
            else if (state == HubState::MENU) {
                if (ev.type == sf::Event::MouseButtonPressed &&
                    ev.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mp = sf::Mouse::getPosition(*window);
                    for (int i = 0; i < 4; i++)
                        if (sf::FloatRect(cards[i].baseX, cards[i].baseY,
                            cards[i].cardW, cards[i].cardH)
                            .contains((float)mp.x, (float)mp.y)) {
                            sndClick.play();
                            bgMusic.stop();
                            return i;
                        }
                    if (sf::FloatRect(cards[4].baseX, cards[4].baseY,
                        cards[4].cardW, cards[4].cardH)
                        .contains((float)mp.x, (float)mp.y)) {
                        sndClick.play();
                        return 4;
                    }
                }
                if (ev.type == sf::Event::KeyPressed &&
                    ev.key.code == sf::Keyboard::L) {
                    sndClick.play();
                    return 4;
                }
            }
        }

        // Morph animation
        if (state == HubState::MORPHING) {
            morphTimer += dt;
            if (morphTimer >= MORPH_DURATION) {
                morphTimer = MORPH_DURATION;
                state = HubState::MENU;
                cardsVisible = true;
                cardRevealTimer = 0.f;
            }
        }

        if (state == HubState::MENU) {
            cardRevealTimer += dt;
            updateHover(sf::Mouse::getPosition(*window), dt);
            updateParticles(dt);
        }

        // Draw
        drawBackground();
        if (state == HubState::MENU || state == HubState::MORPHING)
            drawParticles();

        if (state == HubState::INTRO) {
            drawIntro();
        }
        else if (state == HubState::MORPHING) {
            float t = easeOutCubic(morphTimer / MORPH_DURATION);
            float curY = titleStartY + (titleEndY - titleStartY) * t;
            float curSize = titleStartSize + (titleEndSize - titleStartSize) * t;
            titleText.setCharacterSize((unsigned)curSize);
            sf::FloatRect tb = titleText.getLocalBounds();
            titleText.setOrigin(tb.width / 2.f, tb.height / 2.f);
            titleText.setPosition(600.f, curY);
            titleText.setFillColor(neonCyan);
            window->draw(titleText);
            sf::Uint8 ula = (sf::Uint8)(t * 200.f);
            float uw = 420.f;
            titleUnderline.setSize({ uw,2.f });
            titleUnderline.setOrigin(uw / 2.f, 1.f);
            titleUnderline.setFillColor(sf::Color(0, 240, 255, ula));
            window->draw(titleUnderline);
        }
        else if (state == HubState::MENU) {
            float a = cardRevealTimer / 0.7f;
            if (a > 1.f)a = 1.f;
            drawMenu((sf::Uint8)(a * 255.f));
        }

        window->display();
    }
    bgMusic.stop();
    return -1;
}