/*
 * Fullscreen SDL2 display template for STM32MP1 (Weston/Wayland).
 * Usage: customer-display [image.bmp]
 * - no argument: fullscreen gradient
 * - with a BMP path: gradient + image filled to the output
 * Press ESC or close the window to exit.
 */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("customer-display",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       0, 0,
                                       SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren)
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_Surface *img = NULL;
    SDL_Texture *tex = NULL;
    if (argc > 1) {
        img = SDL_LoadBMP(argv[1]);
        if (!img) {
            fprintf(stderr, "SDL_LoadBMP(%s) failed: %s\n", argv[1], SDL_GetError());
        } else {
            tex = SDL_CreateTextureFromSurface(ren, img);
        }
    }

    int w = 1280, h = 720;
    if (SDL_GetRendererOutputSize(ren, &w, &h) != 0 || w <= 0 || h <= 0) {
        w = 1280;
        h = 720;
    }

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = 0;
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        for (int y = 0; y < h; ++y) {
            Uint8 r = (Uint8)(255 * y / h);
            Uint8 b = (Uint8)(255 - 255 * y / h);
            SDL_SetRenderDrawColor(ren, r, 64, b, 255);
            SDL_RenderDrawLine(ren, 0, y, w, y);
        }

        if (tex && img) {
            /* Fill the whole output. For letterbox, compute a scaled
             * rectangle preserving img->w / img->h instead. */
            SDL_Rect dst = {0, 0, w, h};
            SDL_RenderCopy(ren, tex, NULL, &dst);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (tex)
        SDL_DestroyTexture(tex);
    if (img)
        SDL_FreeSurface(img);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
