//Preprocessor Directives
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

// Forward declarations of structs
typedef struct Game Game;

// Function declarations
void setColor(SDL_Renderer *renderer, uint8_t color);
void Game_Quit(Game *game);
void Game_Login(Game *game, char *username, size_t username_size);

//Macro Definitions
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
#define FONT "./fonts/CC_Wild_Words_Roman.ttf"
#define MAX_HIGH_SCORES 4
#define HIGH_SCORE_FILE "highscores.txt"
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b)) 
#define END(check, str1, str2) \
    if (check) { \
        assert(check); \
        fprintf(stderr, "%s\n%s", str1, str2); \
        exit(1); \
    } \

enum {PIECE_I, PIECE_J, PIECE_L, PIECE_O, PIECE_S, PIECE_T, PIECE_Z, PIECE_COUNT};
enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY, COLOR_BLACK, COLOR_SIZE};
enum {COLLIDE_NONE = 0, COLLIDE_LEFT = 1 << 0, COLLIDE_RIGHT = 1 << 1, COLLIDE_TOP = 1 << 2, COLLIDE_BOTTOM = 1 << 3, COLLIDE_PIECE = 1 << 4};
enum {UPDATE_MAIN, UPDATE_LOSE, UPDATE_PAUSE, UPDATE_GAME_OVER};
//Size of a Single Tetris Piece 
typedef struct Size {
    int w;
    int h;
    uint8_t start_x;
    uint8_t start_y;
} Size;
//Stores info about a Player's HighScore
typedef struct HighScore {
    char name[50];
    uint64_t score;
} HighScore;
// Represents Overall State of the Game
typedef struct Game {
    uint8_t level;                           // current level (affect the difficulty)
    uint64_t score;                          // current score
    SDL_Renderer *renderer;                  // SDL renderer used to draw graphics
    SDL_Window *window;                      // SDL window 
    TTF_Font *lose_font;                     // Font used for "Game Over"
    TTF_Font *ui_font;                       // Font used for scores and instructions
    uint8_t placed[ARENA_SIZE]; // 8 x 18 */ // A 1D array representing the arena grid (8x18 blocks). Each element indicates whether a block is occupied
    HighScore high_scores[MAX_HIGH_SCORES];  // An array to store top high scores 
    int num_high_scores;                     // The number of high scores currently stored
    uint32_t total_rows_cleared;             // Tracks the total number of rows cleared
} Game;
typedef uint8_t (*Update_callback)(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown);  //Defines a function pointer that updates the game based on the current frame, user input etc.
static char current_username[50];  // Global variable to store current username
//Function based on rendering text on screen
void drawText(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Point point){
    if (text == NULL || strlen(text) == 0) {
        fprintf(stderr, "Text is empty");
        return;
    }
    int w = 0;
    int h = 0;
    TTF_SizeText(font, text, &w, &h);
    SDL_Color font_color = {.r = 255, .g = 255, .b = 255, .a = 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, font_color);
    END(surface == NULL, "Could not create surface\n", SDL_GetError());
    //Position the text on the screen
    SDL_Rect rect = {
        .x = point.x - (w / 2),
        .y = point.y - (h / 2),
        .w = w,
        .h = h,
    };
    //Convert thr suraface into a texture (This step is important because SDL2 renders textures, not surfaces)
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    END(texture == NULL, "Could not create texture", SDL_GetError());
    //Renders the texture onto screen
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
//Reads the high scores stored in a file
void load_high_scores(Game *game) {
    FILE *file = fopen(HIGH_SCORE_FILE, "r");
    game->num_high_scores = 0;    
    if (file != NULL) {
        while (game->num_high_scores < MAX_HIGH_SCORES && fscanf(file, "%49[^,],%lu\n", game->high_scores[game->num_high_scores].name, &game->high_scores[game->num_high_scores].score) == 2) {
            game->num_high_scores++;
        }
        fclose(file);
    }
}
//Saving the current high scores
void save_high_scores(Game *game) {
    FILE *file = fopen(HIGH_SCORE_FILE, "w");
    if (file != NULL) {
        for (int i = 0; i < game->num_high_scores; i++) {
            fprintf(file, "%s,%lu\n", game->high_scores[i].name, game->high_scores[i].score);
        }
        fclose(file);
    }
}
// updating the high scores list when a player achieves a new score
void update_high_scores(Game *game, const char *name, uint64_t score) {
    // Find position to insert new score
    int pos = game->num_high_scores;
    for (int i = 0; i < game->num_high_scores; i++) {
        if (score > game->high_scores[i].score) {
            pos = i;
            break;
        }
    }
    // If score is high enough to be in top 4
    if (pos < MAX_HIGH_SCORES) {
        // Shift lower scores down
        for (int i = MIN(game->num_high_scores, MAX_HIGH_SCORES - 1); i > pos; i--) {
            memcpy(&game->high_scores[i], &game->high_scores[i-1], sizeof(HighScore));
        }
        // Insert new score
        strncpy(game->high_scores[pos].name, name, sizeof(game->high_scores[pos].name) - 1);
        game->high_scores[pos].score = score;
        if (game->num_high_scores < MAX_HIGH_SCORES){
            game->num_high_scores++;
        }
        save_high_scores(game);
    }
}
//Rendering the high scores screen in the Tetris game
void draw_high_scores(Game *game, SDL_Rect game_over_box) {
        SDL_Rect container = {
            .x = SCREEN_WIDTH_PX / 4,
            .y = game_over_box.y + game_over_box.h + 20, 
            .w = SCREEN_WIDTH_PX / 2,
            .h = SCREEN_HEIGHT_PX / 2
        };
        SDL_Rect glow = {
            .x = container.x - 4,
            .y = container.y - 4,
            .w = container.w + 8,
            .h = container.h + 8
        };
        setColor(game->renderer, COLOR_BLUE);
        SDL_RenderFillRect(game->renderer, &glow);

        setColor(game->renderer, COLOR_BLACK);
        SDL_RenderFillRect(game->renderer, &container);
        SDL_Point title_pos = {
            .x = SCREEN_WIDTH_PX / 2,
            .y = container.y + 30
        };
        drawText(game->renderer, game->lose_font, "HIGH SCORES", title_pos);
    
        int start_y = title_pos.y + 100;
        int spacing = 60;
        char score_text[256];
        for (int i = 0; i < game->num_high_scores && i < MAX_HIGH_SCORES; i++) {
            SDL_Point score_pos = {
                .x = SCREEN_WIDTH_PX / 2,
                .y = start_y + (i * spacing)
            };
            sprintf(score_text, "#%d  %-20s %8lu", i + 1, game->high_scores[i].name, game->high_scores[i].score);
            drawText(game->renderer, game->ui_font, score_text, score_pos);
        }
        if (game->num_high_scores == 0) {
            SDL_Point no_scores_pos = {
                .x = SCREEN_WIDTH_PX / 2,
                .y = start_y + spacing
            };
            drawText(game->renderer, game->ui_font, "No high scores", no_scores_pos);
        }
        SDL_Point instructions_pos = {
                .x = SCREEN_WIDTH_PX / 2,
                .y = container.y + container.h - 40
            };
            drawText(game->renderer, game->ui_font, "Press SPACE to continue", instructions_pos);
    }
    
//rendering the pause menu
static uint8_t updatePause(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown){
    SDL_Point title_point = {
        .x = SCREEN_WIDTH_PX / 2,
        .y = SCREEN_HEIGHT_PX / 2 - 160 
    };
    SDL_Point resume_point = {
        .x = SCREEN_WIDTH_PX / 2,
        .y = SCREEN_HEIGHT_PX / 2 - 80
    };
    SDL_Point menu_point = {
        .x = SCREEN_WIDTH_PX / 2,
        .y = SCREEN_HEIGHT_PX / 2
    };
    SDL_Point exit_point = {
        .x = SCREEN_WIDTH_PX / 2,
        .y = SCREEN_HEIGHT_PX / 2 + 80
    };
    // Add a semi-transparent overlay
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 200);
    SDL_Rect pause_overlay = {
        .x = 0,
        .y = 0,
        .w = SCREEN_WIDTH_PX,
        .h = SCREEN_HEIGHT_PX
    };
    SDL_RenderFillRect(game->renderer, &pause_overlay);

    // Add a container for the pause menu
    SDL_Rect pause_container = {
        .x = SCREEN_WIDTH_PX / 4,
        .y = SCREEN_HEIGHT_PX / 4,
        .w = SCREEN_WIDTH_PX / 2,
        .h = SCREEN_HEIGHT_PX / 2
    };
    // Add a subtle glow effect
    SDL_Rect glow = pause_container;
    glow.x -= 4;
    glow.y -= 4;
    glow.w += 8;
    glow.h += 8;
    setColor(game->renderer, COLOR_BLUE);
    SDL_RenderFillRect(game->renderer, &glow);
    setColor(game->renderer, COLOR_BLACK);
    SDL_RenderFillRect(game->renderer, &pause_container);
    drawText(game->renderer, game->lose_font, "PAUSED", title_point);
    drawText(game->renderer, game->ui_font, "Press R to Resume", resume_point);
    drawText(game->renderer, game->ui_font, "Press M for Main Menu", menu_point);
    drawText(game->renderer, game->ui_font, "Press E to Exit Game", exit_point);
    if (keydown) {
        switch (key) {
            case SDLK_r: // Resume
                return UPDATE_MAIN;
            case SDLK_m: // Return to main menu
                return UPDATE_GAME_OVER;  // This will trigger the login screen
            case SDLK_e: // Exit game completely
                Game_Quit(game);
                exit(0);
        }
    }
    return UPDATE_PAUSE;//if no key is pressed the game state is kept same
}
//calculate score based on current level
int findPoints(uint8_t level, uint8_t lines){
    switch (lines) {
        case 1: return 40 * (level + 1);
        case 2: return 100 * (level + 1);
        case 3: return 300 * (level + 1);
        case 4: return 1200 * (level + 1);
    }
    return 0;
}
//marking a specific position in game as occupied
void addToArena(uint8_t *placed, uint8_t i){
    if (i < ARENA_SIZE && i >= 0){
        placed[i] = 1;
        }
}
//calculates the index in the placed array coressponding to a given 2D position
uint8_t getPlacedPosition(SDL_Point pos){
    uint8_t i = pos.y * ARENA_WIDTH + pos.x;
    return i < ARENA_SIZE ? i : ARENA_SIZE - 1;
}
//converting a 1D index to its coressponding 2D position
void getXY(uint8_t i, int *x, int *y) {
    *x = i % PIECE_WIDTH;
    *y = floor((float)i / PIECE_HEIGHT);
}
//calculates dimensions of a Tetromino piece
void getPieceSize(uint8_t *piece, Size *size){
    memset(size, 0, sizeof(Size));

    bool foundStartx = false;
    bool foundStarty = false;
    //calclulate the width and starting X-offset 
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
    //calclulates the height and starting Y-offset
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
//Rotating a Tetromino piece 90 degrees clockwise
void rotatePiece(uint8_t *piece, uint8_t *rotated){
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
//Rendering Tetromino piece on screen
void drawTetromino(SDL_Renderer *renderer, uint8_t piece[PIECE_SIZE], SDL_Point position, uint8_t color){
    for (int i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        if (!piece[i]) {
                continue;
        }
        int x, y; 
        getXY(i, &x, &y);
        SDL_Rect rect = {
            .x = (int)((x + position.x) * (int)BLOCK_SIZE_PX) + (int)ARENA_PADDING_PX,
            .y = (int)(y + position.y - ARENA_PADDING_TOP) * (int)BLOCK_SIZE_PX,
            .w = BLOCK_SIZE_PX, 
            .h = BLOCK_SIZE_PX
        };
        setColor(renderer, color);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}
//provides a visual representation of the arena
void printPlaced(uint8_t *placed){
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
//Clearing a completed row and shifting the above one below
void clearRow(uint8_t *placed, uint8_t c){
    uint8_t temp[ARENA_SIZE];
    memset(temp, 0, sizeof(uint8_t) * ARENA_SIZE);
    memcpy(temp, placed, sizeof(uint8_t) * ARENA_SIZE);
    for (int y = c; y > 0; --y) {
        for (uint8_t x = 0; x < ARENA_WIDTH; ++x) {
            uint8_t i = y * ARENA_WIDTH + x;
            if (i >= 0) {
                temp[i] = placed[i - ARENA_WIDTH];
                }
        }
    }
    memcpy(placed, temp, sizeof(uint8_t) * ARENA_SIZE);
}
//detecting and clearing fully occupied rows in arena 
uint8_t checkForRowClearing(uint8_t *placed){
    uint8_t row_count = 0;
    uint8_t lines = 0;
    for (uint8_t y = 0; y < ARENA_HEIGHT; ++y) {
        for (uint8_t x = 0; x < ARENA_WIDTH; ++x) {
            if (x == 0) {
                row_count = 0;
                }
            uint8_t i = y * ARENA_WIDTH + x;
            if (placed[i]) {
                row_count++;
                }
            if ((x == ARENA_WIDTH - 1) && (row_count == ARENA_WIDTH)) {
                clearRow(placed, y);
                lines++;
            }
        }
    }
    return lines;
}
//Adding a Tetromino piece to placed array
void addToPlaced(uint8_t *placed, uint8_t *piece, SDL_Point position){
    uint8_t pos = getPlacedPosition(position);
    for (uint8_t i = 0; i < TETROMINOS_DATA_SIZE; ++i) {
        int x, y; 
        getXY(i, &x, &y);
        uint8_t piece_i = y * PIECE_WIDTH + x;
        uint8_t placed_i = y * ARENA_WIDTH + x;
        if (piece[piece_i]) {
                addToArena(placed, pos + placed_i);
        }
    }
}
//Detecting collision between pieces and boundary
uint8_t collisionCheck(uint8_t *placed, uint8_t *piece, SDL_Point position){
    Size size; 
    getPieceSize(piece, &size);
    uint8_t placed_pos = getPlacedPosition(position);
    uint8_t collide = COLLIDE_NONE;
    if (position.x < -size.start_x) {
        collide |= COLLIDE_LEFT;
        }
    if (position.x + size.start_x + size.w > ARENA_WIDTH){
        collide |= COLLIDE_RIGHT;
        }
    if (position.y + size.start_y + size.h > ARENA_HEIGHT){
        collide |= COLLIDE_BOTTOM;
        }
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
//Selecting Random Tetromino and assign them color
void pickPiece(uint8_t *piece, uint8_t *color){
    const uint8_t tetrominos[TETROMINOS_COUNT]
                                   [PIECE_SIZE] = {
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
    const uint8_t piece_colors[PIECE_COLOR_SIZE] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE};
    uint8_t id = (float)((float)rand() / (float)RAND_MAX) * PIECE_COUNT;
    memcpy(piece, &tetrominos[id], sizeof(uint8_t) * PIECE_SIZE);
    *color = piece_colors[((*color) + 1) % PIECE_COLOR_SIZE];
}
//Initailize the game 
void Game_Init(Game *game){
    memset(game, 0, sizeof(Game));
    END(SDL_Init(SDL_INIT_VIDEO) != 0, "Could not create texture", SDL_GetError());
    END(TTF_Init() != 0, "Could not initialize", TTF_GetError());
    game->lose_font = TTF_OpenFont(FONT, 50);
    END(game->lose_font == NULL, "Could not open font", TTF_GetError());
    game->ui_font = TTF_OpenFont(FONT, 30);
    if (game->ui_font == NULL) {
        fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
        exit(1);
    }
    game->window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH_PX, SCREEN_HEIGHT_PX, SDL_WINDOW_SHOWN);
    END(game->window == NULL, "Could not create window", SDL_GetError());
    game->renderer = SDL_CreateRenderer(game->window, 0, SDL_RENDERER_SOFTWARE);
    END(game->renderer == NULL, "Could not create renderer", SDL_GetError());
    load_high_scores(game);
    game->total_rows_cleared = 0;
}
//Rendering the placed blocks
void drawPlaced(uint8_t *placed, SDL_Renderer *renderer) {
    for (int x = 0; x < ARENA_WIDTH; ++x) {
        for (int y = 0; y < ARENA_HEIGHT; ++y) {
            uint8_t i = y * ARENA_WIDTH + x;
            if (!placed[i]) {
                continue;
            }
            SDL_Rect rect = {
                .x = (int)(x * (int)BLOCK_SIZE_PX) + (int)ARENA_PADDING_PX,
                .y = (int)(y - ARENA_PADDING_TOP) * (int)BLOCK_SIZE_PX,
                .w = BLOCK_SIZE_PX, 
                .h = BLOCK_SIZE_PX
            };
            setColor(renderer, COLOR_GREY);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }
}

static uint8_t updateLose(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown) {
    static bool first_update = true;

    if (first_update) {
        update_high_scores(game, current_username, game->score);
        first_update = false;
    }

    // Draw game over screen
    SDL_Rect overlay = {
        .x = 0,
        .y = 0,
        .w = SCREEN_WIDTH_PX,
        .h = SCREEN_HEIGHT_PX
    };
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(game->renderer, &overlay);

    // Draw game over container
    SDL_Rect container = {
        .x = SCREEN_WIDTH_PX / 4,
        .y = 50,
        .w = SCREEN_WIDTH_PX / 2,
        .h = 150
    };

    // Draw border glow
    SDL_Rect glow = {
        .x = container.x - 4,
        .y = container.y - 4,
        .w = container.w + 8,
        .h = container.h + 8
    };
    setColor(game->renderer, COLOR_BLUE);
    SDL_RenderFillRect(game->renderer, &glow);

    // Draw container background
    setColor(game->renderer, COLOR_BLACK);
    SDL_RenderFillRect(game->renderer, &container);

    // Draw "Game Over" text
    SDL_Point title_pos = {
        .x = SCREEN_WIDTH_PX / 2,
        .y = container.y + 50
    };
    drawText(game->renderer, game->lose_font, "GAME OVER!!", title_pos);

    // Draw final score
    char score_text[255];
    sprintf(score_text, "Final Score: %lu", game->score);
    SDL_Point score_pos = {
        .x = SCREEN_WIDTH_PX / 2,
        .y = container.y + 100
    };
    drawText(game->renderer, game->ui_font, score_text, score_pos);

    draw_high_scores(game, container);

    if (keydown) {
        switch (key) {
            case SDLK_SPACE:
                first_update = true;
                return UPDATE_GAME_OVER;
            case SDLK_ESCAPE:
                Game_Quit(game);
                exit(0);
        }
    }
    return UPDATE_LOSE;
}
//Responsible for resetting the game
static uint8_t updateGameOver(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown){
    game->score = 0;
    game->level = 0;
    memset(game->placed, 0, sizeof(uint8_t) * ARENA_SIZE);
    char username[50];
    Game_Login(game, username, sizeof(username));
    return UPDATE_MAIN;
}
//Core game Loop
static uint8_t updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown) {
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

    if (!keydown) {
        fall_speed = 30;
    }

    switch (key) {
        case SDLK_d: {
            SDL_Point check = {.x = piece_position.x + 1, .y = piece_position.y};
            if (!collisionCheck(game->placed, current_piece, check)) {
                piece_position.x++;
            }
            break;
        }
        case SDLK_a: {
            SDL_Point check = {.x = piece_position.x - 1, .y = piece_position.y};
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
            uint8_t collide = collisionCheck(game->placed, rotated, piece_position);
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
        case SDLK_ESCAPE:
            return UPDATE_PAUSE;
    }

    if (frame % fall_speed == 0) {
        SDL_Point check = {.x = piece_position.x, .y = piece_position.y + 1};
        if (!collisionCheck(game->placed, current_piece, check)) {
            piece_position.y++;
        } else {
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
    game->total_rows_cleared += lines;
    if (game->total_rows_cleared >= (game->level + 1) * 10) {
        game->level++;
        fall_speed = MAX(1, fall_speed - 2);
    }

    SDL_Point point = {.x = ARENA_PADDING_PX / 2, .y = 100};
    char score_string[255];
    game->score += findPoints(game->level, lines);
    sprintf(score_string, "Score: %ld", game->score);
    drawText(game->renderer, game->ui_font, score_string, point);

    SDL_Point level_point = {.x = ARENA_PADDING_PX / 2, .y = 150};
    char level_string[255];
    sprintf(level_string, "Level: %d", game->level);
    drawText(game->renderer, game->ui_font, level_string, level_point);

    drawPlaced(game->placed, game->renderer);
    return UPDATE_MAIN;
}
//Main Game Loop
void Game_Update(Game *game, const uint8_t fps){
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;
    uint8_t update_id = UPDATE_MAIN;
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
            case UPDATE_GAME_OVER: update = updateGameOver; break;
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

void Game_Quit(Game *game){
    TTF_CloseFont(game->lose_font);
    TTF_CloseFont(game->ui_font);
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    TTF_Quit();
    SDL_Quit();
}

void Game_Login(Game *game, char *username, size_t username_size) {
    bool quit = false;
    bool enter_pressed = false;
    SDL_StartTextInput();
    memset(username, 0, username_size);
    SDL_Surface *image_surface = SDL_LoadBMP("./images/tetris_logo.bmp");
    END(image_surface == NULL, "Could not load image", SDL_GetError());
    SDL_Texture *image_texture = SDL_CreateTextureFromSurface(game->renderer, image_surface);
    SDL_FreeSurface(image_surface); 
    END(image_texture == NULL, "Could not create image", SDL_GetError());
    SDL_Rect image_rect = {
        .x = SCREEN_WIDTH_PX / 2 - 150, 
        .y = SCREEN_HEIGHT_PX / 4 - 50, 
        .w = 300,
        .h = 100 
    };
    bool show_cursor = true;
    uint32_t cursor_timer = SDL_GetTicks();

    game->score = 0;
    game->level = 0;
    game->total_rows_cleared = 0;
    memset(game->placed, 0, sizeof(uint8_t) * ARENA_SIZE);

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
        // Clear screen with a modern gradient background
        SDL_SetRenderDrawColor(game->renderer, 35, 41, 50, 255);
        SDL_RenderClear(game->renderer);
        // Render the image
        SDL_RenderCopy(game->renderer, image_texture, NULL, &image_rect);
        // Draw decorative header
        SDL_Rect header_bg = {
            .x = 0,
            .y = 0,
            .w = SCREEN_WIDTH_PX,
            .h = 150
        };
        setColor(game->renderer, COLOR_BLUE);
        SDL_RenderFillRect(game->renderer, &header_bg);
        // Draw input section
        SDL_Rect input_border = {
            .x = SCREEN_WIDTH_PX / 4 - 10,
            .y = SCREEN_HEIGHT_PX - 220,
            .w = SCREEN_WIDTH_PX / 2 + 20,
            .h = 180
        };
        setColor(game->renderer, COLOR_BLUE);
        SDL_RenderFillRect(game->renderer, &input_border);
        SDL_Rect input_inner = input_border;
        input_inner.x += 2; input_inner.y += 2;
        input_inner.w -= 4; input_inner.h -= 4;
        setColor(game->renderer, COLOR_BLACK);
        SDL_RenderFillRect(game->renderer, &input_inner);
        // Draw all text without glow effect
        SDL_Point welcome_pos = {
            .x = SCREEN_WIDTH_PX / 2,
            .y = SCREEN_HEIGHT_PX - 190
        };
        drawText(game->renderer, game->ui_font, "Welcome to Tetris!", welcome_pos);
        char display_text[256];
        if (strlen(username) == 0) {
            strcpy(display_text, "Enter Your Name:  ");
        } else {
            sprintf(display_text, " %s%s", username, show_cursor ? "" : "");
        }
        SDL_Point text_position = {
            .x = SCREEN_WIDTH_PX / 2,
            .y = SCREEN_HEIGHT_PX - 140
        };
        drawText(game->renderer, game->ui_font, display_text, text_position);
        SDL_Point instructions_pos = {
            .x = SCREEN_WIDTH_PX / 2,
            .y = SCREEN_HEIGHT_PX - 90
        };
        drawText(game->renderer, game->ui_font, "Press ENTER to Start", instructions_pos);
        SDL_RenderPresent(game->renderer);
    }
    SDL_StopTextInput();
    // Free the image texture
    SDL_DestroyTexture(image_texture);
    if (quit) {
        Game_Quit(game);
        exit(0);
    }
    strncpy(current_username, username, sizeof(current_username) - 1);
}
//Setting Color for smth smth 
void setColor(SDL_Renderer *renderer, uint8_t color){
    const SDL_Color colors[] = {
        [COLOR_RED] = {.r = 227, .g = 66, .b = 52, .a = 255},     // Vermillion
        [COLOR_GREEN] = {.r = 61, .g = 100, .b = 45, .a = 255},   // Fern Green
        [COLOR_BLUE] = {.r = 2, .g = 86, .b = 105, .a = 255},     // Azure Blue
        [COLOR_ORANGE] = {.r = 255, .g = 164, .b = 32, .a = 255}, // Luminous Bright Orange
        [COLOR_GREY] = {.r = 71, .g = 75, .b = 78, .a = 255},     // Blue Grey
        [COLOR_BLACK] = {.r = 30, .g = 35, .b = 41, .a = 255},    // Deep Black
    };

    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g, colors[color].b, colors[color].a);
}
//MAIN FUNCTION (average (╥﹏╥))
int main(int argc, char *argv[]){
    Game game;
    char username[50];
    Game_Init(&game);
    Game_Login(&game, username, sizeof(username));
    Game_Update(&game, 60);
    Game_Quit(&game);
    return 0;
}
