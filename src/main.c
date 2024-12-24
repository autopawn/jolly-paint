#include <raylib.h>
#include <emscripten.h>
#include <stdbool.h>
#include <string.h>

#include "palettes.h"

#define CANVAS_SIZE 32
#define BUTTON_COUNT 5
#define BGCOLOR RAYWHITE
#define MAX_UNDOS 32

struct layout
{
    bool vertical;
    int scale;
    Rectangle canvas;
    Rectangle palette;
    Rectangle current;
    Rectangle buttons[BUTTON_COUNT];
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
        lay.current.x = 2;
        lay.current.y = 1 + 32*2 + 1;
        lay.current.width = 4;
        lay.current.height = 4;

        lay.palette.x = 2 + 4 + 1;
        lay.palette.y = 1 + 32*2 + 1;
        lay.palette.width = 16*2;
        lay.palette.height = 2*2;
        
        for (int t = 0; t < BUTTON_COUNT; ++t)
        {
            lay.buttons[t].x = lay.palette.x + lay.palette.width + 1 + (4 + 1)*t;
            lay.buttons[t].y = 1 + 32*2 + 1;
            lay.buttons[t].width = 4;
            lay.buttons[t].height = 4;
        }
    }
    else
    {
        lay.current.x = 1 + 32*2 + 1;
        lay.current.y = 2;
        lay.current.width = 4;
        lay.current.height = 4;

        lay.palette.x = 1 + 32*2 + 1;
        lay.palette.y = 2 + 4 + 1;
        lay.palette.width = 2*2;
        lay.palette.height = 16*2;

        for (int t = 0; t < BUTTON_COUNT; ++t)
        {
            lay.buttons[t].x = 1 + 32*2 + 1; 
            lay.buttons[t].y = lay.palette.y + lay.palette.height + 1 + (4 + 1)*t;
            lay.buttons[t].width = 4;
            lay.buttons[t].height = 4;
        }
    }

    rectangle_scale(&lay.canvas, offset_x, offset_y, scale);
    rectangle_scale(&lay.current, offset_x, offset_y, scale);
    rectangle_scale(&lay.palette, offset_x, offset_y, scale);
    for (int t = 0; t < BUTTON_COUNT; ++t)
        rectangle_scale(&lay.buttons[t], offset_x, offset_y, scale);

    return lay;
}

struct matrix
{
    unsigned char cells[CANVAS_SIZE][CANVAS_SIZE];
};

struct state
{
    struct matrix mat;
    int pal; // Current palette
    int col1, col2;
    bool bucket;
    bool grid;
};

static Color get_color(const struct state *st, int idx)
{
    return GetColor(palettes[st->pal].colors[idx]);
}

static void state_load(struct state *st)
{
    int size = 0;
    unsigned char *data = LoadFileData("/offline/state.data", &size);
    if (!data)
        return;
    if (sizeof(struct state) <= size)
        memcpy(st, data, sizeof(struct state));
    UnloadFileData(data);
}

static void state_save(struct state *st)
{
    bool lock = true;
    SaveFileData("/offline/state.data", st, sizeof(struct state));

    EM_ASM({
        FS.syncfs(function (err) {
            Module.setValue($0, false, "i8"); // lock -> false
        });
    }, &lock);

    while (lock)
        emscripten_sleep(1);
}

static void image_save(const struct state *st)
{
    Image img = GenImageColor(32*8, 32*8, WHITE);
    for (int y = 0; y < CANVAS_SIZE; ++y)
    {
        for (int x = 0; x < CANVAS_SIZE; ++x)
            ImageDrawRectangle(&img, 8*x, 8*y, 8, 8, get_color(st, st->mat.cells[y][x]));
    }

    ExportImage(img, "img.png");
    UnloadImage(img);
    emscripten_run_script("saveFileFromMemoryFSToDisk('img.png','image.png')");
}

struct undostack
{
    struct matrix mats[MAX_UNDOS];
    int len;
};

void undostack_save(const struct state *st, struct undostack *stack)
{
    // Check that currrent state is different to last saved state
    if (stack->len > 0)
    {
        bool same = true;
        for (int y = 0; y < CANVAS_SIZE; ++y)
        {
            for (int x = 0; x < CANVAS_SIZE; ++x)
            {
                if (stack->mats[stack->len - 1].cells[y][x] != st->mat.cells[y][x])
                    same = false;
            }
        }
        if (same)
            return;
    }
    // Push stack down
    if (stack->len == MAX_UNDOS)
    {
        for (int i = 0; i < stack->len - 1; ++i)
            stack->mats[i] = stack->mats[i + 1];
        
        stack->len -= 1;
    }
    // Store state in the stack
    stack->mats[stack->len] = st->mat;
    stack->len += 1;
}

void undostack_undo(struct state *st, struct undostack *stack)
{
    if (stack->len < 2)
        return;
    stack->len -= 1;
    st->mat = stack->mats[stack->len - 1];
}

void flood_fill(struct state *st, int x, int y, int a, int b)
{
    if (x < 0 || y < 0 || x >= CANVAS_SIZE || y >= CANVAS_SIZE)
        return;
    if (st->mat.cells[y][x] == b || st->mat.cells[y][x] != a)
        return;
    st->mat.cells[y][x] = b;
    flood_fill(st, x + 1, y, a, b);
    flood_fill(st, x - 1, y, a, b);
    flood_fill(st, x, y + 1, a, b);
    flood_fill(st, x, y - 1, a, b);
}

int main(void)
{
    bool lock = true;
    EM_ASM({
        // Make a directory mounted as IndexedDB
        if (!FS.analyzePath('/offline').exists){
            FS.mkdir('/offline');
        }
        FS.mount(IDBFS, {}, '/offline');
        FS.syncfs(true, function (err) {
            Module.setValue($0, false, "i8"); // lock -> false
        });
    }, &lock);

    while (lock)
        emscripten_sleep(1);

    // Initialization
    InitWindow(400, 400, "Jolly paint");
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    SetTargetFPS(60);

    struct state st = {.col1 = 3, .col2 = 8};
    state_load(&st);
    struct undostack stack = {0};
    undostack_save(&st, &stack);

    // Main game loop
    unsigned int frame = 0;
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

        Vector2 mdelta = GetMouseDelta();
        for (int t = 0; t <= 20; t++)
        {
            float alpha = t/20.0f;
            Vector2 midpos;
            midpos.x = mpos.x - mdelta.x*(1-alpha);
            midpos.y = mpos.y - mdelta.y*(1-alpha);
            if (CheckCollisionPointRec(midpos, layout.canvas))
            {
                int pos_x = (midpos.x - layout.canvas.x)/layout.canvas.width * 32;
                int pos_y = (midpos.y - layout.canvas.y)/layout.canvas.height * 32;

                if (pos_x < 0) pos_x = 0;
                if (pos_x >= 32) pos_x = 31;
                if (pos_y < 0) pos_y = 0;
                if (pos_y >= 32) pos_y = 31;

                if (st.bucket)
                {
                    int current = st.mat.cells[pos_y][pos_x];
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                        flood_fill(&st, pos_x, pos_y, current, st.col1);
                    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
                        flood_fill(&st, pos_x, pos_y, current, st.col2);
                }
                else
                {
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                        st.mat.cells[pos_y][pos_x] = st.col1;
                    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
                        st.mat.cells[pos_y][pos_x] = st.col2;
                }
            }
        }
        // Save undo checkpoint
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
            undostack_save(&st, &stack);

        // Paint bucket toggle
        if (CheckCollisionPointRec(mpos, layout.buttons[0]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            st.bucket = !st.bucket;
        // Grid toggle
        if (CheckCollisionPointRec(mpos, layout.buttons[1]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            st.grid = !st.grid;
        // Undo
        if (CheckCollisionPointRec(mpos, layout.buttons[2]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            undostack_undo(&st, &stack);

        // Save image
        if (CheckCollisionPointRec(mpos, layout.buttons[4]))
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                image_save(&st);
                state_save(&st);
            }
        }

        // Draw
        BeginDrawing();

            ClearBackground(BGCOLOR);
            
            DrawRectangleLinesEx(rect_grow(layout.canvas, 1), 1, DARKGRAY);

            DrawRectangleLinesEx(rect_grow(layout.palette, 1), 1, DARKGRAY);

            { // Draw current colors
                Rectangle rec1 = {
                        layout.current.x,
                        layout.current.y,
                        layout.current.width * 0.75,
                        layout.current.height * 0.75,
                };
                Rectangle rec2 = {
                        layout.current.x + layout.current.width * 0.25,
                        layout.current.y + layout.current.height * 0.25,
                        layout.current.width * 0.75,
                        layout.current.height * 0.75,
                };
                DrawRectangleLinesEx(rect_grow(rec2, 1), 1, DARKGRAY);
                DrawRectangleRec(rect_grow(rec2, -1), get_color(&st, st.col2));
                DrawRectangleRec(rec1, BGCOLOR);
                DrawRectangleLinesEx(rect_grow(rec1, 1), 1, DARKGRAY);
                DrawRectangleRec(rect_grow(rec1, -1), get_color(&st, st.col1));
            }

            for (int y = 0; y < CANVAS_SIZE; ++y)
            {
                for (int x = 0; x < CANVAS_SIZE; ++x)
                {
                    Rectangle r;
                    r.x = layout.canvas.x + 2 * layout.scale * x;
                    r.y = layout.canvas.y + 2 * layout.scale * y;
                    r.width = 2*layout.scale;
                    r.height = 2*layout.scale;

                    int col = st.mat.cells[y][x];

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
            if (st.grid)
            {
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
            }

            // Draw buttons
            int scale = layout.scale;
            for (int t = 0; t < BUTTON_COUNT; ++t)
                DrawRectangleLinesEx(rect_grow(layout.buttons[t], 1), 1, DARKGRAY);

            // Button 0 (paint bucket)
            {
                Rectangle rec = layout.buttons[0];

                if (st.bucket)
                    DrawRectangleRec(rec, YELLOW);

                DrawEllipseLines(rec.x + .5*rec.width, rec.y + 0.3*rec.height,
                        .3*rec.width, .2*rec.height, DARKGRAY);

                rec = layout.buttons[0];
                rec.y += 0.35 * rec.height;
                rec.x += 0.25 * rec.width;
                rec.width *= 0.5;
                rec.height *= 0.6;
                DrawRectangleRec(rec, DARKGRAY);
                rec.height *= 0.2;
                rec.width += 2;
                rec.x -= 1;
                DrawRectangleRec(rec, GRAY);
                rec.y += rec.height * 2;
                DrawRectangleRec(rec, GRAY);
                rec.y += rec.height * 2;
                DrawRectangleRec(rec, GRAY);
                rec.width *= 0.5;
                rec.x += rec.width/2;
                rec.y = layout.buttons[0].y + 0.5*rec.height;
                DrawRectangleRec(rec, GRAY);
            }

            // Button 1 (grid toggle)
            int gx = layout.buttons[1].x;
            int gy = layout.buttons[1].y;
            for (int x = 1; x <= 3; x++)
                DrawLine(gx + x*scale, gy + 1, gx + x*scale, gy + 4*scale - 1, DARKGRAY);
            for (int y = 1; y <= 3; y++)
                DrawLine(gx + 1,  gy + y*scale, gx + 4*scale - 1, gy + y*scale, DARKGRAY);
            if (!st.grid)
                DrawLine(gx + 1,  gy + 4*scale - 1, gx + 4*scale - 1, gy + 1, RED);

            // Button 4 (save icon)
            Rectangle rec = rect_grow(layout.buttons[4], -2);
            DrawRectangleRec(rec, BLUE);
            rec.height /= 4;
            rec.width /= 2;
            rec.x += rec.width/2;
            DrawRectangleRec(rec, GRAY);
            rec.y += 2*rec.height;
            rec.height *= 2;
            DrawRectangleRec(rec, GRAY);

        EndDrawing();

        frame += 1;
        if (frame % 60 == 0)
            state_save(&st);
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
