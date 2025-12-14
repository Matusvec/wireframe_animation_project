Instructions to play the Game:

When the Game begins you msut instantly click the window to ensure that the controls will work. 

Controls:

YOU MUST HOLD THE SPACE BAR CONTINOUSLY FOR THE CONTROLS TO WORK 

move mouse up  - fly down
move mouse down - fly up
move mouse left - fly left
move mouse right - fly right
left mouse click - shoot bullet
+ button - increase speed
- button - decrease speed

You move around like a plane, you need to hold down the spacebar while mvoing your mouse so it registers as an event,
As in a normal plane, when you want to go up you control down, and vice versa, so here that applies as well, moving your mouse down will make the plane go up, moving mouse up will make the plane go down

This game is all played in first person, so its implied by the crosshair that you are seated in the plane and can fly around and observe the world

RULES:

- If you hit the ground, you will die and lose the game
- IF you hit a cube, you will lose one of your three lives, when you have no lives left, you die and lose the game
- Your goal to to amass 1000 points by shooting cubes worth 100 points each, when a cube is sucessfully hit, there will be a sound played to notify you that you hit the block, and it wil dispear form the screen
- If you win, then a special screen will show =>
- When you lose or win, you will be given the option to either hit R to RESTART, or Q to QUIT the game
- THere is a timer tracking how long it takes you to win, it will show you how long it took, try to get the shortest time possible






HOW IS THIS GAME CODED?


ARCHITECTURE

1. Data STRUCTURES
    Point3D      - 3D coordinate (x, y, z)
    Camera       - Player position, orientation (pitch/yaw), precomputed trig values
    Bullet       - Position, velocity, active flag
    Obstacle     - Position, size, rotation, active flag
    Heightmap    - Stores PPM image data (RGB arrays) for win screen
    GameState    - Master struct containing ALL game data (no globals!)

2. MAIN GAME LOOP

    1. Input: Mouse steering
     (from my code)

    mouse_x = gfx_xpos();
    mouse_y = gfx_ypos();
    target_yaw = (mouse_x - SCREEN_CX) * 0.0008;
    target_pitch = (mouse_y - SCREEN_CY) * 0.0006;
    game.camera.yaw += target_yaw * steer_speed;
    game.camera.pitch += target_pitch * steer_speed;

    So basically the offset from the screen center is the rotation amount, positions further from the center will lead to faster turns

    2. MOVEMENT 

    game.camera.position.x += speed * sin_yaw * cos_pitch;
    game.camera.position.z += speed * cos_yaw * cos_pitch;
    game.camera.position.y += speed * sin_pitch;

    Uses trigonometry to move in the direction the user is facing, sin/cos of yaw gives the horinzontal direction, sin of pitch gives vertical component

    3. Collision detection

    -Ground collision -  game over
    - Obstacle collision- lose a life (3 total)
    - bullet obstacle collision - +100 points

    4. RENDER PIPELINE 
    (every frame I do THESE STEPS)

    gfx_clear();        // clear the screen
    draw_sky();         // draw the Wireframe sky + sun
    draw_terrain();     //  draw the Green grid
    draw_obstacles();   //  draw Red cubes
    draw_bullets();     // draw Yellow crosses for bullets
    draw_crosshair();   // draw Yellow crosshair
    draw_hud();         // draw the Score, time, lives
    gfx_flush();        // Display to screen

    5. KEYBOARD INPUT 
        space = trigger event for mouse to work
        +/- speed control up/down
        Q = quit
        R = Restart one win/lose screens


3. 3D PROJECTION SYSTEM (HOW DID I MAKE THIS GAME IN 3D!)
    This is the math of how this all works, converting world coordinates to screen pixels
    the main one is  : void project_point(Point3D p, Camera *cam, int *sx, int *sy)

    Step 1: translate to camera space
        (from my code)

        dx = p.x - cam->position.x;
        dy = p.y - cam->position.y;
        dz = p.z - cam->position.z;

        Make the camera the origin (0,0,0)

    Step 2 : Rotate by the Yaw (left/right look)

        rx = dx * cos_yaw - dz * sin_yaw;
        rz = dx * sin_yaw + dz * cos_yaw;

        2d rotation matrix around the y axis

    Step 3: Rotate by the pitch (up/down look)

        ry = ty * cos_pitch - tz * sin_pitch;
        rz = ty * sin_pitch + tz * cos_pitch;

        Now 2d rotation matrix around the x axis

    Step 4 : Perspective division

    This is becacause I chose to do perspective projection instead of orthogonal projection, basically it means that the further away objects get the smaller they look, so I divide by a scaling factor
    (from my code relveant to this)

    if (rz < 20.0) { *sx = -9999; return; }  // Behind camera!
    scale = 300.0 / rz;  // Closer = bigger
    *sx = rx * scale + 400;  // Center on screen
    *sy = -ry * scale + 300; // Flip Y (screen Y is down)

    Objects that are further away from the viewer get smaller, bc of larger rz, however, if there are objects behind the camera they are marked invalid


4. Procedural Terrain (HOW DID I MAKE THE TERRAIN AND THE GRID CONTINOUSLY?)
    The key here is that NO TERRAIN DATA IS STORED!
    the heights are all calculated mathematically (From my code)

    double get_terrain_height(GameState *game, double x, double z) {
    return 30.0 * sin(x * 0.01) * sin(z * 0.01) +
           15.0 * sin(x * 0.03 + z * 0.02);
    }
    I use combining sine curves to generate this hilly terrain, all including small, medium, and large hills, because flat is boring!
    
    Draw the grid terrain:
         1. Calculate the grid center based on the camera position
         2. Loop through a 30 by 30 grid around the player
         3. Skip the points beyond render distance (this is an optimization)
         4. Draw wireframe lines between adjacent points to actually draw the grid

    (Here is some pseudocode)

    for (i = -GRID_SIZE; i < GRID_SIZE; i++) {
    for (j = -GRID_SIZE; j < GRID_SIZE; j++) {
        // Calculate world position
        // Get height from sine formula
        // Project to screen
        // Draw line to next point
        }
    }

5. Wireframe Cube Drawing 
    Each obstacle is a rotating cube with 12 edges and 8 points
    Here is function : void draw_wireframe_cube(Point3D center, double size, double rot, Camera *cam)

    1. I define 8 corners relative to the center  (+- half,,+- half,+- half +-)
    2.Rotate the corners around the y axis, by rot angle
    3. Project the 8 corners to screen coordinates
    4. Draw 12 edges connecting corners, and ONLY if the endpoints are visible

    FROM MY CODE:
    // Corner rotation
    rx = lx * cos(rot) - lz * sin(rot);
    rz = lx * sin(rot) + lz * cos(rot);
    corners[i].x = center.x + rx;

6. Collision detection
    I use distance -squared checks, so I avoid the slow sqrt() from math.H

    here is my code relevant (PSEUDOCODE)

    dx = bullet.x - obstacle.x;
    dy = bullet.y - obstacle.y;
    dz = bullet.z - obstacle.z;
    distSq = dx*dx + dy*dy + dz*dz;

    if (distSq < hitRadius * hitRadius) {
        // HIT!
    }

    Player obstacle collision has a larger radius (size+ 20) for fairness


7. BULLET FIRING MECHANISM

    Firing PSEUDOCODE:
        bullet.position = camera.position;  // Start at player
        bullet.velocity.x = BULLET_SPEED * sin_yaw * cos_pitch;
        bullet.velocity.y = BULLET_SPEED * sin_pitch;
        bullet.velocity.z = BULLET_SPEED * cos_yaw * cos_pitch;

    This means that the bullet has the same direction math as the player movement

    Updating:
        bullet.position.x += bullet.velocity.x;  // Move each frame
        // Deactivate if too far or hit ground




8. PPM IMAGE DRAWING, LOADING, SCALING FULL EXPLANATION

    PPM (Portable Pixel Map) is a simple image format, and my code uses p6 (Binary format)
    So the reason that I want the images as PPms is its easier to extract all the pixels and color values

    For each image in PPM format . . . 

    p6 is the "magic" number it just means that the image is in binary RGB
    # is an optional comment so we skip all THESE
    512 512 is the width and height repsectively
    255 is the max color value
    also binary RBG data will just be in width times height times 3 bytes of data, since each pixel needs R, G ,and B values



    NOW FOR THE functions
    
        The loading function
            it opens the file in binary mode, "rb", stand for read binary
            it reads the magic number to make sure its binary PPM format
            Then it skips all the cmoments starting with #
            Read the dimesnions 
            Read the binary pixel data into a 2d array struct, I do it in row major order

        For scaling, which I need to do because the images are all different resolutions and sizes, so I need to find a way to increase and decrease sizes
        void draw_ppm_scaled(const char *filename, int destX, int destY, int destW, int destH)
        I use 1d arrays here for the pixel drawing, one array for each color (RGB)

        TO SHRINK THE IMAGE 
            DRAW EVERY NTH Pixel

        TO INCREASE THE IMAGE Size
            DRAW EVERY PIXEL N TIMES
















