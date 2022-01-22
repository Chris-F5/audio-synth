#include "audio.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

struct EditorCtx {
    TTF_Font* font;
    SDL_Color fgCol;
    SDL_Color bgCol;
    struct ChannelFrame frame;
};

void EditorCtx_init(struct EditorCtx* ctx)
{
    ctx->font = TTF_OpenFont("/usr/share/fonts/liberation/LiberationMono-Regular.ttf", 25);
    if (ctx->font == NULL) {
        puts("ERROR: could not load font. exiting");
        exit(1);
    }
    ctx->fgCol = (SDL_Color) { 255, 255, 255 };
    ctx->bgCol = (SDL_Color) { 0, 0, 0 };
    ChannelFrame_init(&ctx->frame, 16, 2);
}

void EditorCtx_destroy(struct EditorCtx* ctx)
{
    TTF_CloseFont(ctx->font);
    ChannelFrame_destroy(&ctx->frame);
}

void draw(struct EditorCtx* ctx, SDL_Renderer* renderer)
{
    SDL_Surface* txtSurface;
    SDL_Texture* txtTex;
    for (int b = 0; b < ctx->frame.len; b++) {
        char text[8];
        sprintf(text, "%d", b);
        txtSurface = TTF_RenderText(ctx->font, text, ctx->fgCol, ctx->bgCol);
        txtTex = SDL_CreateTextureFromSurface(renderer, txtSurface);

        int texW, texH;
        SDL_QueryTexture(txtTex, NULL, NULL, &texW, &texH);
        SDL_Rect dstRect = { 0, b * 25, texW, texH };

        SDL_RenderCopy(renderer, txtTex, NULL, &dstRect);

        SDL_FreeSurface(txtSurface);
        SDL_DestroyTexture(txtTex);
    }
}

int main()
{
    struct ChannelFrame frame;
    ChannelFrame_init(&frame, 16, 2);
    struct ChannelMod mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_FREQ, 432 };
    frame.mods[4] = mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, (uint16_t)(UINT32_MAX * 0.3) };
    frame.mods[5] = mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_FREQ, 432 * 3 / 2 };
    frame.mods[8] = mod;
    mod = (struct ChannelMod) { CHANNEL_MOD_TYPE_AMP, 0 };
    frame.mods[12] = mod;

    // audioInit(0.5, 120, &frame);

    SDL_Init(SDL_INIT_EVERYTHING);
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
    EditorCtx_init(&editorCtx);

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            quit = true;
            break;
        }
        SDL_RenderClear(renderer);

        draw(&editorCtx, renderer);

        SDL_RenderPresent(renderer);
    }

    EditorCtx_destroy(&editorCtx);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    // audioTerminate();

    return 0;
}
