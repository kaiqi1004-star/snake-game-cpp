#include <GL/glut.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>

// Add sound headers for Windows
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

// Forward function declarations
void update(int value);
void initGame(bool resetScore);
void handleCountdown(int value);

struct Tree {
    float x;
    float y;
};

std::vector<Tree> trees;  // Stores positions of all trees in the game

// --- STRUCTURES ---
struct Segment { float x, y; };  // Represents one segment of the snake
std::vector<Segment> snake;      // Vector containing all snake segments

struct Particle {
    float x, y, vx, vy, life, r, g, b;
};  // Used for visual effects like explosions or collectible effects
std::vector<Particle> particles;

// --- GLOBAL STATES ---
bool showMenu = true;        // Whether main menu is displayed
bool gameOver = false;       // Game over state
bool gameWin = false;        // Game win state
bool inCountdown = false;    // Countdown before game starts
int countdownValue = 3;      // Countdown value (3, 2, 1, GO!)
int score = 0;               // Player's current score
int level = 1;               // Current level (1 or 2)
int foodType = 0;            // Type of food (0=tin can, 1=bottle, 2=debris)
float animationTimer = 0.0f; // Timer for animations (tongue, wings, etc.)
float hazardX = -450, hazardY = 100; // Position of red butterfly hazard
float foodX = 140, foodY = 0;        // Position of current food item
int direction = 1;           // Current snake direction (0=up, 1=right, 2=down, 3=left)
int snakeDelay = 120;        // Delay between snake movements (controls speed)
int nextDirection = 1;       // Next direction based on player input

// --- NEW LEVEL 2 BUTTERFLY VARIABLES (VERTICAL) ---
float bfX = 100, bfY = -350; // Position of blue butterfly in level 2

// --- SOUND VARIABLES ---
bool soundEnabled = true;     // Sound on/off toggle
float moveSoundTimer = 0.0f;  // Timer for movement sounds
bool tongueWasOut = false;    // Track tongue state for sound effects

// --- SOUND FUNCTIONS ---
/*
 * Plays a sound effect based on the given sound name
 * Only works on Windows using the Beep() function
 * Different frequencies and durations for different game events
 */
void playSoundEffect(const char* soundName) {
#ifdef _WIN32
    if (!soundEnabled) return;  // Skip if sound is disabled

    if (strcmp(soundName, "move") == 0) {
        Beep(300, 30);  // Low beep for movement
    }
    else if (strcmp(soundName, "eat") == 0) {
        Beep(600, 100); // Higher beep for eating food
    }
    else if (strcmp(soundName, "tongue") == 0) {
        // Tongue sound removed - function kept for compatibility
        return;
    }
    else if (strcmp(soundName, "death") == 0) {
        Beep(400, 200); // Descending tones for death
        Beep(300, 200);
        Beep(200, 300);
    }
    else if (strcmp(soundName, "win") == 0) {
        Beep(523, 150); // Ascending tones for winning
        Beep(659, 150);
        Beep(784, 150);
        Beep(1047, 300);
    }
    else if (strcmp(soundName, "countdown") == 0) {
        Beep(500, 100); // Simple beep for countdown
    }
    else if (strcmp(soundName, "start") == 0) {
        Beep(600, 100); // Two beeps for game start
        Beep(800, 100);
    }
    else if (strcmp(soundName, "collision") == 0) {
        Beep(200, 150); // Low beep for collision with hazard
    }
    else if (strcmp(soundName, "levelup") == 0) {
        Beep(600, 100); // Ascending tones for level up
        Sleep(50);
        Beep(800, 100);
        Sleep(50);
        Beep(1000, 150);
    }
#endif
}

/*
 * Toggles sound effects on/off
 * Called when player presses 'S' key
 */
void toggleSound() {
    soundEnabled = !soundEnabled;
}

// --- UI HELPER FUNCTIONS ---
/*
 * Draws text at specified screen coordinates
 * x, y: Position to draw text
 * text: String to display
 * font: GLUT bitmap font to use (default is Helvetica 18)
 */
void drawText(float x, float y, std::string text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);  // Set position for bitmap text
    for (char c : text) glutBitmapCharacter(font, c);  // Draw each character
}

/*
 * Draws text centered horizontally on screen
 *  y: Vertical position
 *  text: String to display
 *  font: GLUT bitmap font to use
 */
void drawCenteredText(float y, std::string text, void* font = GLUT_BITMAP_HELVETICA_18) {
    int len = text.length() * 9;  // Approximate pixel width
    drawText(-(len / 2.0f), y, text, font);  // Center by offsetting half width
}

/*
 * Creates particle effects at specified position
 * Used for visual feedback when collecting food
 * x, y: Position to spawn particles
 */
void spawnParticles(float x, float y) {
    for (int i = 0; i < 15; i++) {
        Particle p;
        p.x = x; p.y = y;
        p.vx = (rand() % 10 - 5);  // Random velocity in x (-5 to 5)
        p.vy = (rand() % 10 - 5);  // Random velocity in y (-5 to 5)
        p.life = 1.0f;  // Full life (will fade out)
        p.r = 0.7f; p.g = 0.7f; p.b = 0.7f;  // Gray color
        particles.push_back(p);
    }
}

// --- GAME OBJECTS ---
/*
 * Draws a single tree at specified position
 * Tree consists of a brown trunk and green triangular foliage
 * x, y: Position to draw tree
 */
void drawSingleTree(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    // Draw trunk (brown quad)
    glColor3f(0.25f, 0.15f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(-6, -15); glVertex2f(6, -15);
    glVertex2f(6, 0); glVertex2f(-6, 0);
    glEnd();

    // Draw foliage (green triangle)
    glBegin(GL_TRIANGLES);
    glColor3f(0.0f, 0.3f, 0.0f); glVertex2f(-25.0f, 0.0f);
    glColor3f(0.2f, 0.6f, 0.2f); glVertex2f(0.0f, 45.0f);
    glColor3f(0.0f, 0.3f, 0.0f); glVertex2f(25.0f, 0.0f);
    glEnd();

    glPopMatrix();
}

// --- NEW BUTTERFLY DRAWING FUNCTION ---
/*
 * Draws a butterfly at specified position with animated wings
 * Used for level 2 blue butterflies
 * x, y: Position to draw butterfly
 */
void drawButterfly(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    // Animate wing flapping using sine wave
    float w = sinf(animationTimer * 0.8f) * 12.0f;

    // Draw butterfly as two triangles (wings)
    glColor3f(0.4f, 0.7f, 1.0f); // Light Blue for Level 2
    glBegin(GL_TRIANGLES);
    // Left wing
    glVertex2f(0.0f, 0.0f);
    glVertex2f(-15.0f - w, 15.0f);
    glVertex2f(-15.0f - w, -15.0f);
    // Right wing
    glVertex2f(0.0f, 0.0f);
    glVertex2f(15.0f + w, 15.0f);
    glVertex2f(15.0f + w, -15.0f);
    glEnd();

    glPopMatrix();
}

/*
 * Draws the current food item based on foodType
 * Three types: 0=tin can, 1=bottle, 2=debris
 */
void drawFood() {
    glPushMatrix();
    glTranslatef(foodX, foodY, 0);

    // Draw outline
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);

    if (foodType == 0) { // Tin Can
        glVertex2f(-11.0f, 3.0f); glVertex2f(11.0f, -13.0f);
        glVertex2f(13.0f, 0.0f); glVertex2f(11.0f, 13.0f);
        glVertex2f(-11.0f, 13.0f); glVertex2f(-13.0f, 3.0f);
    }
    else if (foodType == 1) { // Bottle
        glVertex2f(-8.0f, -15.0f); glVertex2f(8, -15);
        glVertex2f(8.0f, 9.0f); glVertex2f(5.0f, 15.0f);
        glVertex2f(-5.0f, 15.0f); glVertex2f(-8.0f, 9.0f);
    }
    else { // Debris (triangle)
        glVertex2f(-14.0f, -14.0f);
        glVertex2f(14.0f, -14.0f);
        glVertex2f(0.0f, 14.0f);
    }
    glEnd();

    // Draw filled interior
    if (foodType == 0) {
        glColor3f(0.7f, 0.7f, 0.7f); // Gray for tin can
        glBegin(GL_POLYGON);
        glVertex2f(-10.0f, -12.0f); glVertex2f(10.0f, -12.0f);
        glVertex2f(12.0f, 0.0f); glVertex2f(10.0f, 12.0f);
        glVertex2f(-10.0f, 12.0f); glVertex2f(-12.0f, 3.0f);
        glEnd();
    }
    else if (foodType == 1) {
        glColor3f(0.3f, 0.8f, 1.0f); // Blue for bottle
        glBegin(GL_POLYGON);
        glVertex2f(-7, -14); glVertex2f(7.0f, -14.0f);
        glVertex2f(7.0f, 8.0f); glVertex2f(4.0f, 14.0f);
        glVertex2f(-4.0f, 14.0f); glVertex2f(-7.0f, 8.0f);
        glEnd();
    }
    else {
        glColor3f(0.8f, 0.5f, 0.2f); // Brown for debris
        glBegin(GL_TRIANGLES);
        glVertex2f(-12.0f, -12.0f);
        glVertex2f(12.0f, -12.0f);
        glVertex2f(0.0f, 12.0f);
        glEnd();
    }
    glPopMatrix();
}

/*
 * Draws the complete snake including head and body segments
 * Head has eyes and animated tongue
 * Body segments have patterned coloring
 */
void drawSnake() {
    float lifeFade = (gameOver || gameWin) ? 0.6f : 1.0f;  // Fade when game ends

    // Animate tongue (every 10 frames, tongue is out for 5 frames)
    bool tongueOut = (static_cast<int>(animationTimer) % 10 < 5);

    // Tongue animation will still happen but without sound
    // Play tongue sound when tongue first appears - REMOVED
    if (tongueOut && !tongueWasOut && !gameOver && !gameWin && !inCountdown) {
        // Sound removed - tongue animation still visible
    }
    tongueWasOut = tongueOut;

    // Draw from tail to head (so head appears on top)
    for (int i = (int)snake.size() - 1; i >= 0; i--) {
        glPushMatrix();
        glTranslatef(snake[i].x, snake[i].y, 0);

        if (i == 0) { // HEAD
            // Rotate head based on direction
            float rotation = (direction == 0) ? 90 : (direction == 2) ? -90 : (direction == 3) ? 180 : 0;
            glRotatef(rotation, 0, 0, 1);

            // Draw tongue if extended
            if (tongueOut) {
                glColor3f(1.0f, 0.2f, 0.2f);
                glBegin(GL_LINES);
                glVertex2f(17, 0); glVertex2f(30, 0);
                glVertex2f(30, 0); glVertex2f(35, 5);
                glVertex2f(30, 0); glVertex2f(35, -5);
                glEnd();
            }

            // Draw head as ellipse (triangle fan)
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(0.0f, 0.8f, 0.0f); glVertex2f(0, 0);  // Center
            glColor3f(0.0f, 0.4f, 0.0f);  // Edge color
            // Draw ellipse points
            for (int j = 0; j <= 360; j += 45)
                glVertex2f(cosf(j * 3.14159f / 180.0f) * 18, sin(j * 3.14 / 180) * 15);
            glEnd();

            // Draw eyes (white part)
            glColor3f(1.0f, 1.0f, 1.0f);
            glPointSize(4.0f);
            glBegin(GL_POINTS);
            glVertex2f(8.0f, 6.0f); glVertex2f(8.0f, -6.0f);
            glEnd();

            // Draw pupils (black part)
            glColor3f(0.0f, 0.0f, 0.0f);
            glPointSize(2.0f);
            glBegin(GL_POINTS);
            glVertex2f(10.0f, 6.0f); glVertex2f(10.0f, -6.0f);
            glEnd();
        }
        else { // BODY SEGMENTS
            float size = 16.0f;
            int slices = 24;

            // Create moving pattern on body
            float pattern = sinf((i + animationTimer * 0.1f) * 0.5f);
            float r, g, b;

            // Alternate between two shades of green
            if (pattern > 0) {
                r = 0.05f * lifeFade;
                g = (0.45f + pattern * 0.15f) * lifeFade;
                b = 0.05f * lifeFade;
            }
            else {
                r = 0.05f * lifeFade;
                g = (0.35f + pattern * 0.1f) * lifeFade;
                b = 0.05f * lifeFade;
            }

            // Draw circular segment with lighting effect
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(r, g, b);
            glVertex2f(0, 0);  // Center

            for (int j = 0; j <= slices; j++) {
                float angle = (float)j * 2.0f * 3.14159f / (float)slices;
                float lx = cosf(angle) * size;
                float ly = sinf(angle) * size;

                // Simple lighting based on y-position
                float light = (ly / size + 1.0f) * 0.5f;
                float segmentG = g * (0.8f + light * 0.4f);

                glColor3f(r * (0.8f + light * 0.4f), segmentG, b * (0.8f + light * 0.4f));
                glVertex2f(lx, ly);
            }
            glEnd();
        }
        glPopMatrix();
    }
}

// --- SYSTEM LOGIC ---
/*
 * Handles the countdown timer before game starts
 * value: Unused timer parameter
 */
void handleCountdown(int value) {
    if (countdownValue > 0) {
        countdownValue--;
        playSoundEffect("countdown");
        glutTimerFunc(1000, handleCountdown, 0);  // Call again in 1 second
    }
    else {
        inCountdown = false;  // Countdown finished
        playSoundEffect("start");
    }
}

/*
 * Initializes tree positions for the current level
 * Creates border trees and level-specific obstacles
 */
void initTrees() {
    trees.clear();

    // Top and bottom border trees
    for (int i = -400; i <= 400; i += 50) {
        trees.push_back({ (float)i, 270.0f });   // Top
        trees.push_back({ (float)i, -285.0f });  // Bottom
    }

    // Left and right border trees
    for (int i = -250; i <= 250; i += 60) {
        trees.push_back({ -390.0f, (float)i });  // Left
        trees.push_back({ 390.0f, (float)i });   // Right
    }

    // Level 2 additional obstacles
    if (level == 2) {
        trees.push_back({ -150.0f, 100.0f });
        trees.push_back({ 150.0f, -100.0f });
    }
}

/*
 * Initializes or resets the game state
 * resetScore: Whether to reset score to 0
 */
void initGame(bool resetScore = true) {
    snake.clear();

    // Snake starting position (3 segments)
    snake.push_back({ 0,0 });      // Head
    snake.push_back({ -20,0 });    // Segment 2
    snake.push_back({ -40,0 });    // Segment 3

    if (resetScore) score = 0;
    gameOver = false;
    gameWin = false;

    // Reset direction and countdown
    direction = 1;
    nextDirection = 1;
    inCountdown = true;
    countdownValue = 3;

    // Snake speed (level 2 is faster)
    snakeDelay = (level == 1) ? 120 : 75;

    // Clear particles
    particles.clear();

    // Reset sound timers
    moveSoundTimer = 0.0f;
    tongueWasOut = false;

    // Initialize trees
    initTrees();

    //  Level 2 butterfly safe spawn
    if (level == 2) {
        // Find safe spawn position (not too close to snake)
        do {
            bfX = (rand() % 700) - 350;
            bfY = (rand() % 700) - 350;
            // Ensure butterfly is at least 80 units from snake head
        } while (sqrt(bfX * bfX + bfY * bfY) < 80.0f);
    }
}

/*
 * Main game update function called by GLUT timer
 * Handles all game logic: movement, collision, scoring
 * value: Unused timer parameter
 */
void update(int value) {
    if (!showMenu && !gameOver && !gameWin && !inCountdown) {
        // Collision radii
        float snakeRadius = 18.0f;
        float foodRadius = 12.0f;
        float treeRadius = 20.0f;
        float hazardRadius = 15.0f;

        // Update timers
        animationTimer += 1.0f;

        // Move red hazard butterfly
        hazardX += (level == 1 ? 4.5f : 6.5f);  // Faster in level 2
        if (hazardX > 450) {
            hazardX = -450;
            hazardY = (rand() % 360) - 180;
        }

        // --- NEW: LEVEL 2 VERTICAL BUTTERFLY MOVEMENT ---
        if (level == 2) {
            bfY += 4.5f;  // Move vertically
            if (bfY > 350) {
                bfY = -350;
                bfX = (rand() % 700) - 350;
            }
        }

        // Update particles (fade and move)
        for (int i = 0; i < (int)particles.size(); i++) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].life -= 0.05f;  // Fade out
            if (particles[i].life <= 0) {
                particles.erase(particles.begin() + i);
                i--;
            }
        }

        // Play movement sound at intervals
        moveSoundTimer += 1.0f;
        if (moveSoundTimer > 5.0f) {
            playSoundEffect("move");
            moveSoundTimer = 0.0f;
        }

        // Move snake body (each segment follows the one in front)
        for (size_t i = snake.size() - 1; i > 0; i--)
            snake[i] = snake[i - 1];

        // Move snake head based on current direction
        direction = nextDirection;
        if (direction == 0) snake[0].y += 20.0f;       // Up
        else if (direction == 1) snake[0].x += 20.0f;  // Right
        else if (direction == 2) snake[0].y -= 20.0f;  // Down
        else if (direction == 3) snake[0].x -= 20.0f;  // Left

        // Check self-collision (head hits body)
        for (size_t i = 1; i < snake.size(); i++) {
            float dx = snake[0].x - snake[i].x;
            float dy = snake[0].y - snake[i].y;
            if (sqrt(dx * dx + dy * dy) < 10.0f) {  // 10 is collision threshold
                gameOver = true;
                playSoundEffect("death");
            }
        }

        //Check boundary collision
        if (abs(snake[0].x) > 370 || abs(snake[0].y) > 250) {
            gameOver = true;
            playSoundEffect("death");
        }

        // Check tree collisions
        for (const auto& t : trees) {
            float tx = snake[0].x - t.x;
            float ty = snake[0].y - t.y;
            float tDist = sqrt(tx * tx + ty * ty);

            //Tree collision = instant death
            if (tDist <= (snakeRadius + treeRadius)) {
                gameOver = true;
                playSoundEffect("death");
            }
        }

        // Check food collision
        float dx = snake[0].x - foodX;
        float dy = snake[0].y - foodY;
        float dist = sqrt(dx * dx + dy * dy);

        // Food must be completely inside snake head
        if (dist <= (snakeRadius - foodRadius)) {
            score += 10;
            spawnParticles(foodX, foodY);  // Visual effect
            playSoundEffect("eat");

            snake.push_back(snake.back());  // Grow snake

            // Generate new food at random position
            foodType = rand() % 3;
            do {
                foodX = (rand() % 16 - 8) * 40.0f;  // Grid-aligned positions
                foodY = (rand() % 10 - 5) * 40.0f;

                // Level 2: avoid spawning food too close to middle trees
                if (level == 2) {
                    float tree1X = -150, tree1Y = 100;
                    float tree2X = 150, tree2Y = -100;

                    float d1 = sqrt((foodX - tree1X) * (foodX - tree1X) +
                        (foodY - tree1Y) * (foodY - tree1Y));
                    float d2 = sqrt((foodX - tree2X) * (foodX - tree2X) +
                        (foodY - tree2Y) * (foodY - tree2Y));

                    if (d1 > 60.0f && d2 > 60.0f) break; // Safe distance
                }
                else break; // Level 1 no restrictions
            } while (true);

            // Check win condition
            if (score >= 100) {
                gameWin = true;
                playSoundEffect("win");
            }
        }

        // Check red butterfly collision
        float hx = snake[0].x - hazardX;
        float hy = snake[0].y - hazardY;
        float hDist = sqrt(hx * hx + hy * hy);

        // Edge collision counts
        if (hDist <= (snakeRadius + hazardRadius)) {
            score -= 10;
            if (score < 0) score = 0;

            hazardX = -450;
            hazardY = (rand() % 360) - 180;
            playSoundEffect("collision");
        }

        // --- NEW: LEVEL 2 VERTICAL BUTTERFLY COLLISION ---
        if (level == 2) {
            float bx = snake[0].x - bfX;
            float by = snake[0].y - bfY;
            float bDist = sqrt(bx * bx + by * by);

            if (bDist <= (snakeRadius + hazardRadius)) {
                score -= 10;
                if (score < 0) score = 0;

                bfY = -350;  // Reset butterfly position
                playSoundEffect("collision");
            }
        }
    }

    glutPostRedisplay();  // Trigger screen refresh
    glutTimerFunc(snakeDelay, update, 0);  // Schedule next update
}

/*
 * GLUT reshape callback - adjusts viewport when window resized
 * Maintains proper aspect ratio for game world
 * w, h: New window dimensions
 */
void reshape(int w, int h)
{
    if (h == 0) h = 1;  // Prevent division by zero

    glViewport(0, 0, w, h);  // Set drawing area

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = (float)w / (float)h;

    if (aspect >= 4.0f / 3.0f) {
        // Wide window (e.g., 16:9) - extend world width
        float worldWidth = 300.0f * aspect;
        gluOrtho2D(-worldWidth, worldWidth, -300, 300);
    }
    else {
        // Tall window - extend world height
        float worldHeight = 400.0f / aspect;
        gluOrtho2D(-400, 400, -worldHeight, worldHeight);
    }

    glMatrixMode(GL_MODELVIEW);
}

/*
 * Draws text centered on screen in screen coordinates (not world coordinates)
 * Used for UI elements that should stay fixed on screen
 * yOffset: Vertical offset from center
 * text: String to display
 * font: GLUT bitmap font
 */
void drawCenteredTextScreen(float yOffset,
    const std::string& text,
    void* font = GLUT_BITMAP_HELVETICA_18)
{
    int winW = glutGet(GLUT_WINDOW_WIDTH);
    int winH = glutGet(GLUT_WINDOW_HEIGHT);

    // Calculate text width
    int textWidth = 0;
    for (char c : text) {
        textWidth += glutBitmapWidth(font, c);
    }

    // Switch to screen space (pixel coordinates)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();  // Save current projection
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);  // Pixel coordinates

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();  // Save current modelview
    glLoadIdentity();

    // Calculate centered position
    float x = (winW - textWidth) / 2.0f;
    float y = winH / 2.0f + yOffset;

    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }

    // Restore matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

/*
 * GLUT display callback - renders everything to screen
 * Called whenever screen needs refreshing
 */
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (showMenu) {
        // Draw main menu
        glClearColor(0.65f, 0.80f, 0.65f, 1.0f);  // Light green background

        glColor3f(1, 1, 1);
        for (const auto& t : trees) {
            drawSingleTree(t.x, t.y);
        }

        // Draw menu text
        drawCenteredTextScreen(180, "ECO-SERPENT: FOREST CLEANUP", GLUT_BITMAP_TIMES_ROMAN_24);
        drawCenteredTextScreen(120, "HOW TO PLAY:");
        drawCenteredTextScreen(80, "Use ARROW KEYS to navigate.");
        drawCenteredTextScreen(50, "Eat garbage for +10 points.");
        drawCenteredTextScreen(20, "Avoid RED Butterflies! (Penalty: -10)");
        drawCenteredTextScreen(-10, "Avoid trees and boundaries!");

        glColor3f(1, 1, 0);  // Yellow for level selection
        drawCenteredTextScreen(-60, "Select Cleaning Level:");
        drawCenteredTextScreen(-90, "Press '1' for EASY JUNGLE");
        drawCenteredTextScreen(-120, "Press '2' for HARD JUNGLE");

        glColor3f(1.0f, 1.0f, 1.0f);  // White for exit
        drawCenteredTextScreen(-170, "Press 'S' to ON/OFF Sound");
        drawCenteredTextScreen(-200, "Press 'ESC' to EXIT");

    }
    else {
        // Draw game screen
        glClearColor(0.8f, 0.9f, 0.8f, 1.0f);  // Game background

        // Draw border trees
        trees.clear();
        for (int i = -400; i <= 400; i += 50) {
            drawSingleTree(i, 270);  // Top border
            trees.push_back({ (float)i, 270.0f });

            drawSingleTree(i, -285);  // Bottom border
            trees.push_back({ (float)i, -285.0f });
        }

        for (int i = -250; i <= 250; i += 60) {
            drawSingleTree(-390, i);  // Left border
            trees.push_back({ -390.0f, (float)i });

            drawSingleTree(390, i);   // Right border
            trees.push_back({ 390.0f, (float)i });
        }

        // --- NEW: LEVEL 2 EXTRA TREES AND VERTICAL BUTTERFLY ---
        if (level == 2) {
            drawSingleTree(-150, 100);  // Extra obstacle
            trees.push_back({ -150.0f, 100.0f });

            drawSingleTree(150, -100);  // Extra obstacle
            trees.push_back({ 150.0f, -100.0f });

            drawButterfly(bfX, bfY);    // Blue butterfly
        }

        // Draw particles
        for (auto& p : particles) {
            glColor4f(p.r, p.g, p.b, p.life);  // Use alpha for fading
            glPushMatrix();
            glTranslatef(p.x, p.y, 0);
            glBegin(GL_QUADS);
            glVertex2f(-2, -2); glVertex2f(2, -2);
            glVertex2f(2, 2); glVertex2f(-2, 2);
            glEnd();
            glPopMatrix();
        }

        // Draw red hazard butterfly
        glPushMatrix();
        glTranslatef(hazardX, hazardY, 0);
        float w = sinf(animationTimer * 0.8f) * 12.0f;  // Animate wings
        glColor3f(1.0f, 0.0f, 0.0f);  // Red
        glBegin(GL_TRIANGLES);
        // Left wing
        glVertex2f(0.0f, 0.0f);
        glVertex2f(-15.0f - w, 15.0f);
        glVertex2f(-15.0f - w, -15.0f);
        // Right wing
        glVertex2f(0.0f, 0.0f);
        glVertex2f(15.0f + w, 15.0f);
        glVertex2f(15.0f + w, -15.0f);
        glEnd();
        glPopMatrix();

        // Draw game objects
        drawFood();
        drawSnake();

        // Draw score overlay (semi-transparent black bar)
        glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
        glBegin(GL_QUADS);
        glVertex2f(-400.0f, 300.0f);
        glVertex2f(400.0f, 300.0f);
        glVertex2f(400.0f, 265.0f);
        glVertex2f(-400.0f, 265.0f);
        glEnd();

        // Draw score text
        glColor3f(1.0f, 1.0f, 1.0f);
        int target = (level == 1) ? 100 : 100;  // Both levels need 100 points
        std::string lvlName = (level == 1) ? "EASY" : "HARD";
        drawText(-380, 275, "SCORE: " + std::to_string(score) + "/" + std::to_string(target));
        drawText(280, 275, "LEVEL: " + lvlName);

        // Draw countdown if active
        if (inCountdown) {
            glColor3f(0.0f, 0.0f, 0.0f);
            if (countdownValue > 0) {
                drawCenteredText(40, std::to_string(countdownValue), GLUT_BITMAP_TIMES_ROMAN_24);
                drawCenteredText(0, "READY?", GLUT_BITMAP_HELVETICA_18);
            }
            else drawCenteredText(20, "START", GLUT_BITMAP_TIMES_ROMAN_24);
        }

        // Draw game over screen
        if (gameOver) {
            glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
            glBegin(GL_QUADS);
            glVertex2f(-200.0f, 50.0f);
            glVertex2f(200.0f, 50.0f);
            glVertex2f(200.0f, -100.0f);
            glVertex2f(-200.0f, -100.0f);
            glEnd();

            glColor3f(1.0f, 0.0f, 0.0f);
            drawCenteredText(10, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawCenteredText(-30, "Press 'R' to Restart This Level");
            drawCenteredText(-60, "Press 'M' For Menu");
        }

        // Draw win screen
        if (gameWin) {
            glColor4f(0, 0, 0, 0.7f);
            glBegin(GL_QUADS);
            glVertex2f(-250.0f, 80.0f);
            glVertex2f(250.0f, 80.0f);
            glVertex2f(250.0f, -100.0f);
            glVertex2f(-250.0f, -100.0f);
            glEnd();

            glColor3f(0, 1, 0);
            drawCenteredText(40, "SUCCESSFULLY PROTECTED THE ENVIRONMENT!", GLUT_BITMAP_HELVETICA_12);
            glColor3f(1, 1, 1);

            if (level == 1) {
                // Level 1 win: option to continue to level 2
                drawCenteredText(-10, "Press 'C' To Start Level 2");
                drawCenteredText(-40, "Press 'M' For Menu");
            }
            else {
                // Level 2 win: complete game
                drawCenteredText(-10, "CONGRATULATIONS! THE JUNGLE IS PURE!");
                drawCenteredText(-40, "Press 'M' For Menu");
            }
        }

        // Draw sound status
        glColor3f(0.7, 0.7, 0.7);
        drawText(280, -280, "Sound: " + std::string(soundEnabled ? "ON" : "OFF"));
    }

    glutSwapBuffers();  // Double buffering for smooth animation
}

// GLUT special key callback - handles arrow keys
void specialKey(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        if (direction != 2) nextDirection = 0; // Prevent 180° turn
        break;
    case GLUT_KEY_RIGHT:
        if (direction != 3) nextDirection = 1;
        break;
    case GLUT_KEY_DOWN:
        if (direction != 0) nextDirection = 2;
        break;
    case GLUT_KEY_LEFT:
        if (direction != 1) nextDirection = 3;
        break;
    }
}

/*
 * GLUT keyboard callback - handles regular key presses
 * key: ASCII key pressed
 * x, y: Mouse position (unused)
 */
void keyboard(unsigned char key, int x, int y) {
    // Level selection
    if (key == '1') {
        level = 1;
        showMenu = false;
        initGame();
        glutTimerFunc(1000, handleCountdown, 0);
    }
    if (key == '2') {
        level = 2;
        showMenu = false;
        initGame();
        glutTimerFunc(1000, handleCountdown, 0);
    }

    // Menu navigation
    if (key == 'm' || key == 'M') showMenu = true;

    // Restart current level
    if ((key == 'r' || key == 'R') && (gameOver || !showMenu)) {
        initGame(true);
        glutTimerFunc(1000, handleCountdown, 0);
    }

    // Continue to next level after winning level 1
    if ((key == 'c' || key == 'C') && gameWin && level == 1) {
        level = 2;
        playSoundEffect("levelup");
        initGame(true);
        glutTimerFunc(1000, handleCountdown, 0);
    }

    // Toggle sound
    if (key == 's' || key == 'S') {
        toggleSound();
        glutPostRedisplay();  // Update sound status display
    }

    // Exit game (ESC key)
    if (key == 27) exit(0);
}

/*
 * Main function - program entry point
 * Initializes GLUT, sets up callbacks, and starts main loop
 */
int main(int argc, char** argv) {
    srand(time(NULL));  // Seed random number generator

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);  // Double buffering, RGB mode
    glutInitWindowSize(800, 600);
    glutCreateWindow("Eco-Serpent: Forest Cleanup");

    // Set up 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-400, 400, -300, 300);  // World coordinates
    glMatrixMode(GL_MODELVIEW);

    // Enable alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Register callback functions
    glutDisplayFunc(display);      // Rendering
    glutKeyboardFunc(keyboard);    // Keyboard input
    glutSpecialFunc(specialKey);   // Special keys (arrows)
    glutTimerFunc(100, update, 0); // Game timer

    // Start main event loop
    glutMainLoop();

    return 0;
}

