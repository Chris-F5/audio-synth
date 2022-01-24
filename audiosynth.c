#include "audio.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

struct EditorCtx {
    TTF_Font* font;
    SDL_Color fgCol;
    SDL_Color bgCol;
    struct ChannelFrame* frame;
    int cursorX;
    int cursorY;
    int beatHeight;
    int modAttribWidth;
    int interModPadding;
    int intraModPadding;
};

void EditorCtx_init(struct EditorCtx* ctx, struct ChannelFrame* frame)
{
    //ctx->font = TTF_OpenFont("/usr/share/fonts/liberation/LiberationMono-Regular.ttf", 25);
    ctx->font = TTF_OpenFont("kongtext.ttf", 8 * 3);
    if (ctx->font == NULL) {
        puts("ERROR: could not load font. exiting");
        exit(1);
    }
    ctx->fgCol = (SDL_Color) { 255, 255, 255 };
    ctx->bgCol = (SDL_Color) { 0, 0, 0 };
    ctx->frame = frame;
    ctx->cursorX = ctx->cursorY = 0;
    ctx->modAttribWidth = 3 * 16;
    ctx->interModPadding = 3 * 4;
    ctx->intraModPadding = 3 * 2;
    ctx->beatHeight = 8 * 3;
}

void EditorCtx_setFrameAttrib(struct EditorCtx* ctx, unsigned char x)
{
    struct ChannelMod* mod = &ctx->frame->mods[ctx->cursorY * ctx->frame->modCount + ctx->cursorX / 2];
    if (ctx->cursorX % 2 == 0) {
        mod->type = x;
    } else {
        mod->value = x;
    }
}

void EditorCtx_destroy(struct EditorCtx* ctx)
{
    TTF_CloseFont(ctx->font);
}

void handleKeyboardEvent(struct EditorCtx* editor, SDL_KeyboardEvent* event)
{
    switch (event->keysym.sym) {
    case SDLK_l:
        editor->cursorX++;
        editor->cursorX = MIN(editor->cursorX, editor->frame->modCount * 2 - 1);
        break;
    case SDLK_h:
        editor->cursorX--;
        editor->cursorX = MAX(editor->cursorX, 0);
        break;
    case SDLK_j:
        editor->cursorY++;
        editor->cursorY = MIN(editor->cursorY, editor->frame->len - 1);
        break;
    case SDLK_k:
        editor->cursorY--;
        editor->cursorY = MAX(editor->cursorY, 0);
        break;
    case SDLK_q:
        puts("1");
        EditorCtx_setFrameAttrib(
            editor,
            (editor->cursorX % 2 == 0) ? 1 : 2);
        break;
    }
}

void drawText(
    SDL_Renderer* renderer,
    char* text,
    TTF_Font* font,
    SDL_Rect* rect,
    SDL_Color fgCol,
    SDL_Color bgCol)
{
    SDL_Surface* surface;
    SDL_Texture* tex;
    surface = TTF_RenderText(font, text, fgCol, bgCol);
    tex = SDL_CreateTextureFromSurface(renderer, surface);
    int w, h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);

    SDL_Rect srcRect = { 0, 0, MIN(w, rect->w), MIN(h, rect->h) };
    SDL_Rect dstRect = { rect->x, rect->y, MIN(w, rect->w), MIN(h, rect->h) };

    SDL_RenderCopy(renderer, tex, &srcRect, &dstRect);

    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surface);
}

void draw(struct EditorCtx* ctx, SDL_Renderer* renderer)
{
    for (int b = 0; b < ctx->frame->len; b++) {
        for (int m = 0; m < ctx->frame->modCount; m++) {
            char text[3];
            struct ChannelMod* mod = &ctx->frame->mods[b * ctx->frame->modCount + m];
            SDL_Color fg, bg;
            SDL_Rect rect;
            /* TYPE */
            sprintf(text, "%02X", mod->type);
            fg = ctx->fgCol;
            bg = ctx->bgCol;
            if (b == ctx->cursorY && m * 2 == ctx->cursorX) {
                fg = ctx->bgCol;
                bg = ctx->fgCol;
            }
            rect = (SDL_Rect) {
                m * (ctx->modAttribWidth * 2 + ctx->intraModPadding + ctx->interModPadding),
                b * ctx->beatHeight,
                ctx->modAttribWidth,
                ctx->beatHeight
            };
            drawText(renderer, text, ctx->font, &rect, fg, bg);
            /* VALUE */
            sprintf(text, "%02X", mod->value);
            fg = ctx->fgCol;
            bg = ctx->bgCol;
            if (b == ctx->cursorY && m * 2 + 1 == ctx->cursorX) {
                fg = ctx->bgCol;
                bg = ctx->fgCol;
            }
            rect.x += ctx->modAttribWidth + ctx->intraModPadding;
            drawText(renderer, text, ctx->font, &rect, fg, bg);
        }
    }
}

int main()
{
    struct ChannelFrame frame;
    ChannelFrame_init(&frame, 16, 2);
    struct ChannelMod mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_FREQ, 2 };
    frame.mods[4] = mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, (uint8_t)(UINT8_MAX * 0.3) };
    frame.mods[5] = mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_FREQ, 1 };
    frame.mods[8] = mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, 0 };
    frame.mods[12] = mod;

    //audioInit(0.5, 120, &frame);

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow(
        "window title",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        500,
        500,
        SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    struct EditorCtx editorCtx;
    EditorCtx_init(&editorCtx, &frame);

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            quit = true;
            break;
        case SDL_KEYDOWN:
            handleKeyboardEvent(&editorCtx, (SDL_KeyboardEvent*)&event.key);
            break;
        }
        SDL_RenderClear(renderer);

        draw(&editorCtx, renderer);

        SDL_RenderPresent(renderer);
    }

    EditorCtx_destroy(&editorCtx);
    ChannelFrame_destroy(&frame);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    // audioTerminate();

    return 0;
}
