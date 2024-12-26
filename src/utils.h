#include <raylib.h>

static inline void rectangle_scale(Rectangle *rect, int offset_x, int offset_y, int scale)
{
    rect->x *= scale;
    rect->y *= scale;
    rect->width *= scale;
    rect->height *= scale;
    rect->x += offset_x;
    rect->y += offset_y;
}

static inline Rectangle rect_grow(Rectangle rect, int growth)
{
    rect.x -= growth;
    rect.y -= growth;
    rect.width += 2*growth;
    rect.height += 2*growth;
    return rect;
}

