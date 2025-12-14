/*
 * 3D Flight Shooter - Final Project
 * Author: Matus Vecera
 * Date: December 2025
 * FundComp Lab 11 mini-Final Project
 * 
 * A first-person 3D wireframe flight game where you shoot obstacles.
 * Score 1000 points to see a special thank-you screen!
 * 
 * Controls:
 *   Mouse  - Steer (fly toward cursor)
 *   Click  - Shoot
 *   +/-    - Adjust speed
 *   Q      - Quit
 */

#define _XOPEN_SOURCE 500 // This is for the usleep function
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for usleep
#include <math.h>
#include <time.h> // for time()
#include "gfx.h"

/* ==================== CONSTANTS  ==================== */
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define SCREEN_CX 400
#define SCREEN_CY 300
#define GRID_SIZE 20
#define GRID_SPACING 25
#define RENDER_DISTANCE 1200
#define RENDER_DIST_SQ (RENDER_DISTANCE * RENDER_DISTANCE)
#define MAX_OBSTACLES 15
#define MAX_BULLETS 10
#define BULLET_SPEED 15.0
#define WIN_SCORE 1000
#define PI 3.14159265358979 //I made this becuase PI constant in math libary was being weird
#define FOV_SCALE 0.8 // Field of view scaling factor
#define PROJ_DISTANCE 300.0 // Distance from camera to projection plane
#define START_SPEED 1.5

/* ==================== DATA STRUCTURES ==================== */
// 3D point structure
typedef struct {
    double x, y, z;
} Point3D;

// Camera structure
typedef struct {
    Point3D position;
    double pitch, yaw; // rotation angles
    double cos_pitch, sin_pitch, cos_yaw, sin_yaw;  /* precomputed trig */
    double speed; // movement speed
} Camera;

// Bullet structure
typedef struct {
    Point3D position; //bullet position
    Point3D velocity; //bullet velocity
    int active; //1=active, 0=inactive
} Bullet;

// Obstacle structure
typedef struct {
    Point3D position;              
    double size;
    double rotation;
    int active;      
} Obstacle;

//camera and game state
typedef struct {
    Camera camera;
    Bullet bullets[MAX_BULLETS];
    Obstacle obstacles[MAX_OBSTACLES];
    int score;
    int lives;
    int is_moving;
    int show_Win_Screen;  /* unlocked when score >= WIN_SCORE */
    int game_over;       /* 1 = crashed/died */
    time_t start_time;   /* when game started */  
    int final_time;      /* seconds to win (frozen at win) */                  
} GameState;

/* ==================== FUNCTION DECLARATIONS ==================== */

void init_game(GameState *game);
void update_camera_trig(Camera *cam);
void project_point(Point3D p, Camera *cam, int *sx, int *sy);
double get_terrain_height(GameState *game, double x, double z);
void draw_sky(void);
void draw_terrain(GameState *game);
void draw_win_screen(GameState *game);
void draw_lose_screen(GameState *game);
void draw_obstacles(GameState *game);
void draw_bullets(GameState *game);
void draw_crosshair(void);
void draw_hud(GameState *game);
void update_bullets(GameState *game);
void update_obstacles(GameState *game);
void fire_bullet(GameState *game);
void check_collisions(GameState *game);

/* ==================== MAIN FUNCTION ==================== */

int main(void) {
    GameState game; // main game state
    char c;
    int mouse_x, mouse_y;
    double target_yaw, target_pitch; // desired camera angles based on mouse
    double steer_speed = 0.06;  /* How fast camera turns toward mouse */
    
    srand(time(NULL)); // Seed random number generator
    init_game(&game); // Initialize game state
    
    gfx_open(SCREEN_WIDTH, SCREEN_HEIGHT, "3D Flight Shooter - Fly toward mouse, Left CLick to shoot!"); //  Open graphics window
    
    /* Main game loop */
    while (1) {
        // Get mouse position and steer toward it
        mouse_x = gfx_xpos();
        mouse_y = gfx_ypos();
        
        // Calculate how much to turn based on mouse offset from center
        // Mouse left of center = turn left (negative yaw change)
        target_yaw = (mouse_x - SCREEN_CX) * 0.0008;  // Scale mouse offset to rotation
        target_pitch = (mouse_y - SCREEN_CY) * 0.0006;  // Pitch based on vertical offset
        
        // Smoothly steer toward mouse direction
        game.camera.yaw += target_yaw * steer_speed;
        game.camera.pitch += target_pitch * steer_speed;
        
        // Clamp pitch, which is more limited than yaw
        if (game.camera.pitch < -1.2) game.camera.pitch = -1.2;
        if (game.camera.pitch > 0.8) game.camera.pitch = 0.8;
        
        // Update trig values (used for movement and rendering)
        update_camera_trig(&game.camera);
        
        // Always move forward - I am flying!
        game.camera.position.x += game.camera.speed * game.camera.sin_yaw * game.camera.cos_pitch;
        game.camera.position.z += game.camera.speed * game.camera.cos_yaw * game.camera.cos_pitch;
        game.camera.position.y += game.camera.speed * game.camera.sin_pitch;
        
        // Check ground collision = death
        {
            double ground = get_terrain_height(&game, game.camera.position.x, game.camera.position.z) + 15;
            if (game.camera.position.y < ground) {
                game.game_over = 1;
                printf("\a");  // Crash sound
                printf("\n*** CRASHED INTO GROUND! ***\n");
            }
        }
                             
        // Check if game over - show lose screen
        if (game.game_over) {
            game.final_time = (int)(time(NULL) - game.start_time);
            printf("*** GAME OVER! ***\n");
            printf("Final Score: %d\n", game.score);
            
            draw_lose_screen(&game);
            gfx_flush();
            
            // Wait for user to quit or restart
            while (1) {
                c = gfx_wait();
                if (c == 'q' || c == 'Q') { //quit
                    return 0;
                }
                if (c == 'r' || c == 'R') { // restart
                    init_game(&game); // Restart game
                    gfx_clear_color(0, 0, 0);  /* Reset background to black */
                    break;  /* Restart game loop */
                }
            }
        }
        
        // Check if player won - show win screen
        if (game.score >= WIN_SCORE && !game.show_Win_Screen) { // won
            game.show_Win_Screen = 1;
            game.final_time = (int)(time(NULL) - game.start_time);  // Freeze time
            printf("\n*** CONGRATULATIONS! You won in %d:%02d! ***\n", game.final_time / 60, game.final_time % 60);
            printf("*** Press Q to quit, R to restart ***\n\n");
            
            // Draw win screen once
            draw_win_screen(&game);
            gfx_flush();
            
            // Wait for user to quit or restart
            while (1) {
                c = gfx_wait();
                if (c == 'q' || c == 'Q') {
                    printf("Final Score: %d\n", game.score);
                    return 0;
                }
                if (c == 'r' || c == 'R') {
                    init_game(&game);
                    gfx_clear_color(0, 0, 0);  // Reset background to black
                    break;  // Restart game loop
                }
            }
        }
        
        // Update game state
        update_bullets(&game);
        update_obstacles(&game);
        check_collisions(&game);
        
        // Draw everything
        gfx_clear();
        draw_sky();
        draw_terrain(&game);
        draw_obstacles(&game);
        draw_bullets(&game);
        draw_crosshair();
        draw_hud(&game);
        gfx_flush();
        
        usleep(12000);  /* ~80 FPS for smoother animation */
        
        /* Handle input */
        while (gfx_event_waiting()) {
            c = gfx_wait();
            
            if (c == 1) fire_bullet(&game);  /* mouse click = shoot */
            if (c == 'q' || c == 'Q') {
                printf("Final Score: %d\n", game.score);
                return 0;
            }
            if (c == '=' || c == '+') {
                game.camera.speed += 0.5;
                if (game.camera.speed > 10.0) game.camera.speed = 10.0;
                printf("Speed: %.1f\n", game.camera.speed);
            }
            if (c == '-' || c == '_') {
                game.camera.speed -= 0.5;
                if (game.camera.speed < 1.0) game.camera.speed = 1.0;
                printf("Speed: %.1f\n", game.camera.speed);
            }
        }
    }
    
    return 0;
}

/* ==================== INITIALIZATION ==================== */

void init_game(GameState *game) {
    int i;
    //set up the initial conditions of the game
    game->camera.position.x = 0.0; // Start at origin
    game->camera.position.y = 300.0;  // Start higher
    game->camera.position.z = 0.0; // Start at origin
    game->camera.pitch = 0.0;  // Looking straight ahead
    game->camera.yaw = 0.0; // Facing along +Z axis
    game->camera.speed = START_SPEED; // Moderate speed
    
    game->score = 0;
    game->lives = 3;
    game->is_moving = 1;
    game->show_Win_Screen = 0;
    game->game_over = 0;
    game->start_time = time(NULL);
    game->final_time = 0;
    
    for (i = 0; i < MAX_BULLETS; i++) { //initialize bullets
        game->bullets[i].active = 0;
    }
    for (i = 0; i < MAX_OBSTACLES; i++) { //initialize obstacles
        game->obstacles[i].active = 0;
    }
}

/* ==================== CAMERA & PROJECTION ==================== */
// Update precomputed trig values for camera
void update_camera_trig(Camera *cam) {
    cam->cos_pitch = cos(cam->pitch);
    cam->sin_pitch = sin(cam->pitch);
    cam->cos_yaw = cos(cam->yaw);
    cam->sin_yaw = sin(cam->yaw);
}


// Project a 3D point to 2D screen coordinates
void project_point(Point3D p, Camera *cam, int *sx, int *sy) {
    double dx, dy, dz;
    double rx, ry, rz, ty, tz;
    double scale;
    
    // Translate to camera space
    dx = p.x - cam->position.x; // make sure that i get the relative position to the camera for these coordinates
    dy = p.y - cam->position.y;
    dz = p.z - cam->position.z;
    
    // Rotate by yaw
    rx = dx * cam->cos_yaw - dz * cam->sin_yaw;
    rz = dx * cam->sin_yaw + dz * cam->cos_yaw;
    ry = dy;
    
    // Rotate by pitch
    ty = ry; // temporary y
    tz = rz; //
    ry = ty * cam->cos_pitch - tz * cam->sin_pitch;
    rz = ty * cam->sin_pitch + tz * cam->cos_pitch;
    
    // Perspective projection - mark behind-camera points as off-screen
    if (rz < 20.0) {
        *sx = -9999;  // Mark as invalid/behind camera
        *sy = -9999;
        return;
    }
    scale = PROJ_DISTANCE / rz * FOV_SCALE; // FOV scaling, which stands for field of view, makes things look less distorted
    
    *sx = (int)(rx * scale) + SCREEN_CX; // center on screen
    *sy = (int)(-ry * scale) + SCREEN_CY; // invert y for screen coords
}




/* ==================== SKY BACKGROUND ==================== */

// Draw simple sky with sun and rays
void draw_sky(void) {
    int i;
    int horizon = SCREEN_CY + 50;  /* Horizon line position */
    /* Precomputed sun ray endpoints (8 rays at 45 degree intervals) */
    static const int ray_dx[] = {45, 31, 0, -31, -45, -31, 0, 31};
    static const int ray_dy[] = {0, 31, 45, 31, 0, -31, -45, -31};
    static const int ray2_dx[] = {60, 42, 0, -42, -60, -42, 0, 42};
    static const int ray2_dy[] = {0, 42, 60, 42, 0, -42, -60, -42};
    
    // Draw gradient horizon lines (wireframe style)
    gfx_color(30, 30, 80);  // Dark blue at top
    for (i = 0; i < horizon; i += 40) {
        gfx_line(0, i, SCREEN_WIDTH, i);
    }
    
    // Draw a simple wireframe sun (top right)
    gfx_color(255, 200, 50);  // Yellow/orange
    gfx_circle(650, 80, 40);  // Sun circle
    gfx_circle(650, 80, 35);  // Inner ring
    
    // Sun rays (precomputed)
    for (i = 0; i < 8; i++) {
        gfx_line(650 + ray_dx[i], 80 + ray_dy[i], 650 + ray2_dx[i], 80 + ray2_dy[i]);
    }
}


/* ==================== TERRAIN ==================== */

// Simple procedural terrain height function
double get_terrain_height(GameState *game, double x, double z) {
    (void)game;  // unused parameter
    // Sine wave terrain
    return 30.0 * sin(x * 0.01) * sin(z * 0.01) +
           15.0 * sin(x * 0.03 + z * 0.02);
}

// Draw wireframe terrain grid
void draw_terrain(GameState *game) {
    int i, j, x1, y1, x2, y2; // screen coords
    double baseX, baseZ, wx, wz, h1, h2; // world coords
    double dx, dz, distSq; // distance squared
    Point3D p1, p2; // 3D points
    int gridSize; // grid size
    double spacing; // grid spacing
    double camX = game->camera.position.x; // camera position
    double camZ = game->camera.position.z; // camera position
    
    spacing = GRID_SPACING;
    gridSize = GRID_SIZE;
    
    baseX = ((int)(camX / spacing)) * spacing; // base grid cell X
    baseZ = ((int)(camZ / spacing)) * spacing; // base grid cell Z
    
    for (i = -gridSize; i < gridSize; i++) { // for each grid line in X direction
        for (j = -gridSize; j < gridSize; j++) { // for each grid line in Z direction
            wx = baseX + i * spacing; // world X
            wz = baseZ + j * spacing; // world Z
            
            dx = wx - camX; // horizontal distance from camera
            dz = wz - camZ; // horizontal distance from camera
            distSq = dx * dx + dz * dz; // squared distance
            if (distSq > RENDER_DIST_SQ) continue; // skip if too far
            
            h1 = get_terrain_height(game, wx, wz); // height at this grid point
            
            gfx_color(100, 255, 100);  /* Green terrain */
            
            /* Draw X-direction line */
            h2 = get_terrain_height(game, wx + spacing, wz); // height at next grid point
            p1.x = wx; p1.y = h1; p1.z = wz; // first point
            p2.x = wx + spacing; p2.y = h2; p2.z = wz; // second point
            project_point(p1, &game->camera, &x1, &y1); // project first point
            project_point(p2, &game->camera, &x2, &y2); // project second point
            /* Only draw if both points are in front of camera */
            if (x1 > -9000 && x2 > -9000 && // both points valid
                (x1 > -200 && x1 < SCREEN_WIDTH + 200) &&  // first point on screen
                (x2 > -200 && x2 < SCREEN_WIDTH + 200)) { // second point on screen
                gfx_line(x1, y1, x2, y2); // draw line
            }
            
            /* Draw Z-direction line */
            h2 = get_terrain_height(game, wx, wz + spacing); // height at next grid point
            p2.x = wx; p2.y = h2; p2.z = wz + spacing; // second point
            project_point(p2, &game->camera, &x2, &y2); // project second point
            /* Only draw if both points are in front of camera */
            if (x1 > -9000 && x2 > -9000 && // both points valid
                (x1 > -200 && x1 < SCREEN_WIDTH + 200) &&  // first point on screen
                (x2 > -200 && x2 < SCREEN_WIDTH + 200)) { // second point on screen
                gfx_line(x1, y1, x2, y2); // draw line
            }
        }
    }
    
    gfx_color(255, 255, 255);
}



/* ==================== WIN SCREEN ==================== */

// Helper function to draw a scaled PPM image at position
void draw_ppm_scaled(const char *filename, int destX, int destY, int destW, int destH) {
    FILE *file;
    char magic[3]; // PPM magic number
    int width, height, maxval, c, x, y; // loop vars
    int r, g, b;
    unsigned char *imgR, *imgG, *imgB;
    double scaleX, scaleY;
    int srcX, srcY, dx, dy;
    
    file = fopen(filename, "rb"); //means reading binary
    if (!file) return;
    
    /* Read PPM header */
    if (fscanf(file, "%2s", magic) != 1) { fclose(file); return; }
    // Read magic number (e.g., "P6")
    /* Skip comments */
    c = fgetc(file);
    while (c == '#' || c == '\n' || c == ' ') {
        if (c == '#') while (fgetc(file) != '\n');
        c = fgetc(file);
    }
    ungetc(c, file);
    
    if (fscanf(file, "%d %d %d", &width, &height, &maxval) != 3) { fclose(file); return; }
    fgetc(file);
    
    // Allocate memory for image, the reason that I use malloc instead of a static array is that the image size can vary, and malloc allows for dynamic allocation based on the image dimensions
    // If the image is too large, then the static array may exceed stack size, so we use heap allocation instead
    imgR = malloc(width * height);
    imgG = malloc(width * height);
    imgB = malloc(width * height);
    if (!imgR || !imgG || !imgB) { fclose(file); return; }
    
    /* Read pixel data */
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            r = fgetc(file);
            g = fgetc(file);
            b = fgetc(file);
            imgR[y * width + x] = (unsigned char)r;
            imgG[y * width + x] = (unsigned char)g;
            imgB[y * width + x] = (unsigned char)b;
        }
    }
    fclose(file);
    
    /* Draw scaled image */
    scaleX = (double)width / destW;
    scaleY = (double)height / destH;
    
    // Draw each pixel in destination, we use 1d arrays here for each color
    for (dy = 0; dy < destH; dy++) {
        for (dx = 0; dx < destW; dx++) {
            srcX = (int)(dx * scaleX); // source X coordinate in original image
            srcY = (int)(dy * scaleY);
            if (srcX >= width) srcX = width - 1;
            if (srcY >= height) srcY = height - 1;
            
            gfx_color(imgR[srcY * width + srcX],
                      imgG[srcY * width + srcX],
                      imgB[srcY * width + srcX]);
            gfx_point(destX + dx, destY + dy);
        }
    }
    
    //free up the memory when Im done with it
    free(imgR);
    free(imgG);
    free(imgB);
}

// Draw the win screen with professor and TAs
void draw_win_screen(GameState *game) {
    int profSize = 220;  /* Professor image size - even bigger */
    int taSize = 95;     // TA image size
    int profX, profY;
    int i;
    
    // TA filenames - 14 TAs
    const char *ta_files[] = {
        "693d9e2026f2d.ppm", "693d9e7737592.ppm", "asvenss2.ppm", "cmassman.ppm",
        "fdrake.ppm", "hflick.ppm", "jnkouka.ppm", "maiyener.ppm",
        "mbriamon.ppm", "mzitella.ppm", "schou2.ppm", "sco.ppm",
        "sdevared.ppm", "thieber.ppm"
    };
    int numTAs = 14;
    
    // Clear to dark blue
    gfx_clear_color(20, 20, 50);
    gfx_clear();
    
    // Draw "Thank you" text at top - single line
    gfx_color(255, 255, 0);  // Yellow text
    gfx_text(SCREEN_WIDTH/2 - 250, 25, "Thank you Professor Ramzi and all TAs for a wonderful semester!");
    
    // Draw professor in center (bigger)
    profX = SCREEN_WIDTH/2 - profSize/2;
    profY = SCREEN_HEIGHT/2 - profSize/2 + 5;
    draw_ppm_scaled("ramzinew.ppm", profX, profY, profSize, profSize);
    
    // Gold border around professor
    gfx_color(255, 215, 0);
    gfx_line(profX - 3, profY - 3, profX + profSize + 3, profY - 3);
    gfx_line(profX + profSize + 3, profY - 3, profX + profSize + 3, profY + profSize + 3);
    gfx_line(profX + profSize + 3, profY + profSize + 3, profX - 3, profY + profSize + 3);
    gfx_line(profX - 3, profY + profSize + 3, profX - 3, profY - 3);
    
    // Draw TAs around the professor - use more space
    // Top row: 5 TAs
    for (i = 0; i < 5 && i < numTAs; i++) {
        int x = 25 + i * (taSize + 40);
        int y = 50;
        draw_ppm_scaled(ta_files[i], x, y, taSize, taSize);
    }
    
    // Left column: 2 TAs
    for (i = 0; i < 2; i++) {
        int x = 25;
        int y = 160 + i * (taSize + 15);
        if (5 + i < numTAs) {
            draw_ppm_scaled(ta_files[5 + i], x, y, taSize, taSize);
        }
    }
    
    // Right column: 2 TAs
    for (i = 0; i < 2; i++) {
        int x = SCREEN_WIDTH - taSize - 25;
        int y = 160 + i * (taSize + 15);
        if (7 + i < numTAs) {
            draw_ppm_scaled(ta_files[7 + i], x, y, taSize, taSize);
        }
    }
    
    /* Bottom row: 5 TAs */
    for (i = 0; i < 5 && 9 + i < numTAs; i++) {
        int x = 25 + i * (taSize + 40);
        int y = SCREEN_HEIGHT - taSize - 25;
        draw_ppm_scaled(ta_files[9 + i], x, y, taSize, taSize);
    }
    
    /* Score and time at very bottom */
    gfx_color(100, 255, 100);  /* Green */
    {
        char time_str[64];
        int mins = game->final_time / 60;
        int secs = game->final_time % 60;
        sprintf(time_str, "Won in %d:%02d! R=Restart Q=Quit", mins, secs);
        gfx_text(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT - 8, time_str);
    }
}



// Draw the lose screen with score/time
void draw_lose_screen(GameState *game) {
    char msg[64];
    int mins = game->final_time / 60;
    int secs = game->final_time % 60;
    
    /* Dark red background */
    gfx_clear_color(60, 20, 20);
    gfx_clear();
    
    // Big "GAME OVER" text
    gfx_color(255, 50, 50);  // Red
    gfx_text(SCREEN_CX - 40, SCREEN_CY - 60, "GAME OVER");
    
    // Score info
    gfx_color(255, 200, 100);  // Orange
    sprintf(msg, "Score: %d", game->score);
    gfx_text(SCREEN_CX - 35, SCREEN_CY + 80, msg);
    
    sprintf(msg, "Time: %d:%02d", mins, secs);
    gfx_text(SCREEN_CX - 35, SCREEN_CY + 100, msg);
    
    gfx_color(150, 150, 150);  /* Gray */
    gfx_text(SCREEN_CX - 75, SCREEN_CY + 140, "R to Restart | Q to Quit");
}

/* ==================== OBSTACLES ==================== */

// Helper to check if a projected point is valid (not behind camera) 
int valid_point(int x, int y) {
    return (x != -9999 && y != -9999);
}

// Helper to draw a line only if both endpoints are valid
void safe_line(int x1, int y1, int x2, int y2) {
    if (valid_point(x1, y1) && valid_point(x2, y2)) {
        gfx_line(x1, y1, x2, y2);
    }
}

// Draw a wireframe cube obstacle
void draw_wireframe_cube(Point3D center, double size, double rot, Camera *cam) {
    double half = size * 0.5;
    double cosR = cos(rot), sinR = sin(rot);
    Point3D corners[8];
    int px[8], py[8];
    int i;
    double lx, lz, rx, rz;
    double dx, dz, distSq, minDistSq;
    
    /* Skip cubes that are too close to the camera (avoid sqrt) */
    dx = center.x - cam->position.x;
    dz = center.z - cam->position.z;
    distSq = dx*dx + dz*dz;
    minDistSq = size * size * 2.25;  /* (1.5 * size)^2 */
    if (distSq < minDistSq) {
        return;  /* Too close, don't draw */
    }
    
    /* Define and rotate 8 corners */
    double local[8][3] = {
        {-half, -half, -half}, {half, -half, -half},
        {half, half, -half}, {-half, half, -half},
        {-half, -half, half}, {half, -half, half},
        {half, half, half}, {-half, half, half}
    };
    
    // Rotate and translate corners
    for (i = 0; i < 8; i++) {
        lx = local[i][0];
        lz = local[i][2];
        rx = lx * cosR - lz * sinR;
        rz = lx * sinR + lz * cosR;
        corners[i].x = center.x + rx;
        corners[i].y = center.y + local[i][1];
        corners[i].z = center.z + rz;
        project_point(corners[i], cam, &px[i], &py[i]);
    }
    
    // Draw 12 edges (only if both endpoints are valid)
    safe_line(px[0], py[0], px[1], py[1]);
    safe_line(px[1], py[1], px[2], py[2]);
    safe_line(px[2], py[2], px[3], py[3]);
    safe_line(px[3], py[3], px[0], py[0]);
    safe_line(px[4], py[4], px[5], py[5]);
    safe_line(px[5], py[5], px[6], py[6]);
    safe_line(px[6], py[6], px[7], py[7]);
    safe_line(px[7], py[7], px[4], py[4]);
    safe_line(px[0], py[0], px[4], py[4]);
    safe_line(px[1], py[1], px[5], py[5]);
    safe_line(px[2], py[2], px[6], py[6]);
    safe_line(px[3], py[3], px[7], py[7]);
}

// Draw all active obstacles
void draw_obstacles(GameState *game) {
    int i;
    
    gfx_color(255, 100, 100);  /* Red obstacles */
    // Draw each active obstacle
    for (i = 0; i < MAX_OBSTACLES; i++) {
        if (game->obstacles[i].active) {
            draw_wireframe_cube(game->obstacles[i].position,
                               game->obstacles[i].size,
                               game->obstacles[i].rotation,
                               &game->camera);
        }
    }
    
    gfx_color(255, 255, 255);
}

// Update obstacle positions, spawn new ones
void update_obstacles(GameState *game) {
    int i, active_count = 0;
    double dist, angle;
    double dx, dz, distSqFromCam;
    double maxDistSq = 1500.0 * 1500.0;  /* Precompute threshold */
    Obstacle *obs;
    
    // Update existing obstacles
    for (i = 0; i < MAX_OBSTACLES; i++) {
        obs = &game->obstacles[i]; // get pointer to obstacle
        if (obs->active) { // if active
            active_count++; // count active obstacles
            obs->rotation += 0.02; // rotate obstacle
            
            // Remove if too far from camera (any direction) - avoid sqrt
            dx = obs->position.x - game->camera.position.x;
            dz = obs->position.z - game->camera.position.z;
            distSqFromCam = dx*dx + dz*dz;
            if (distSqFromCam > maxDistSq) {
                obs->active = 0;
                active_count--;
            }
        }
    }
    
    // Spawn new obstacles (but not after the game is over)
    if (!game->show_Win_Screen && active_count < 8 && rand() % 40 == 0) {
        for (i = 0; i < MAX_OBSTACLES; i++) {
            if (!game->obstacles[i].active) { // find inactive obstacle
                obs = &game->obstacles[i]; // get pointer to it
                obs->active = 1; // activate it
                
                // Scatter around field of vision - random angle within ~120 degree FOV
                dist = 400 + rand() % 600; // Distance from camera
                angle = game->camera.yaw + ((rand() % 120) - 60) * PI / 180.0;  // -60 to +60 degrees 
                
                obs->position.x = game->camera.position.x + dist * sin(angle);
                obs->position.z = game->camera.position.z + dist * cos(angle);
                obs->position.y = get_terrain_height(game, obs->position.x, obs->position.z) 
                                  + 30 + rand() % 100;  // More height variation 
                obs->size = 30 + rand() % 30;
                obs->rotation = 0;
                break;
            }
        }
    }
}



/* ==================== BULLETS ==================== */

// Draw all active bullets
void draw_bullets(GameState *game) {
    int i, sx, sy;
    
    gfx_color(255, 255, 0);  /* Yellow bullets */
    
    for (i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            project_point(game->bullets[i].position, &game->camera, &sx, &sy);
            if (sx > 0 && sx < SCREEN_WIDTH && sy > 0 && sy < SCREEN_HEIGHT) {
                gfx_line(sx - 3, sy, sx + 3, sy); //this just draws a cross for the bullet
                gfx_line(sx, sy - 3, sx, sy + 3);
            }
        }
    }
    
    gfx_color(255, 255, 255);
}

// Fire a new bullet from camera position
void fire_bullet(GameState *game) {
    int i;
    Bullet *b;
    Camera *cam = &game->camera;

    // Find inactive bullet slot
    for (i = 0; i < MAX_BULLETS; i++) {
        if (!game->bullets[i].active) {
            b = &game->bullets[i];
            b->active = 1;
            b->position = cam->position;
            // Use cached trig values 
            b->velocity.x = BULLET_SPEED * cam->sin_yaw * cam->cos_pitch; // set x velocity
            b->velocity.y = BULLET_SPEED * cam->sin_pitch; // set y velocity
            b->velocity.z = BULLET_SPEED * cam->cos_yaw * cam->cos_pitch; // set z velocity
            return;
        }
    }
}

// Update bullet positions and deactivate if out of range
void update_bullets(GameState *game) {
    int i;
    double dx, dy, dz, distSq;
    Bullet *b;
    // Update each active bullet
    for (i = 0; i < MAX_BULLETS; i++) {
        b = &game->bullets[i];
        if (b->active) {
            b->position.x += b->velocity.x;
            b->position.y += b->velocity.y;
            b->position.z += b->velocity.z;
            
            /* Check distance from camera */
            dx = b->position.x - game->camera.position.x;
            dy = b->position.y - game->camera.position.y;
            dz = b->position.z - game->camera.position.z;
            distSq = dx*dx + dy*dy + dz*dz;
            
            if (distSq > RENDER_DISTANCE * RENDER_DISTANCE * 2) {
                b->active = 0;
            }
            
            /* Check if hit ground */
            if (b->position.y < get_terrain_height(game, b->position.x, b->position.z)) {
                b->active = 0;
            }
        }
    }
}



// Check for collisions between bullets, obstacles, and player
void check_collisions(GameState *game) {
    int i, j;
    double dx, dy, dz, distSq, hitDist;
    
    // Check bullet-obstacle collisions
    for (i = 0; i < MAX_BULLETS; i++) {
        if (!game->bullets[i].active) continue;
        
        // Check against all obstacles
        for (j = 0; j < MAX_OBSTACLES; j++) {
            if (!game->obstacles[j].active) continue;
            
            // Calculate squared distance
            dx = game->bullets[i].position.x - game->obstacles[j].position.x;
            dy = game->bullets[i].position.y - game->obstacles[j].position.y;
            dz = game->bullets[i].position.z - game->obstacles[j].position.z;
            distSq = dx*dx + dy*dy + dz*dz;
            
            hitDist = game->obstacles[j].size; // hit distance based on obstacle size
            
            if (distSq < hitDist * hitDist) {
                game->bullets[i].active = 0;
                game->obstacles[j].active = 0;
                game->score += 100;
                printf("\a");  /* Beep jingle for hit! */
                fflush(stdout);
                printf("HIT! Score: %d\n", game->score);
                
                if (game->score >= WIN_SCORE && !game->show_Win_Screen) {
                    printf("\n*** SCORE %d REACHED! Professor terrain unlocked! ***\n\n", WIN_SCORE);
                }
            }
        }
    }
    
    //Check player-obstacle collisions (lose a life)
    for (j = 0; j < MAX_OBSTACLES; j++) {
        if (!game->obstacles[j].active) continue;
        
        dx = game->camera.position.x - game->obstacles[j].position.x;
        dy = game->camera.position.y - game->obstacles[j].position.y;
        dz = game->camera.position.z - game->obstacles[j].position.z;
        distSq = dx*dx + dy*dy + dz*dz;
        
        hitDist = game->obstacles[j].size + 20;  /* Player collision radius */
        
        if (distSq < hitDist * hitDist) {
            game->obstacles[j].active = 0;  /* Destroy the obstacle */
            game->lives--;
            printf("\a");  /* Crash sound */
            printf("COLLISION! Lives remaining: %d\n", game->lives);
            
            if (game->lives <= 0) {
                game->game_over = 1;
                printf("\n*** OUT OF LIVES! ***\n");
            }
        }
    }
}



/* ==================== HUD ==================== */

// Draw crosshair at center of screen
void draw_crosshair(void) {
    gfx_color(255, 255, 0);  /* Yellow for visibility */
    gfx_line(SCREEN_CX - 15, SCREEN_CY, SCREEN_CX - 5, SCREEN_CY);
    gfx_line(SCREEN_CX + 5, SCREEN_CY, SCREEN_CX + 15, SCREEN_CY);
    gfx_line(SCREEN_CX, SCREEN_CY - 15, SCREEN_CX, SCREEN_CY - 5);
    gfx_line(SCREEN_CX, SCREEN_CY + 5, SCREEN_CX, SCREEN_CY + 15);
    gfx_color(255, 255, 255);
}

// Draw heads-up display (HUD)
void draw_hud(GameState *game) {
    int i, bar_len, x;
    char score_str[32];
    
    gfx_color(0, 255, 0);
    
    /* Score bar */
    bar_len = game->score / 5;
    if (bar_len > 200) bar_len = 200;
    gfx_line(10, 10, 10 + bar_len, 10);
    gfx_line(10, 11, 10 + bar_len, 11);
    gfx_line(10, 12, 10 + bar_len, 12);
    
    /* Score text next to bar */
    sprintf(score_str, "%d/%d", game->score, WIN_SCORE);
    gfx_text(220, 12, score_str);
    
    /* Timer display */
    {
        char time_str[32];
        int elapsed;
        if (game->show_Win_Screen) {
            elapsed = game->final_time;
        } else {
            elapsed = (int)(time(NULL) - game->start_time);
        }
        sprintf(time_str, "Time: %d:%02d", elapsed / 60, elapsed % 60);
        gfx_text(10, 25, time_str);
    }
    
    /* Win indicator */
    if (game->score >= WIN_SCORE) {
        gfx_color(255, 255, 0);
        gfx_line(10, 35, 100, 35);  /* Gold bar for winning */
        gfx_line(10, 36, 100, 36);
    }
    
    /* Lives display */
    gfx_color(255, 100, 100);
    gfx_text(SCREEN_WIDTH - 100, 12, "Lives:");
    for (i = 0; i < game->lives; i++) {
        /* Draw small hearts for lives */
        x = SCREEN_WIDTH - 45 + i * 15;
        gfx_circle(x, 15, 5);
    }
    
    gfx_color(255, 255, 255);
}
