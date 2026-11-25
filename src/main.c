#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define MAX_ENEMIES   8

typedef struct {
    float x, y;
    float w, h;
    float speed;
} Player;

typedef struct {
    float x, y;
    float w, h;
    float vx, vy;
    bool active;
} Enemy;

typedef struct {
    float x, y;
    float size;
} Coin;

static bool rects_overlap(float ax, float ay, float aw, float ah,
                          float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

static float clampf(float val, float min_v, float max_v) {
    if (val < min_v) return min_v;
    if (val > max_v) return max_v;
    return val;
}

static float rand_range(float min_v, float max_v) {
    float t = (float)rand() / (float)RAND_MAX;
    return min_v + t * (max_v - min_v);
}

/* Tiny 3x5 bitmap font for digits 0-9 so we don't need external font files. */
static void draw_digit(SDL_Renderer *r, int digit, int x, int y, int scale) {
    static const unsigned short glyphs[10] = {
        0b111101101101111, /* 0 */
        0b010110010010111, /* 1 */
        0b111001111100111, /* 2 */
        0b111001111001111, /* 3 */
        0b101101111001001, /* 4 */
        0b111100111001111, /* 5 */
        0b111100111101111, /* 6 */
        0b111001001001001, /* 7 */
        0b111101111101111, /* 8 */
        0b111101111001111  /* 9 */
    };
    if (digit < 0 || digit > 9) return;
    unsigned short g = glyphs[digit];
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            int bit = 14 - (row * 3 + col);
            if ((g >> bit) & 1) {
                SDL_Rect px = {x + col * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

static void draw_number(SDL_Renderer *r, int value, int x, int y, int scale) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    for (int i = 0; buf[i] != '\0'; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            draw_digit(r, buf[i] - '0', x + i * 4 * scale, y, scale);
        }
    }
}

static void spawn_coin(Coin *coin) {
    coin->size = 20.0f;
    coin->x = rand_range(40.0f, WINDOW_WIDTH - 40.0f - coin->size);
    coin->y = rand_range(60.0f, WINDOW_HEIGHT - 40.0f - coin->size);
}

static void reset_enemy(Enemy *e, int index) {
    e->w = 28.0f;
    e->h = 28.0f;
    e->x = rand_range(20.0f, WINDOW_WIDTH - 20.0f - e->w);
    e->y = -e->h - (float)(index * 90);
    e->vx = rand_range(-90.0f, 90.0f);
    e->vy = rand_range(160.0f, 260.0f);
    e->active = true;
}

static void reset_game(Player *player, Coin *coin, Enemy enemies[], int *score, int *lives,
                       float *invuln_timer, bool *game_over) {
    player->w = 32.0f;
    player->h = 32.0f;
    player->x = (WINDOW_WIDTH - player->w) * 0.5f;
    player->y = WINDOW_HEIGHT - 80.0f;
    player->speed = 340.0f;

    spawn_coin(coin);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        reset_enemy(&enemies[i], i);
    }

    *score = 0;
    *lives = 3;
    *invuln_timer = 0.0f;
    *game_over = false;
}

int main(void) {
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "2D C Game - Coin & Dodge",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Player player;
    Coin coin;
    Enemy enemies[MAX_ENEMIES];
    int score = 0;
    int lives = 3;
    float invuln_timer = 0.0f;
    bool game_over = false;

    reset_game(&player, &coin, enemies, &score, &lives, &invuln_timer, &game_over);

    Uint64 prev_ticks = SDL_GetTicks64();
    bool running = true;

    while (running) {
        /* 1. Delta time (seconds since last frame) */
        Uint64 now_ticks = SDL_GetTicks64();
        float dt = (float)(now_ticks - prev_ticks) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f;
        prev_ticks = now_ticks;

        /* 2. Events */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_r && game_over) {
                    reset_game(&player, &coin, enemies, &score, &lives, &invuln_timer, &game_over);
                }
            }
        }

        /* 3. Update */
        if (!game_over) {
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            float move_x = 0.0f;
            float move_y = 0.0f;

            if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])  move_x -= 1.0f;
            if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) move_x += 1.0f;
            if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])    move_y -= 1.0f;
            if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S])  move_y += 1.0f;

            player.x += move_x * player.speed * dt;
            player.y += move_y * player.speed * dt;

            player.x = clampf(player.x, 0.0f, WINDOW_WIDTH - player.w);
            player.y = clampf(player.y, 0.0f, WINDOW_HEIGHT - player.h);

            if (invuln_timer > 0.0f) {
                invuln_timer -= dt;
            }

            /* Coin pickup */
            if (rects_overlap(player.x, player.y, player.w, player.h,
                              coin.x, coin.y, coin.size, coin.size)) {
                score += 10;
                spawn_coin(&coin);
            }

            /* Enemy movement & collision */
            for (int i = 0; i < MAX_ENEMIES; i++) {
                Enemy *e = &enemies[i];
                e->x += e->vx * dt;
                e->y += e->vy * dt;

                if (e->x < 0.0f || e->x + e->w > WINDOW_WIDTH) {
                    e->vx = -e->vx;
                    e->x = clampf(e->x, 0.0f, WINDOW_WIDTH - e->w);
                }

                if (e->y > WINDOW_HEIGHT) {
                    reset_enemy(e, 0);
                    e->vy += (float)(score / 20) * 8.0f;
                }

                if (invuln_timer <= 0.0f &&
                    rects_overlap(player.x, player.y, player.w, player.h,
                                  e->x, e->y, e->w, e->h)) {
                    lives--;
                    invuln_timer = 1.2f;
                    if (lives <= 0) {
                        game_over = true;
                    }
                }
            }
        }

        /* 4. Render */
        SDL_SetRenderDrawColor(renderer, 20, 24, 34, 255);
        SDL_RenderClear(renderer);

        /* Draw coin (gold) */
        SDL_SetRenderDrawColor(renderer, 250, 204, 21, 255);
        SDL_Rect coin_rect = {(int)coin.x, (int)coin.y, (int)coin.size, (int)coin.size};
        SDL_RenderFillRect(renderer, &coin_rect);

        /* Draw enemies (red) */
        SDL_SetRenderDrawColor(renderer, 239, 68, 68, 255);
        for (int i = 0; i < MAX_ENEMIES; i++) {
            SDL_Rect er = {(int)enemies[i].x, (int)enemies[i].y, (int)enemies[i].w, (int)enemies[i].h};
            SDL_RenderFillRect(renderer, &er);
        }

        /* Draw player (cyan, blinks while invulnerable) */
        if (invuln_timer <= 0.0f || ((int)(invuln_timer * 10.0f) % 2 == 0)) {
            SDL_SetRenderDrawColor(renderer, 56, 189, 248, 255);
            SDL_Rect pr = {(int)player.x, (int)player.y, (int)player.w, (int)player.h};
            SDL_RenderFillRect(renderer, &pr);
        }

        /* Draw HUD: score (top-left) and lives (top-right) */
        SDL_SetRenderDrawColor(renderer, 248, 250, 252, 255);
        draw_number(renderer, score, 16, 16, 4);

        for (int i = 0; i < lives; i++) {
            SDL_SetRenderDrawColor(renderer, 56, 189, 248, 255);
            SDL_Rect life_box = {WINDOW_WIDTH - 24 - i * 24, 16, 16, 16};
            SDL_RenderFillRect(renderer, &life_box);
        }

        /* Game over banner */
        if (game_over) {
            SDL_SetRenderDrawColor(renderer, 239, 68, 68, 255);
            SDL_Rect banner = {WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 - 40, 240, 80};
            SDL_RenderFillRect(renderer, &banner);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            draw_number(renderer, score, WINDOW_WIDTH / 2 - 24, WINDOW_HEIGHT / 2 - 15, 6);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
