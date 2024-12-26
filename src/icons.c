#include "icons.h"

#include "utils.h"

void draw_backwards_arrow(Rectangle rec, Color background, bool avail, bool mirror)
{
    Color col = avail ? BLUE : GRAY;
    int s = rec.width*.3;
    int x = rec.x + .5*rec.width;
    int y = rec.y + .6*rec.height;
    float mx = mirror ? -1 : 1; 

    // Semi circle
    DrawCircleLines(x, y, s, col);
    rec.width /= 2;
    if (mirror)
      rec.x += rec.width;
    DrawRectangleRec(rec, background);

    DrawLine(x - mx*.5*s, y - s, x, y - s - .5*s, col);
    DrawLine(x - mx*.5*s, y - s, x, y - s + .5*s, col);
    DrawLine(x - mx*s, y + s - .5, x, y + s - .5, col);
}

void draw_arrow(Rectangle rec, int dir)
{
    Color col = DARKBLUE;
    int s = rec.width*.35;
    int x = rec.x + .5*rec.width;
    int y = rec.y + .5*rec.height;

    if (dir == 0)
    {
        DrawLine(x - s, y, x + s, y, col);
        DrawLine(x, y - s, x + s, y, col);
        DrawLine(x, y + s, x + s, y, col);
    }
    if (dir == 1)
    {
        DrawLine(x - s, y, x + s, y, col);
        DrawLine(x, y - s, x - s, y, col);
        DrawLine(x, y + s, x - s, y, col);
    }
    if (dir == 2)
    {
        DrawLine(x, y - s, x, y + s, col);
        DrawLine(x - s, y, x, y - s, col);
        DrawLine(x + s, y, x, y - s, col);
    }
    if (dir == 3)
    {
        DrawLine(x, y - s, x, y + s, col);
        DrawLine(x - s, y, x, y + s, col);
        DrawLine(x + s, y, x, y + s, col);
    }
}

void draw_paint_bucket(Rectangle rec, bool enabled)
{
      if (enabled)
          DrawRectangleRec(rec, YELLOW);

      Rectangle rec2 = rec;
      DrawEllipseLines(rec2.x + .5*rec2.width, rec2.y + 0.3*rec2.height,
              .3*rec2.width, .2*rec2.height, DARKGRAY);

      rec2 = rec;
      rec2.y += 0.35 * rec2.height;
      rec2.x += 0.25 * rec2.width;
      rec2.width *= 0.5;
      rec2.height *= 0.6;
      DrawRectangleRec(rec2, DARKGRAY);
      rec2.height *= 0.2;
      rec2.width += 2;
      rec2.x -= 1;
      DrawRectangleRec(rec2, GRAY);
      rec2.y += rec2.height * 2;
      DrawRectangleRec(rec2, GRAY);
      rec2.y += rec2.height * 2;
      DrawRectangleRec(rec2, GRAY);
      rec2.width *= 0.5;
      rec2.x += rec2.width/2;
      rec2.y = rec.y + 0.5*rec2.height;
      DrawRectangleRec(rec2, GRAY);
}

void draw_gear(Rectangle rec, Color background, bool enabled)
{
    if (enabled)
        DrawRectangleRec(rec, YELLOW);
    rec = rect_grow(rec, -1);
    for (int t = 0; t < 6; ++t)
        DrawCircleSector((Vector2){rec.x + rec.width/2, rec.y + rec.height/2},
                rec.width/2, 60*t + 15, 60*t + 45, 6, DARKGRAY);
    DrawCircle(rec.x + rec.width/2, rec.y + rec.height/2, rec.width * 0.35, DARKGRAY);
    DrawCircle(rec.x + rec.width/2, rec.y + rec.height/2, rec.width * 0.15,
            enabled ? YELLOW : background);
}

void draw_grid(Rectangle rec, bool enabled)
{
    int gx = rec.x;
    int gy = rec.y;
    float scale = rec.width / 4;
    for (int x = 1; x <= 3; x++)
        DrawLine(gx + x*scale, gy + 1, gx + x*scale, gy + 4*scale - 1, DARKGRAY);
    for (int y = 1; y <= 3; y++)
        DrawLine(gx + 1,  gy + y*scale, gx + 4*scale - 1, gy + y*scale, DARKGRAY);
    if (!enabled)
        DrawLine(gx + 1,  gy + 4*scale - 1, gx + 4*scale - 1, gy + 1, RED);
}

void draw_save_icon(Rectangle rec)
{
    rec = rect_grow(rec, -2);
    DrawRectangleRec(rec, BLUE);
    rec.height /= 4;
    rec.width /= 2;
    rec.x += rec.width/2;
    DrawRectangleRec(rec, GRAY);
    rec.y += 2*rec.height;
    rec.height *= 2;
    DrawRectangleRec(rec, LIGHTGRAY);
}
