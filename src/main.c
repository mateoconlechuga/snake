#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <keypadc.h>

#define BACKGROUND_COLOR 0
#define SNOWFLAKE_COLOR 128

#define ONE_SECOND         32768/1
#define HALF_SECOND        32768/2
#define QUARTER_SECOND     32768/4
#define EIGHTH_SECOND      32768/8
#define SIXTEENTH_SECOND      32768/16
#define THIRTYSECOND_SECOND      32768/32
#define SIXTYFOUTH_SECOND      32768/128

typedef struct {
    int x, y;
    int w, h;
} box_t;
static box_t box = { 0, 0, LCD_WIDTH - 2, LCD_HEIGHT };

extern uint16_t palette[256];

typedef enum {
    DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} direction_t;

typedef struct {
    int x, y;
    int tx, ty;
    direction_t dir;
    int h, v;
    int add;
    uint8_t color;
    int8_t u;
} snake_t;
static snake_t snake;

static void draw_snowflake(void);
static void draw_snake_head(void);
static void draw_snake_tail(void);
static void draw_snake(void);

void main(void) {
    srand(rtc_Time());
    snake.color = rand() & 255;

    snake.tx = snake.x = box.w - 2;
    snake.ty = snake.y = box.h - 2;
    snake.v = -1;
    snake.add = 10;
    snake.u = randInt(0, 2) == 1 ? 1 : -1;

    gfx_Begin();
    gfx_FillScreen(BACKGROUND_COLOR);
    gfx_SetPalette(palette, sizeof palette, 0);

    timer_Control = TIMER1_DISABLE;
    timer_1_ReloadValue = timer_1_Counter = THIRTYSECOND_SECOND;
    timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_0INT | TIMER1_DOWN;

    draw_snake_head();
    draw_snowflake();

    do {
        kb_key_t key;

        kb_Scan();
        key = kb_Data[7];

        if (key) {
            switch (key) {
                case kb_Down:
                    snake.dir = DIR_DOWN;
                    break;
                case kb_Right:
                    snake.dir = DIR_RIGHT;
                    break;
                case kb_Up:
                    snake.dir = DIR_UP;
                    break;
                case kb_Left:
                    snake.dir = DIR_LEFT;
                    break;
                default:
                    break;
            }
        }

        if (timer_IntStatus & TIMER1_RELOADED) {
            if (snake.h) {
                snake.v = (snake.dir == DIR_DOWN ? 1 : 0) -
                          (snake.dir == DIR_UP ? 1 : 0);
                if (snake.v) {
                    snake.h = 0;
                }
            } else {
                snake.h = (snake.dir == DIR_RIGHT ? 1 : 0) -
                          (snake.dir == DIR_LEFT ? 1 : 0);
                if (snake.h) {
                    snake.v = 0;
                }
            }
            snake.x += 3u * snake.h;
            snake.y += 3u * snake.v;
            gfx_SetColor(BACKGROUND_COLOR);
            if (snake.x > box.w) {
                gfx_SetPixel(snake.x - 2, snake.y);
                snake.x = 1;
            }
            if (snake.x < 1) {
                gfx_SetPixel(snake.x + 2, snake.y);
                snake.x = box.w - 2;
            }
            if (snake.y > box.h) {
                gfx_SetPixel(snake.x, snake.y - 2);
                snake.y = 1;
            }
            if (snake.y < 1) {
                gfx_SetPixel(snake.x, snake.y + 2);
                snake.y = box.h - 2;
            }
            draw_snake();
            timer_IntAcknowledge = TIMER1_RELOADED;
        }

    } while (kb_Data[1] != kb_2nd);

    gfx_End();
}

static void draw_snowflake(void) {
    int x, y;
    x = 1 + 3 * randInt(0, box.w / 3);
    y = 1 + 3 * randInt(0, box.h / 3);
    gfx_SetColor(SNOWFLAKE_COLOR);
    gfx_SetPixel(x + 0, y + 0);
    gfx_SetPixel(x + 1, y + 0);
    gfx_SetPixel(x + 0, y + 1);
    gfx_SetPixel(x - 1, y - 0);
    gfx_SetPixel(x + 0, y - 1);
}

static void set_snake_color(int x, int y) {
    gfx_SetColor(snake.color);
    gfx_SetPixel(x, y);
}
static void set_background_color(int x, int y) {
    gfx_SetColor(BACKGROUND_COLOR);
    gfx_SetPixel(x, y);
}

static void draw_snake_tail(void) {
    gfx_SetColor(BACKGROUND_COLOR);
    gfx_Rectangle(snake.tx - 1, snake.ty - 1, 3, 3);
}

static void draw_snake_head(void) {
    gfx_SetColor(snake.color);
    gfx_Rectangle(snake.x - 1, snake.y - 1, 3, 3);
}

static void draw_snake(void) {
    int r;
    snake.color += snake.u;
    if (snake.color == 0) {
        snake.color += snake.u;
    }
    if (snake.color == 128) {
        snake.color += snake.u;
    }
    if (randInt(0, 100) == 50) {
        snake.u = -snake.u;
    }
    r = gfx_GetPixel(snake.x + 1, snake.y);
    if (r != SNOWFLAKE_COLOR && r != BACKGROUND_COLOR) {
        gfx_End();
        exit(1);
    }
    draw_snake_head();
    set_background_color(snake.x - (snake.h * 1), snake.y - (snake.v * 1));
    set_background_color(snake.x - (snake.h * 2), snake.y - (snake.v * 2));
    r = gfx_GetPixel(snake.x, snake.y);
    if (r) {
        if (r == SNOWFLAKE_COLOR) {
            snake.add += 2;
            set_background_color(snake.x, snake.y);
            draw_snowflake();
        } else {
            gfx_End();
            exit(1);
        }
    }
    if (snake.add) {
        snake.add--;
    } else {
        int x, y;
        y = (gfx_GetPixel(snake.tx, snake.ty - 1) != BACKGROUND_COLOR ? 1 : 0) -
            (gfx_GetPixel(snake.tx, snake.ty + 1) != BACKGROUND_COLOR ? 1 : 0);
        x = (gfx_GetPixel(snake.tx - 1, snake.ty) != BACKGROUND_COLOR ? 1 : 0) -
            (gfx_GetPixel(snake.tx + 1, snake.ty) != BACKGROUND_COLOR ? 1 : 0);
        if (y) {
            set_snake_color(snake.tx, snake.ty + y * 2);
            draw_snake_tail();
            snake.ty += 3u * y;
            if (snake.ty > box.h) {
                snake.ty = 1;
                set_background_color(snake.tx, box.h - 2);
                set_snake_color(snake.tx, snake.ty - 1);
            }
            if (snake.ty < 1) {
                snake.ty = box.h - 2;
                set_background_color(snake.tx, 1);
                set_snake_color(snake.tx, snake.ty + 1);
            }
        } else {
            set_snake_color(snake.tx + x * 2, snake.ty);
            draw_snake_tail();
            snake.tx += 3u * x;
            if (snake.tx > box.w) {
                snake.tx = 1;
                set_background_color(box.w, snake.ty);
                set_snake_color(snake.tx - 1, snake.ty);
            }
            if (snake.tx < 1) {
                snake.tx = box.w - 2;
                set_background_color(1, snake.ty);
                set_snake_color(snake.tx + 1, snake.ty);
            }
        }
    }
}

