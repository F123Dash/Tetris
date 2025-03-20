#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 300
#define MAX_INPUT_LEN 20

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

char username[MAX_INPUT_LEN + 1] = "";
char password[MAX_INPUT_LEN + 1] = "";
bool is_password = false;
bool login_success = false;
bool input_username = true;

void renderText(const char *text, int x, int y) {
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void renderInputBox(const char *text, int x, int y) {
    SDL_Rect box = {x - 5, y - 5, 200, 30};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &box);
    renderText(text, x, y);
}

void handleInput(SDL_Event *e) {
    if (e->type == SDL_TEXTINPUT || e->type == SDL_KEYDOWN) {
        char *inputField = input_username ? username : password;
        int len = strlen(inputField);
        
        if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_BACKSPACE && len > 0) {
            inputField[len - 1] = '\0';
        } else if (e->type == SDL_TEXTINPUT && len < MAX_INPUT_LEN) {
            strcat(inputField, e->text.text);
        }
    }
}

void checkLogin() {
    if (strcmp(username, "admin") == 0 && strcmp(password, "1234") == 0) {
        login_success = true;
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderText("Username:", 50, 80);
    renderInputBox(username, 150, 80);

    renderText("Password:", 50, 130);
    char masked[MAX_INPUT_LEN + 1];
    memset(masked, '*', strlen(password));
    masked[strlen(password)] = '\0';
    renderInputBox(masked, 150, 130);

    if (login_success) {
        renderText("Login Successful!", 120, 200);
    } else {
        renderText("Press Enter to Login", 120, 220);
    }

    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    
    window = SDL_CreateWindow("Login Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("arial.ttf", 24);
    
    SDL_StartTextInput();
    bool running = true;
    SDL_Event e;
    
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) checkLogin();
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_TAB) input_username = !input_username;
            handleInput(&e);
        }
        render();
    }
    
    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}