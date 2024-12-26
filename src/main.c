#include <raylib.h>
#include <emscripten.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "palettes.h"
#include "utils.h"
#include "icons.h"

#define MAX_CANVAS_SIZE 32
#define BGCOLOR RAYWHITE
#define MAX_UNDOS 32

#define BUTTON_OPTIONS   0
#define BUTTON_GRID      1
#define BUTTON_UNDO      2
#define BUTTON_REDO      3
#define BUTTON_BUCKET    4
#define BUTTON_LEFT      5
#define BUTTON_RIGHT     6
#define BUTTON_UP        7
#define BUTTON_DOWN      8
#define BUTTON_SAVE      9
#define BUTTON_SAVE_BIG 10
#define BUTTON_COUNT    11

#define ARRAY_SIZE(X) (sizeof((X))/sizeof((X)[0]))

static const int SIZE_OPTIONS[] = {16, 21, 24, 32};

struct layout
{
    bool vertical;
    int scale;
    int pixel_size;
    Rectangle canvas;
    Rectangle board;
    Rectangle palette;
    Rectangle current;
    Rectangle buttons[BUTTON_COUNT];

    Rectangle size_buttons[ARRAY_SIZE(SIZE_OPTIONS)];
    Rectangle palette_buttons[ARRAY_SIZE(PALETTES)];
    Rectangle ok_button;
};

static struct layout compute_layout_oriented(int size, bool vertical)
{
    struct layout lay = {0};
    lay.vertical = vertical;

    int size_w = GetScreenWidth();
    int size_h = GetScreenHeight();

    int required_w = 1 + 64 + 1 + (vertical ? 0 : 4 + 1 + 4 + 1);
    int required_h = 1 + 64 + 1 + (vertical ? 4 + 1 + 4 + 1 : 0);

    int scale_w = size_w / required_w;
    int scale_h = size_h / required_h;
    int scale = (scale_w < scale_h) ? scale_w : scale_h;
    lay.scale = scale;

    int offset_x = (size_w - scale * required_w)/2;
    int offset_y = (size_h - scale * required_h)/2;

    lay.canvas.x = 1;
    lay.canvas.y = 1;
    lay.canvas.width = 64;
    lay.canvas.height = 64;

    if (vertical)
    {
        lay.current.x = 2;
        lay.current.y = 1 + 64 + 1;
        lay.current.width = 4;
        lay.current.height = 4;

        lay.palette.x = 2 + 4 + 3;
        lay.palette.y = 1 + 64 + 1;
        lay.palette.width = 16*3;
        lay.palette.height = 2*2;

        lay.buttons[0].x = required_w - 2 - 4;
        lay.buttons[0].y = 1 + 64 + 1;
        lay.buttons[0].width = 4;
        lay.buttons[0].height = 4;

        for (int t = 1; t < BUTTON_COUNT; ++t)
        {
            lay.buttons[t].x = 2 + (4 + 1)*(t - 1);
            lay.buttons[t].y = 1 + 64 + 1 + 4 + 1;
            lay.buttons[t].width = 4;
            lay.buttons[t].height = 4;
        }
    }
    else
    {
        lay.current.x = 1 + 64 + 1;
        lay.current.y = 2;
        lay.current.width = 4;
        lay.current.height = 4;

        lay.palette.x = 1 + 64 + 1;
        lay.palette.y = 2 + 4 + 3;
        lay.palette.width = 2*2;
        lay.palette.height = 16*3;

        lay.buttons[0].x = 1 + 64 + 1;
        lay.buttons[0].y = required_h - 2 - 4;
        lay.buttons[0].width = 4;
        lay.buttons[0].height = 4;

        for (int t = 1; t < BUTTON_COUNT; ++t)
        {
            lay.buttons[t].x = 1 + 64 + 1 + 4 + 1;
            lay.buttons[t].y = 2 + (4 + 1)*(t - 1);
            lay.buttons[t].width = 4;
            lay.buttons[t].height = 4;
        }
    }

    for (int i = 0; i < ARRAY_SIZE(SIZE_OPTIONS); ++i)
    {
        lay.size_buttons[i].x = 3 + (14 + 1)*i;
        lay.size_buttons[i].y = 4;
        lay.size_buttons[i].width = 14;
        lay.size_buttons[i].height = 4;
    }
    for (int i = 0; i < ARRAY_SIZE(PALETTES); ++i)
    {
        lay.palette_buttons[i].x = 2;
        lay.palette_buttons[i].y = 11 + (4 + 1)*i;
        lay.palette_buttons[i].width = 60;
        lay.palette_buttons[i].height = 4;
    }
    lay.ok_button.x = 16;
    lay.ok_button.y = 11 + (4 + 1)*(int)ARRAY_SIZE(PALETTES);
    lay.ok_button.width = 32;
    lay.ok_button.height = 4;

    rectangle_scale(&lay.canvas, offset_x, offset_y, scale);
    rectangle_scale(&lay.current, offset_x, offset_y, scale);
    rectangle_scale(&lay.palette, offset_x, offset_y, scale);
    for (int t = 0; t < BUTTON_COUNT; ++t)
        rectangle_scale(&lay.buttons[t], offset_x, offset_y, scale);
    for (int t = 0; t < ARRAY_SIZE(SIZE_OPTIONS); ++t)
        rectangle_scale(&lay.size_buttons[t], offset_x, offset_y, scale);
    for (int t = 0; t < ARRAY_SIZE(PALETTES); ++t)
        rectangle_scale(&lay.palette_buttons[t], offset_x, offset_y, scale);
    rectangle_scale(&lay.ok_button, offset_x, offset_y, scale);

    lay.board = lay.canvas;

    lay.pixel_size = (int)(lay.canvas.width/size);
    lay.canvas.x += (int)(.5*(lay.canvas.width - lay.pixel_size * size));
    lay.canvas.y += (int)(.5*(lay.canvas.height - lay.pixel_size * size));
    lay.canvas.height = lay.pixel_size * size;
    lay.canvas.width = lay.pixel_size * size;

    return lay;
}

static struct layout compute_layout(int size)
{
    struct layout layout_v = compute_layout_oriented(size, true);
    struct layout layout_h = compute_layout_oriented(size, false);
    return (layout_v.scale >= layout_h.scale) ? layout_v : layout_h;
}

struct matrix
{
    unsigned char cells[MAX_CANVAS_SIZE][MAX_CANVAS_SIZE];
};

struct state
{
    struct matrix mat;
    int size;
    int pal; // Current palette
    int col1, col2;
    bool grid;
};

static Color get_color(const struct state *st, int idx)
{
    return GetColor(PALETTES[st->pal].colors[idx]);
}

static bool state_load(struct state *st)
{
    int size = 0;
    unsigned char *data = LoadFileData("/offline/state.data", &size);
    if (!data)
        return false;
    if (size < sizeof(struct state))
    {
        UnloadFileData(data);
        return false;
    }
    memcpy(st, data, sizeof(struct state));
    UnloadFileData(data);
    return true;
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

static void state_shift_left(struct state *st)
{
    for (int y = 0; y < st->size; ++y)
    {
        unsigned char aux = st->mat.cells[y][0];
        for (int x = 0; x < st->size - 1; ++x)
            st->mat.cells[y][x] = st->mat.cells[y][x + 1];
        st->mat.cells[y][st->size - 1] = aux;
    }
}

static void state_shift_right(struct state *st)
{
    for (int y = 0; y < st->size; ++y)
    {
        unsigned char aux = st->mat.cells[y][st->size - 1];
        for (int x = st->size - 1; x >= 1; --x)
            st->mat.cells[y][x] = st->mat.cells[y][x - 1];
        st->mat.cells[y][0] = aux;
    }
}

static void state_shift_up(struct state *st)
{
    for (int x = 0; x < st->size; ++x)
    {
        unsigned char aux = st->mat.cells[0][x];
        for (int y = 0; y < st->size - 1; ++y)
            st->mat.cells[y][x] = st->mat.cells[y + 1][x];
        st->mat.cells[st->size - 1][x] = aux;
    }
}

static void state_shift_down(struct state *st)
{
    for (int x = 0; x < st->size; ++x)
    {
        unsigned char aux = st->mat.cells[st->size - 1][x];
        for (int y = st->size - 1; y >= 1; --y)
            st->mat.cells[y][x] = st->mat.cells[y - 1][x];
        st->mat.cells[0][x] = aux;
    }
}

static void image_save(const struct state *st, bool big)
{
    int scale = big ? 16 : 1;
    
    Image img = GenImageColor(st->size*scale, st->size*scale, WHITE);
    for (int y = 0; y < st->size; ++y)
    {
        for (int x = 0; x < st->size; ++x)
            ImageDrawRectangle(&img, scale*x, scale*y, scale, scale, get_color(st, st->mat.cells[y][x]));
    }

    ExportImage(img, "img.png");
    UnloadImage(img);
    if (big)
        emscripten_run_script("saveFileFromMemoryFSToDisk('img.png','jolly_paint_img_big.png')");
    else
        emscripten_run_script("saveFileFromMemoryFSToDisk('img.png','jolly_paint_img.png')");
}

struct undostack
{
    struct matrix mats[MAX_UNDOS];
    // States saved to undo (the top one is the current one).
    int len;
    // Last valid length for redos.
    int redo_len;
};

void undostack_save(const struct state *st, struct undostack *stack)
{
    // Check that currrent state is different to last saved state
    if (stack->len > 0)
    {
        bool same = true;
        for (int y = 0; y < MAX_CANVAS_SIZE; ++y)
        {
            for (int x = 0; x < MAX_CANVAS_SIZE; ++x)
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
    stack->redo_len = stack->len;
}

bool undostack_can_undo(const struct undostack *stack)
{
    return stack->len >= 2;
}

void undostack_undo(struct state *st, struct undostack *stack)
{
    if (!undostack_can_undo(stack))
        return;
    stack->len -= 1;
    st->mat = stack->mats[stack->len - 1];
}

bool undostack_can_redo(const struct undostack *stack)
{
    return stack->len < stack->redo_len;
}

void undostack_redo(struct state *st, struct undostack *stack)
{
    if (!undostack_can_redo(stack))
        return;
    st->mat = stack->mats[stack->len];
    stack->len += 1;
}

void flood_fill(struct state *st, int x, int y, int a, int b)
{
    if (x < 0 || y < 0 || x >= st->size || y >= st->size)
        return;
    if (st->mat.cells[y][x] == b || st->mat.cells[y][x] != a)
        return;
    st->mat.cells[y][x] = b;
    flood_fill(st, x + 1, y, a, b);
    flood_fill(st, x - 1, y, a, b);
    flood_fill(st, x, y + 1, a, b);
    flood_fill(st, x, y - 1, a, b);
}

void draw_text_centered(const struct layout *layout, Rectangle rect, const char *text, int size)
{
    int font_size = size*layout->scale;
    int w = MeasureText(text, font_size);
    DrawText(text, rect.x + (rect.width - w)/2, rect.y + (rect.height - font_size)/2, font_size, DARKGRAY);
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

    bool options = false;
    bool bucket = false;

    struct state st = {.col1 = 8, .col2 = 3, .size = 24};
    bool loaded = state_load(&st);
    if (!loaded)
        options = true;
    struct undostack stack = {0};
    undostack_save(&st, &stack);


    bool left_on_canvas = false;
    bool right_on_canvas = false;

    // Main game loop
    unsigned int frame = 0;
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        struct layout layout = compute_layout(st.size);
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

        if (options)
        {
            for (int i = 0; i < ARRAY_SIZE(SIZE_OPTIONS); ++i)
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                        && CheckCollisionPointRec(mpos, layout.size_buttons[i]))
                {
                    st.size = SIZE_OPTIONS[i];
                    layout = compute_layout(st.size);
                }
            }
            for (int i = 0; i < ARRAY_SIZE(PALETTES); ++i)
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                        && CheckCollisionPointRec(mpos, layout.palette_buttons[i]))
                    st.pal = i;
            }
            // Ok button
            if (CheckCollisionPointRec(mpos, layout.ok_button) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                options = false;
        }
        else if (!CheckCollisionPointRec(mpos, layout.canvas))
        {
            left_on_canvas = false;
            right_on_canvas = false;
        }
        else
        {
            Vector2 mdelta = GetMouseDelta();
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                mdelta = (Vector2){0, 0}; // Preempt large deltas on the phone.

            for (int t = 0; t <= 20; t++)
            {
                float alpha = t/20.0f;
                Vector2 midpos;
                midpos.x = mpos.x - mdelta.x*(1-alpha);
                midpos.y = mpos.y - mdelta.y*(1-alpha);
                if (CheckCollisionPointRec(midpos, layout.canvas))
                {
                    int pos_x = (midpos.x - layout.canvas.x)/layout.pixel_size;
                    int pos_y = (midpos.y - layout.canvas.y)/layout.pixel_size;

                    if (pos_x < 0) pos_x = 0;
                    if (pos_x >= st.size) pos_x = st.size - 1;
                    if (pos_y < 0) pos_y = 0;
                    if (pos_y >= st.size) pos_y = st.size - 1;

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        left_on_canvas = true;
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                        right_on_canvas = true;

                    if (bucket)
                    {
                        int current = st.mat.cells[pos_y][pos_x];
                        if (left_on_canvas && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                            flood_fill(&st, pos_x, pos_y, current, st.col1);
                        if (right_on_canvas && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
                            flood_fill(&st, pos_x, pos_y, current, st.col2);
                    }
                    else
                    {
                        if (left_on_canvas && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                            st.mat.cells[pos_y][pos_x] = st.col1;
                        if (right_on_canvas && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
                            st.mat.cells[pos_y][pos_x] = st.col2;
                    }
                }
            }
        }
        // Save undo checkpoint
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            left_on_canvas = false;
            undostack_save(&st, &stack);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
        {
            right_on_canvas = false;
            undostack_save(&st, &stack);
        }

        // Swap colors
        if (IsKeyPressed(KEY_X) ||
                (CheckCollisionPointRec(mpos, layout.current) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            int aux = st.col1;
            st.col1 = st.col2;
            st.col2 = aux;
        }

        // Options toggle
        if (IsKeyPressed(KEY_O) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_OPTIONS]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
            options = !options;
        // Grid toggle
        if (IsKeyPressed(KEY_G) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_GRID]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
            st.grid = !st.grid;
        // Undo
        if (IsKeyPressed(KEY_Z) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_UNDO]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
            undostack_undo(&st, &stack);
        if (IsKeyPressed(KEY_Y) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_REDO]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
            undostack_redo(&st, &stack);
        // Paint bucket toggle
        if (IsKeyPressed(KEY_P) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_BUCKET]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
            bucket = !bucket;
        // Shift buttons
        if (IsKeyPressed(KEY_LEFT) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_LEFT]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            state_shift_left(&st);
            undostack_save(&st, &stack);
        }
        if (IsKeyPressed(KEY_RIGHT) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_RIGHT]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            state_shift_right(&st);
            undostack_save(&st, &stack);
        }
        if (IsKeyPressed(KEY_UP) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_UP]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            state_shift_up(&st);
            undostack_save(&st, &stack);
        }
        if (IsKeyPressed(KEY_DOWN) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_DOWN]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            state_shift_down(&st);
            undostack_save(&st, &stack);
        }

        // Save image
        bool shift_down = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if ((!shift_down && IsKeyPressed(KEY_S)) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_SAVE]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            image_save(&st, false);
            state_save(&st);
        }
        // Save image (big)
        if ((shift_down && IsKeyPressed(KEY_S)) ||
                (CheckCollisionPointRec(mpos, layout.buttons[BUTTON_SAVE_BIG]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            image_save(&st, true);
            state_save(&st);
        }

        // Draw
        BeginDrawing();
        {

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

            // Draw canvas
            for (int y = 0; y < st.size; ++y)
            {
                for (int x = 0; x < st.size; ++x)
                {
                    Rectangle r;
                    r.x = layout.canvas.x + layout.pixel_size * x;
                    r.y = layout.canvas.y + layout.pixel_size * y;
                    r.width = layout.pixel_size;
                    r.height = layout.pixel_size;

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
                for (int x = 0; x < st.size; ++x)
                {
                    int px = layout.canvas.x + x*layout.pixel_size + 1;
                    DrawLine(px, layout.canvas.y, px, layout.canvas.y + layout.canvas.height, GRAY);
                }
                for (int y = 0; y < st.size; ++y)
                {
                    int py = layout.canvas.y + y*layout.pixel_size;
                    DrawLine(layout.canvas.x, py, layout.canvas.x + layout.canvas.width, py, GRAY);
                }
            }

            // Draw options
            if (options)
            {
                DrawRectangleRec(rect_grow(layout.board, 1), Fade(RAYWHITE, 0.95));

                for (int i = 0; i < ARRAY_SIZE(SIZE_OPTIONS); ++i)
                {
                    Rectangle rec = layout.size_buttons[i];
                    DrawRectangleRec(rec, st.size == SIZE_OPTIONS[i] ? YELLOW : BGCOLOR);
                    DrawRectangleLinesEx(rect_grow(rec, 1), 1, DARKGRAY);

                    char buffer[20];
                    sprintf(buffer, "%ux%u", SIZE_OPTIONS[i], SIZE_OPTIONS[i]);
                    draw_text_centered(&layout, rec, buffer, 4);
                }

                for (int i = 0; i < ARRAY_SIZE(PALETTES); ++i)
                {
                    Rectangle rec = layout.palette_buttons[i];
                    DrawRectangleRec(rec, st.pal == i ? YELLOW : BGCOLOR);
                    DrawText(PALETTES[i].name, rec.x + 1, rec.y + 1, 2*layout.scale, DARKGRAY);
                    DrawRectangleLinesEx(rect_grow(rec, 1), 1, DARKGRAY);

                    for (int c = 0; c < 16; ++c)
                    {
                        DrawRectangle(
                            rec.x + 28*layout.scale + c*2*layout.scale, rec.y,
                            2*layout.scale, rec.height, GetColor(PALETTES[i].colors[c]));
                    }
                }

                {
                    Rectangle rec = layout.ok_button;
                    DrawRectangleRec(rec, BGCOLOR);
                    draw_text_centered(&layout, rec, "OK", 4);
                    DrawRectangleLinesEx(rect_grow(rec, 1), 1, DARKGRAY);
                }
            }

            // Draw buttons
            for (int t = 0; t < BUTTON_COUNT; ++t)
                DrawRectangleLinesEx(rect_grow(layout.buttons[t], 1), 1, DARKGRAY);

            draw_gear(layout.buttons[BUTTON_OPTIONS], BGCOLOR, options);
            draw_grid(layout.buttons[BUTTON_GRID], st.grid);
            draw_backwards_arrow(layout.buttons[BUTTON_UNDO], BGCOLOR,
                    undostack_can_undo(&stack), false);
            draw_backwards_arrow(layout.buttons[BUTTON_REDO], BGCOLOR,
                    undostack_can_redo(&stack), true);
            draw_paint_bucket(layout.buttons[BUTTON_BUCKET], bucket);

            draw_arrow(layout.buttons[BUTTON_RIGHT], 0);
            draw_arrow(layout.buttons[BUTTON_LEFT], 1);
            draw_arrow(layout.buttons[BUTTON_UP], 2);
            draw_arrow(layout.buttons[BUTTON_DOWN], 3);

            draw_save_icon(layout.buttons[BUTTON_SAVE]);

            draw_save_icon(layout.buttons[BUTTON_SAVE_BIG]);
            Rectangle rec = layout.buttons[BUTTON_SAVE_BIG];
            rec.height /= 2;
            rec.y += rec.height;
            draw_text_centered(&layout, rec, "x16", 2);

        }
        EndDrawing();

        frame += 1;
        if (frame % 60 == 0)
            state_save(&st);
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
