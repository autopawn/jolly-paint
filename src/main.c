#include "raylib.h"

int main(void)
{
    // Initialization
    InitWindow(400, 400, "Jolly paint");
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);

    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

        EndDrawing();
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
