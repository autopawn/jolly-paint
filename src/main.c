#include <raylib.h>

#include "palettes.h"

#define CANVAS_SIZE 32

struct layout
{
    bool vertical;
    int scale;
    Rectangle canvas;
    Rectangle palette;
    Rectangle current[2];
    Rectangle buttons[2];
};

static void rectangle_scale(Rectangle *rect, int offset_x, int offset_y, int scale)
{
    rect->x *= scale;
    rect->y *= scale;
    rect->width *= scale;
    rect->height *= scale;
    rect->x += offset_x;
    rect->y += offset_y;
}

static Rectangle rect_grow(Rectangle rect, int growth)
{
    rect.x -= growth;
    rect.y -= growth;
    rect.width += 2*growth;
    rect.height += 2*growth;
    return rect;
}

static struct layout compute_layout(bool vertical)
{
    struct layout lay = {0};
    lay.vertical = vertical;

    int size_w = GetScreenWidth();
    int size_h = GetScreenHeight();

    int required_w = 1 + 32*2 + 1 + (vertical ? 0 : 2*2 + 1);
    int required_h = 1 + 32*2 + 1 + (vertical ? 2*2 + 1 : 0);

    int scale_w = size_w / required_w;
    int scale_h = size_h / required_h;
    int scale = (scale_w < scale_h) ? scale_w : scale_h;
    lay.scale = scale;

    int offset_x = (size_w - scale * required_w)/2;
    int offset_y = (size_h - scale * required_h)/2;

    lay.canvas.x = 1;
    lay.canvas.y = 1;
    lay.canvas.width = 32*2;
    lay.canvas.height = 32*2;

    if (vertical)
    {
        for (int t = 0; t < 2; ++t)
        {
            lay.current[t].x = 1 + (4 + 1)*t;
            lay.current[t].y = 1 + 32*2 + 1;
            lay.current[t].width = 4;
            lay.current[t].height = 4;
        }

        lay.palette.x = 1 + 4 + 1 + 4 + 1;
        lay.palette.y = 1 + 32*2 + 1;
        lay.palette.width = 16*2;
        lay.palette.height = 2*2;
    }
    else
    {
        for (int t = 0; t < 2; ++t)
        {
            lay.current[t].x = 1 + 32*2 + 1;
            lay.current[t].y = 1 + (4 + 1)*t;
            lay.current[t].width = 4;
            lay.current[t].height = 4;
        }

        lay.palette.x = 1 + 32*2 + 1;
        lay.palette.y = 1 + 4 + 1 + 4 + 1;
        lay.palette.width = 2*2;
        lay.palette.height = 16*2;
    }

    rectangle_scale(&lay.canvas, offset_x, offset_y, scale);
    rectangle_scale(&lay.current[0], offset_x, offset_y, scale);
    rectangle_scale(&lay.current[1], offset_x, offset_y, scale);
    rectangle_scale(&lay.palette, offset_x, offset_y, scale);

    return lay;
}

struct state
{
    int cells[CANVAS_SIZE][CANVAS_SIZE];
    int pal; // Current palette
    int col1, col2;
};

static Color get_color(const struct state *st, int idx)
{
    return GetColor(palettes[st->pal].colors[idx]);
}

int main(void)
{
    // Initialization
    InitWindow(400, 400, "Jolly paint");
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);

    SetTargetFPS(60);

    struct state st = {.col1 = 3, .col2 = 8};

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        struct layout layout_v = compute_layout(true);
        struct layout layout_h = compute_layout(false);
        struct layout layout = (layout_v.scale >= layout_h.scale) ? layout_v : layout_h;
        Vector2 mpos = GetMousePosition();

        // Update selected colors
        for (int c = 0; c < 16; ++c)
        {
            Rectangle r = layout.palette;
            if (layout.vertical)
            {
                r.width /= 16;
                r.x += r.width * c;
            }
            else
            {
                r.height /= 16;
                r.y += r.height * c;
            }

            if (CheckCollisionPointRec(mpos, r))
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    st.col1 = c;
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                    st.col2 = c;
            }
        }

        if (CheckCollisionPointRec(mpos, layout.canvas))
        {
            int pos_x = (mpos.x - layout.canvas.x)/layout.canvas.width * 32;
            int pos_y = (mpos.y - layout.canvas.y)/layout.canvas.height * 32;

            if (pos_x < 0) pos_x = 0;
            if (pos_x >= 32) pos_x = 31;
            if (pos_y < 0) pos_y = 0;
            if (pos_y >= 32) pos_y = 31;

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                st.cells[pos_y][pos_x] = st.col1;
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
                st.cells[pos_y][pos_x] = st.col2;
        }

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);
            
            DrawRectangleLinesEx(rect_grow(layout.canvas, 1), 1, DARKGRAY);
            DrawRectangleLinesEx(rect_grow(layout.palette, 1), 1, DARKGRAY);

            
            DrawRectangleLinesEx(rect_grow(layout.current[0], 1), 1, DARKGRAY);
            DrawRectangleRec(rect_grow(layout.current[0], -1), get_color(&st, st.col1));
            
            DrawRectangleLinesEx(rect_grow(layout.current[1], 1), 1, DARKGRAY);
            DrawRectangleRec(rect_grow(layout.current[1], -1), get_color(&st, st.col2));

            for (int y = 0; y < CANVAS_SIZE; ++y)
            {
                for (int x = 0; x < CANVAS_SIZE; ++x)
                {
                    Rectangle r;
                    r.x = layout.canvas.x + 2 * layout.scale * x;
                    r.y = layout.canvas.y + 2 * layout.scale * y;
                    r.width = 2*layout.scale;
                    r.height = 2*layout.scale;

                    int col = st.cells[y][x];

                    DrawRectangleRec(r, get_color(&st, col));
                }
            }

            // Draw palette
            for (int c = 0; c < 16; ++c)
            {
                Rectangle r = layout.palette;
                if (layout.vertical)
                {
                    r.width /= 16;
                    r.x += r.width * c;
                }
                else
                {
                    r.height /= 16;
                    r.y += r.height * c;
                }
                DrawRectangleRec(r, get_color(&st, c));
            }

            // Draw grid
            for (int x = 0; x < CANVAS_SIZE; ++x)
            {
                int px = layout.canvas.x + 2*layout.scale*x + 1;
                DrawLine(px, layout.canvas.y, px, layout.canvas.y + layout.canvas.height, GRAY);
            }
            for (int y = 0; y < CANVAS_SIZE; ++y)
            {
                int py = layout.canvas.y + 2*layout.scale*y;
                DrawLine(layout.canvas.x, py, layout.canvas.x + layout.canvas.width, py, GRAY);
            }

        EndDrawing();
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
