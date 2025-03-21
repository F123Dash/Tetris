#include <SDL2/SDL_error.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define ARENA_WIDTH_PX 400U
#define ARENA_HEIGHT_PX 800U
#define SCREEN_WIDTH_PX 1200U
#define SCREEN_HEIGHT_PX 800U
#define ARENA_PADDING_PX 400U
#define ARENA_WIDTH 8U
#define ARENA_HEIGHT 18U
#define ARENA_SIZE 144U
#define PIECE_WIDTH 4U
#define PIECE_HEIGHT 4U
#define PIECE_SIZE 16U
#define TETROMINOS_DATA_SIZE 16U
#define BLOCK_SIZE_PX 50U
#define TETROMINOS_COUNT 7U
#define PIECE_COLOR_SIZE 4U
#define ARENA_PADDING_TOP 2U
#define FONT "./fonts/NotoSansMono-Regular.ttf"

#define END(check, str1, str2) \
    if (check) { \
        assert(check); \
        fprintf(stderr, "%s\n%s", str1, str2); \
        exit(1); \
    } \

enum {PIECE_I, PIECE_J, PIECE_L, PIECE_O, PIECE_S, PIECE_T, PIECE_Z,
      PIECE_COUNT};

enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_BLACK, COLOR_SIZE};

enum {COLLIDE_NONE = 0, COLLIDE_LEFT = 1 << 0, COLLIDE_RIGHT = 1 << 1, 
      COLLIDE_TOP = 1 << 2, COLLIDE_BOTTOM = 1 << 3, COLLIDE_PIECE = 1 << 4};

enum {UPDATE_MAIN, UPDATE_LOSE, UPDATE_PAUSE};

typedef struct _Size {
    int w;
    int h;
    uint8_t start_x;
    uint8_t start_y;
} Size;

typedef struct _Game {
    uint8_t level;
    uint64_t score;
    SDL_Renderer *renderer;
    SDL_Window *window;
    TTF_Font *lose_font;
    TTF_Font *ui_font;
    uint8_t placed[ARENA_SIZE]; // 8 x 18 */
} Game;

typedef uint8_t (*Update_callback)(Game *game, uint64_t frame, SDL_KeyCode key,
                                   bool keydown);

void 
drawText(SDL_Renderer *renderer, TTF_Font *font, const char *text,
         SDL_Point point)
{
    if (text == NULL || strlen(text) == 0) {
        fprintf(stderr, "Error: Text is empty or null\n");
        return;
    }

    int w = 0;
    int h = 0;
    int padding = 8;

    TTF_SizeText(font, text, &w, &h);

    SDL_Color font_color = {.r = 255, .g = 255, .b = 255, .a = 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, font_color);

    END(surface == NULL, "Could not create text surface\n", SDL_GetError());

    SDL_Rect rect = {
        .x = point.x - (w / 2),
        .y = point.y - (h / 2),
        .w = w,
        .h = h,
    };

    SDL_Rect text_background = {
        .x = rect.x - padding,
        .y = rect.y - padding,
        .w = rect.w + padding * 2,
        .h = rect.h + padding * 2
    };

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    END(texture == NULL, "Could not create texture", SDL_GetError());

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &text_background);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &text_background);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void Game_Quit(Game *game); // Forward declaration of Game_Quit

static uint8_t
updatePause(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown)
{
    SDL_Point point = {.x = SCREEN_WIDTH_PX / 2, .y = SCREEN_HEIGHT_PX / 2 - 100};
    SDL_Point resume_point = {.x = SCREEN_WIDTH_PX / 2, .y = SCREEN_HEIGHT_PX / 2 - 50};
    SDL_Point exit_point = {.x = SCREEN_WIDTH_PX / 2, .y = SCREEN_HEIGHT_PX / 2};

    // Render pause menu background
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 200); // Semi-transparent black
    SDL_Rect pause_background = {
        .x = SCREEN_WIDTH_PX / 4,
        .y = SCREEN_HEIGHT_PX / 4,
        .w = SCREEN_WIDTH_PX / 2,
        .h = SCREEN_HEIGHT_PX / 2
    };
    SDL_RenderFillRect(game->renderer, &pause_background);

    // Render pause menu text
    drawText(game->renderer, game->ui_font, "Paused", point);
    drawText(game->renderer, game->ui_font, "Press R to Resume", resume_point);
    drawText(game->renderer, game->ui_font, "Press E to Exit", exit_point);

    if (keydown) {
        switch (key) {
            case SDLK_r: // Resume
                return UPDATE_MAIN;

            case SDLK_e: // Exit
                Game_Quit(game);
                exit(0);
        }
    }

    return UPDATE_PAUSE;
}

int
findPoints(uint8_t level, uint8_t lines)
{
    switch (lines) {
        case 1: return 40 * (level + 1);
        case 2: return 100 * (level + 1);
        case 3: return 300 * (level + 1);
        case 4: return 1200 * (level + 1);
    }

    return 0;
}

void
addToArena(uint8_t *placed, uint8_t i)
{
    // bounds checking
    if (i < ARENA_SIZE && i >= 0) placed[i] = 1;
}

uint8_t
getPlacedPosition(SDL_Point pos)
{
    uint8_t i = pos.y * ARENA_WIDTH + pos.x;
    return i < ARENA_SIZE ? i : ARENA_SIZE - 1;
}

void
setColor(SDL_Renderer *renderer, uint8_t color)
{
    const SDL_Color colors[] = {
        [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
        [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
        [COLOR_BLUE] = {.r = 146, .g = 161, .b = 185, .a = 255},
        [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
        [COLOR_GREY] = {.r = 89, .g = 89, .b = 89, .a = 89},
        [COLOR_BLACK] = {.r = 0, .g = 0, .b = 0, .a = 0},
    };

    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g,
                           colors[color].b, colors[color].a);
}

void
getXY(uint8_t i, int *x, int *y) {
    *x = i % PIECE_WIDTH;
    *y = floor((float)i / PIECE_HEIGHT);
}

void
getPieceSize(uint8_t *piece, Size *size)
{
    memset(size, 0, sizeof(Size));

    bool foundStartx = false;
    bool foundStarty = false;

    for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
        for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
            uint8_t i = y * PIECE_WIDTH + x;

            if (piece[i]) {
                if (!foundStartx) {
                    size->start_x = x;
                    foundStartx = true;
                }

                size->w++;
                break;
            }
        }
    }

    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            uint8_t i = y * PIECE_WIDTH + x;

            if (piece[i]) {
                if (!foundStarty) {
                    size->start_y = y;
                    foundStarty = true;
                }

                size->h++;
                break;
            }
        }
    }
}

void
rotatePiece(uint8_t *piece, uint8_t *rotated)
{
    memset(rotated, 0, sizeof(uint8_t) * PIECE_SIZE);

    uint8_t i = 0;

    // 90 degrees
    for (int x = 0; x < PIECE_HEIGHT; ++x) {
        for (int y = PIECE_WIDTH - 1; y >= 0; --y) {
            uint8_t j = y * PIECE_WIDTH + x;

            rotated[i] = piece[j];
            ++i;
        }
    }
}

void
drawTetromino(SDL_Renderer *renderer, uint8_t piece[PIECE_SIZE],
                     SDL_Point position, uint8_t color)
{
    for (int i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        if (!piece[i]) continue;

        int x, y; getXY(i, &x, &y);

        SDL_Rect rect = {
            .x = (int)((x + position.x) * (int)BLOCK_SIZE_PX) + (int)ARENA_PADDING_PX,
            .y = (int)(y + position.y - ARENA_PADDING_TOP) * (int)BLOCK_SIZE_PX,
            .w = BLOCK_SIZE_PX, .h = BLOCK_SIZE_PX
        };

        setColor(renderer, color);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void
printPlaced(uint8_t *placed)
{
    uint8_t i = 0;

    for (int y = 0; y < ARENA_HEIGHT; ++y) {
        printf("%.3d: ", y * ARENA_WIDTH);

        for (int x = 0; x < ARENA_WIDTH; ++x) {
            i = (y * ARENA_WIDTH) + x;

            printf("%d", placed[i]);
        }

        printf("\n");
    }
}

void
clearRow(uint8_t *placed, uint8_t c)
{
    uint8_t temp[ARENA_SIZE];

    memset(temp, 0, sizeof(uint8_t) * ARENA_SIZE);
    memcpy(temp, placed, sizeof(uint8_t) * ARENA_SIZE);

    for (int y = c; y > 0; --y) {
        for (uint8_t x = 0; x < ARENA_WIDTH; ++x) {
            uint8_t i = y * ARENA_WIDTH + x;

            if (i >= 0) temp[i] = placed[i - ARENA_WIDTH];
        }
    }

    memcpy(placed, temp, sizeof(uint8_t) * ARENA_SIZE);
}

uint8_t
checkForRowClearing(uint8_t *placed)
{
    uint8_t row_count = 0;
    uint8_t lines = 0;

    for (uint8_t y = 0; y < ARENA_HEIGHT; ++y) {
        for (uint8_t x = 0; x < ARENA_WIDTH; ++x) {
            if (x == 0) row_count = 0;

            uint8_t i = y * ARENA_WIDTH + x;

            if (placed[i]) row_count++;

            if ((x == ARENA_WIDTH - 1) && (row_count == ARENA_WIDTH)) {
                clearRow(placed, y);
                lines++;
            }

        }
    }

    return lines;
}

void
addToPlaced(uint8_t *placed, uint8_t *piece, SDL_Point position)
{
    uint8_t pos = getPlacedPosition(position);

    for (uint8_t i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        int x, y; getXY(i, &x, &y);
        uint8_t piece_i = y * PIECE_WIDTH + x;
        uint8_t placed_i = y * ARENA_WIDTH + x;

        if (piece[piece_i]) addToArena(placed, pos + placed_i);
    }
}

uint8_t
collisionCheck(uint8_t *placed, uint8_t *piece, SDL_Point position)
{
    Size size; getPieceSize(piece, &size);
    uint8_t placed_pos = getPlacedPosition(position);
    uint8_t collide = COLLIDE_NONE;

    if (position.x < -size.start_x) collide |= COLLIDE_LEFT;

    if (position.x + size.start_x + size.w > ARENA_WIDTH)
        collide |= COLLIDE_RIGHT;

    if (position.y + size.start_y + size.h > ARENA_HEIGHT)
        collide |= COLLIDE_BOTTOM;

    for (uint8_t y = 0; y < PIECE_HEIGHT; ++y) {
        for (uint8_t x = 0; x < PIECE_WIDTH; ++x) {
            uint8_t piece_i = y * PIECE_WIDTH + x;
            uint8_t placed_i = placed_pos + (y * ARENA_WIDTH + x);
            if (piece[piece_i] && placed[placed_i]) {
                collide |= COLLIDE_PIECE;
                return collide;
            }
        }
    }

    return collide;
}

void
pickPiece(uint8_t *piece, uint8_t *color)
{
    const uint8_t tetrominos[TETROMINOS_COUNT]
                                   [PIECE_SIZE] = {
        // each piece has 4 ones
        [PIECE_I] = {0,0,0,0,
                     1,1,1,1,
                     0,0,0,0,
                     0,0,0,0},

        [PIECE_J] = {0,0,0,0,
                     1,0,0,0,
                     1,1,1,0,
                     0,0,0,0},

        [PIECE_L] = {0,0,0,0,
                     0,0,1,0,
                     1,1,1,0,
                     0,0,0,0},

        [PIECE_O] = {0,0,0,0,
                     0,1,1,0,
                     0,1,1,0,
                     0,0,0,0},

        [PIECE_S] = {0,0,0,0,
                     0,1,1,0,
                     1,1,0,0,
                     0,0,0,0},

        [PIECE_T] = {0,0,0,0,
                     0,1,0,0,
                     1,1,1,0,
                     0,0,0,0},

        [PIECE_Z] = {0,0,0,0,
                     1,1,0,0,
                     0,1,1,0,
                     0,0,0,0},
    };

    const uint8_t piece_colors[PIECE_COLOR_SIZE] = {COLOR_RED, COLOR_GREEN,
                                                    COLOR_BLUE, COLOR_ORANGE};

    uint8_t id = (float)((float)rand() / (float)RAND_MAX) * PIECE_COUNT;

    memcpy(piece, &tetrominos[id], sizeof(uint8_t) * PIECE_SIZE);

    *color = piece_colors[((*color) + 1) % PIECE_COLOR_SIZE];
}

void
Game_Init(Game *game)
{
    memset(game, 0, sizeof(Game));

    END(SDL_Init(SDL_INIT_VIDEO) != 0, "Could not create texture",
        SDL_GetError());

    END(TTF_Init() != 0, "Could not initialize TTF", TTF_GetError());

    game->lose_font = TTF_OpenFont(FONT, 50);

    END(game->lose_font == NULL, "Could not open font", TTF_GetError());

    game->ui_font = TTF_OpenFont(FONT, 30);
    if (game->ui_font == NULL) {
        fprintf(stderr, "Error: Could not open font: %s\n", TTF_GetError());
        exit(1);
    }

    game->window = SDL_CreateWindow("tetris", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH_PX, 
                     SCREEN_HEIGHT_PX, SDL_WINDOW_SHOWN);

    END(game->window == NULL, "Could not create window", SDL_GetError());

    game->renderer = SDL_CreateRenderer(game->window, 0,
                                        SDL_RENDERER_SOFTWARE);

    END(game->renderer == NULL, "Could not create renderer", SDL_GetError());
}

void
drawPlaced(uint8_t *placed, SDL_Renderer *renderer) {
    for (int x = 0; x < ARENA_WIDTH; ++x) {
        for (int y = 0; y < ARENA_HEIGHT; ++y) {
            uint8_t i = y * ARENA_WIDTH + x;

            if (!placed[i]) continue;

            SDL_Rect rect = {
                .x = (int)(x * (int)BLOCK_SIZE_PX) + (int)ARENA_PADDING_PX,
                .y = (int)(y - ARENA_PADDING_TOP) * (int)BLOCK_SIZE_PX,
                .w = BLOCK_SIZE_PX, .h = BLOCK_SIZE_PX
            };

            setColor(renderer, COLOR_GREY);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }
}

static uint8_t
updateLose(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown)
{
    SDL_Point point = {.x = ARENA_WIDTH_PX / 2 + ARENA_PADDING_PX,
                       .y = ARENA_HEIGHT_PX / 2};

    drawPlaced(game->placed, game->renderer);
    drawText(game->renderer, game->lose_font, "You Lose", point);
    return UPDATE_LOSE;
}

static uint8_t
updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown)
{
    static SDL_Point piece_position = {.x = 0, .y = -1};
    static uint8_t fall_speed = 30;
    static uint8_t current_piece[PIECE_SIZE];
    static uint8_t color = COLOR_RED;
    static bool init = true;

    if (init) {
        srand(time(NULL));
        memset(&game->placed, 0, sizeof(uint8_t) * ARENA_SIZE);
        pickPiece(current_piece, &color);
        init = false;
    }

    if (piece_position.y == -1) {
        Size size;
        getPieceSize(current_piece, &size);
        piece_position.x = (ARENA_WIDTH / 2) - (size.w / 2);
    }

    drawTetromino(game->renderer, current_piece, piece_position, color);

    if (!keydown) fall_speed = 30;

    switch (key) {
        case SDLK_d: {
            SDL_Point check = {.x = piece_position.x + 1,
                               .y = piece_position.y};

            if (!collisionCheck(game->placed, current_piece, check)) {
                piece_position.x++;
            }

            break;
        }

        case SDLK_a: {
            SDL_Point check = {.x = piece_position.x - 1,
                               .y = piece_position.y};

            if (!collisionCheck(game->placed, current_piece, check)) {
                piece_position.x--;
            }

            break;
        }

        case SDLK_s: {
            fall_speed = 1;
            break;
        }

        case SDLK_r: {
            uint8_t rotated[PIECE_SIZE];
            SDL_Point check;

            rotatePiece(current_piece, rotated);

            uint8_t collide = collisionCheck(game->placed, rotated,
                                             piece_position);

            memcpy(&check, &piece_position, sizeof(SDL_Point));

            if (collide == COLLIDE_LEFT) {
                while (collide == COLLIDE_LEFT) {
                    check.x++;
                    collide = collisionCheck(game->placed, rotated, check);
                }

            } else if (collide == COLLIDE_RIGHT) {
                while (collide == COLLIDE_RIGHT) {
                    check.x--;
                    collide = collisionCheck(game->placed, rotated, check);
                }
            }

            if (collide == COLLIDE_NONE) {
                memcpy(&piece_position, &check, sizeof(SDL_Point));
                memcpy(current_piece, rotated, sizeof(uint8_t) * PIECE_SIZE);
            }

            break;
        }

        case SDLK_p: // Pause the game
            return UPDATE_PAUSE;
    }

    if (frame % fall_speed == 0) {
        SDL_Point check = {.x = piece_position.x,
                           .y = piece_position.y + 1};
        if (!collisionCheck(game->placed, current_piece, check))  {
            piece_position.y++;
        } else{
            Size size;

            getPieceSize(current_piece, &size);

            if (piece_position.y + size.start_y - size.h < 0) {
                addToPlaced(game->placed, current_piece, piece_position);
                return UPDATE_LOSE;
            } else {
                fall_speed = 30;
                addToPlaced(game->placed, current_piece, piece_position);
                pickPiece(current_piece, &color);
                piece_position.y = -1;
            }
        } 
    }

    uint8_t lines = checkForRowClearing(game->placed);
    SDL_Point point = {.x = ARENA_PADDING_PX / 2, .y = 50};
    char score_string[255];

    game->score += findPoints(game->level, lines);

    sprintf(score_string, "score %ld", game->score);
    drawText(game->renderer, game->ui_font, score_string, point);
    drawPlaced(game->placed, game->renderer);
    return UPDATE_MAIN;
}

void
Game_Update(Game *game, const uint8_t fps)
{
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;
    uint8_t update_id = UPDATE_MAIN; // Start with the main game state
    Update_callback update;
    float mspd = (1.0f / (float)fps) * 1000.0f;

    SDL_Rect arena_background_rect = {
        .x = ARENA_PADDING_PX,
        .y = 0,
        .w = ARENA_WIDTH_PX,
        .h = ARENA_HEIGHT_PX
    };

    while (!quit) {
        uint32_t start = SDL_GetTicks();

        switch (update_id) {
            case UPDATE_MAIN: update = updateMain; break;
            case UPDATE_LOSE: update = updateLose; break;
            case UPDATE_PAUSE: update = updatePause; break;
        }

        setColor(game->renderer, COLOR_GREY);
        SDL_RenderClear(game->renderer);
        setColor(game->renderer, COLOR_BLACK);
        SDL_RenderFillRect(game->renderer, &arena_background_rect);

        SDL_Event event;
        int key = 0;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN: {
                    if (event.key.repeat == 0) {
                      key = event.key.keysym.sym;
                      keydown = true;
                    }

                    break;
                }

                case SDL_KEYUP: keydown = false; break;
                case SDL_QUIT: quit = true; break;
            }
        }

        update_id = update(game, frame, (SDL_KeyCode)key, keydown);

        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        if (elapsed_time < mspd) {
            elapsed_time = mspd - elapsed_time;
            SDL_Delay(elapsed_time);
        } 

        SDL_RenderPresent(game->renderer);
        frame++;
    }
}

void
Game_Quit(Game *game)
{
    TTF_CloseFont(game->lose_font);
    TTF_CloseFont(game->ui_font);
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    TTF_Quit();
    SDL_Quit();
}

void Game_Login(Game *game, char *username, size_t username_size)
{
    bool quit = false;
    bool enter_pressed = false;
    SDL_StartTextInput();
    memset(username, 0, username_size);

    // Simple blinking cursor
    bool show_cursor = true;
    uint32_t cursor_timer = SDL_GetTicks();

    while (!quit && !enter_pressed) {
        // Handle cursor blinking
        uint32_t current_time = SDL_GetTicks();
        if (current_time - cursor_timer > 500) {
            show_cursor = !show_cursor;
            cursor_timer = current_time;
        }

        // Process events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_TEXTINPUT) {
                if (strlen(username) < username_size - 1) {
                    strcat(username, event.text.text);
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(username) > 0) {
                    username[strlen(username) - 1] = '\0';
                } else if (event.key.keysym.sym == SDLK_RETURN && strlen(username) > 0) {
                    enter_pressed = true;
                }
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
        SDL_RenderClear(game->renderer);

        // Draw text
        int text_width, text_height;
        char display_text[256];
        
        if (strlen(username) == 0) {
            strcpy(display_text, "Enter Your Name: ");
        } else {
            sprintf(display_text, "Name: %s", username);
        }
        
        TTF_SizeText(game->ui_font, display_text, &text_width, &text_height);
        
        SDL_Point text_position = {
            .x = SCREEN_WIDTH_PX / 2 - text_width / 2,
            .y = SCREEN_HEIGHT_PX / 2 - text_height / 2
        };
        
        drawText(game->renderer, game->ui_font, display_text, text_position);
    
        SDL_RenderPresent(game->renderer);
    }

    SDL_StopTextInput();

    if (quit) {
        Game_Quit(game);
        exit(0);
    }
    
    printf("Current username: '%s'\n", username);
}
int
main(int argc, char *argv[])
{
    Game game;
    char username[50];

    Game_Init(&game);

    Game_Login(&game, username, sizeof(username));
    printf("Welcome, %s!\n", username);

    Game_Update(&game, 60);

    printf("Goodbye, %s! Your final score is %lu.\n", username, game.score);

    Game_Quit(&game);
    return 0;
}
