#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "gfx.h"
#include <time.h>
#include <math.h>

int main(){
    int screenWidth = 800;
    int screenHeight = 600;
    int mode = 1;  // 1=Y-axis, 2=X-axis, 3=Z-axis, 4=Diagonal, 5=All axes
    double angleX = 0.0;
    double angleY = 0.0;
    double angleZ = 0.0;
     // for a cube centered at the origin with size 100
    typedef struct {
        double x,y,z;
    } Point3D;
    Point3D cubeVertices[8] = {
        {-50, -50, -50},  //A
        {50, -50, -50},  //B
        {50, 50, -50}, //C
        {-50, 50, -50}, //D
        {-50, -50, 50}, //E
        {50, -50, 50}, //F
        {50, 50, 50}, //G
        {-50, 50, 50} //H
    };
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Back face
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Front face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Side edges
    };
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    double distance = 600.0;  // Camera distance for perspective

    Point3D rotatedVertices[8]; //stores all rotated vertices
    int screenX[8]; //stores all projected x coordinates
    int screenY[8]; //stores all projected y coordinates

    gfx_open(screenWidth, screenHeight, "3D Rotating Cube Animation");

    while(1){
        gfx_clear();

        //step 1 : Rotate all vertices based on mode
        for(int i=0; i < 8; i++){
            double x = cubeVertices[i].x;
            double y = cubeVertices[i].y;
            double z = cubeVertices[i].z;
            
            if(mode == 1) {  // Y-axis (vertical spin)
                rotatedVertices[i].x = x * cos(angleY) - z * sin(angleY);
                rotatedVertices[i].z = x * sin(angleY) + z * cos(angleY);
                rotatedVertices[i].y = y;
            }
            else if(mode == 2) {  // X-axis (forward/back tumble)
                rotatedVertices[i].y = y * cos(angleX) - z * sin(angleX);
                rotatedVertices[i].z = y * sin(angleX) + z * cos(angleX);
                rotatedVertices[i].x = x;
            }
            else if(mode == 3) {  // Z-axis (twist)
                rotatedVertices[i].x = x * cos(angleZ) - y * sin(angleZ);
                rotatedVertices[i].y = x * sin(angleZ) + y * cos(angleZ);
                rotatedVertices[i].z = z;
            }
            else if(mode == 4) {  // Diagonal tumble (Y then X)
                // First rotate around Y
                double tempX = x * cos(angleY) - z * sin(angleY);
                double tempZ = x * sin(angleY) + z * cos(angleY);
                // Then rotate around X
                rotatedVertices[i].x = tempX;
                rotatedVertices[i].y = y * cos(angleX) - tempZ * sin(angleX);
                rotatedVertices[i].z = y * sin(angleX) + tempZ * cos(angleX);
            }
            else if(mode == 5) {  // All three axes
                // Y rotation
                double rx = x * cos(angleY) - z * sin(angleY);
                double rz = x * sin(angleY) + z * cos(angleY);
                // X rotation
                double ry = y * cos(angleX) - rz * sin(angleX);
                rz = y * sin(angleX) + rz * cos(angleX);
                // Z rotation
                rotatedVertices[i].x = rx * cos(angleZ) - ry * sin(angleZ);
                rotatedVertices[i].y = rx * sin(angleZ) + ry * cos(angleZ);
                rotatedVertices[i].z = rz;
            }
        }
        //step 2 : Project all rotated vertices to 2D screen coordinates with PERSPECTIVE
        for(int i=0; i < 8; i++){
            // Perspective projection: scale by distance/(distance - z)
            // Objects farther away (negative z) appear smaller
            double scale = distance / (distance - rotatedVertices[i].z);
            screenX[i] = (int)(rotatedVertices[i].x * scale) + centerX;
            screenY[i] = (int)(rotatedVertices[i].y * scale) + centerY;
        }

        //Step 3 : Draw edges between projected vertices
        for(int i=0; i < 12; i++){
            int startIdx = edges[i][0];
            int endIdx = edges[i][1];
            gfx_line(screenX[startIdx], screenY[startIdx], screenX[endIdx], screenY[endIdx]);
        }

        gfx_flush();
        angleX += 0.03;
        angleY += 0.03;
        angleZ += 0.03;
        usleep(16666); // Pause for 30 milliseconds

        if(gfx_event_waiting()){
            char c = gfx_wait();
            if(c == 'q'){
                break;
            }
            if(c == '1') mode = 1;  // Y-axis rotation
            if(c == '2') mode = 2;  // X-axis rotation
            if(c == '3') mode = 3;  // Z-axis rotation
            if(c == '4') mode = 4;  // Diagonal tumble
            if(c == '5') mode = 5;  // All axes
        }
    }

    return 0;
}
