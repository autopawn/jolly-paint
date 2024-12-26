#include "icons.h"

void draw_backwards_arrow_button(Rectangle rec, Color background, bool avail, bool mirror)
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
