/*
 * Wii U entry point for SDLReader.
 *
 * Differences from cli/main.cpp:
 *  - there is no command line, so the PDF is picked from a list of folders
 *  - stdout/stderr are routed to WHBLog (Cemu log.txt / udplogserver)
 *  - the font and a sample PDF ship inside the .wuhb (mounted at /vol/content)
 *  - SDL2 drives ProcUI (HOME menu, exit to menu) itself
 */

#include "app.h"
#include "renderer.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>

#include <sys/iosupport.h>
#include <dirent.h>
#include <strings.h>

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

static const char *FONT_PATH = "fs:/vol/content/Roboto-Regular.ttf";

// Searched in order; the first folder containing a .pdf wins.
static const char *PDF_DIRS[] = {
    "fs:/vol/external01/wiiu/apps/SDLReader",
    "fs:/vol/external01/pdf",
    "fs:/vol/content",
};

// --- stdout/stderr -> WHBLog -------------------------------------------------

static ssize_t logWrite(struct _reent *, void *, const char *ptr, size_t len)
{
    static std::string line;
    for (size_t i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            WHBLogPrint(line.c_str());
            line.clear();
        } else {
            line += ptr[i];
        }
    }
    return len;
}

static devoptab_t s_logDevoptab;

static void initLogging()
{
    WHBLogCafeInit(); // OSReport: Cemu log.txt
    WHBLogUdpInit();  // real hardware: run udplogserver on the PC

    s_logDevoptab.name = "whblog";
    s_logDevoptab.write_r = logWrite;
    devoptab_list[STD_OUT] = &s_logDevoptab;
    devoptab_list[STD_ERR] = &s_logDevoptab;
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
}

static void deinitLogging()
{
    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
}

// --- PDF selection -----------------------------------------------------------

static bool hasPdfExtension(const std::string &name)
{
    return name.size() > 4 && strcasecmp(name.c_str() + name.size() - 4, ".pdf") == 0;
}

static std::string findFirstPdf()
{
    for (const char *dir : PDF_DIRS) {
        DIR *d = opendir(dir);
        if (!d) {
            printf("No folder %s\n", dir);
            continue;
        }
        std::vector<std::string> pdfs;
        while (struct dirent *ent = readdir(d)) {
            if (hasPdfExtension(ent->d_name)) {
                pdfs.push_back(ent->d_name);
            }
        }
        closedir(d);

        if (!pdfs.empty()) {
            std::sort(pdfs.begin(), pdfs.end());
            return std::string(dir) + "/" + pdfs.front();
        }
        printf("No PDFs in %s\n", dir);
    }
    return "";
}

// --- Error screen ------------------------------------------------------------

// Shows a message until the user quits, so failures aren't a black screen.
static void showMessage(SDL_Renderer *renderer, const std::string &message)
{
    TTF_Font *font = TTF_OpenFont(FONT_PATH, 28);
    SDL_Texture *texture = nullptr;
    SDL_Rect rect = { 40, 40, 0, 0 };

    if (font) {
        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, message.c_str(), white, 1200);
        if (surface) {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            rect.w = surface->w;
            rect.h = surface->h;
            SDL_FreeSurface(surface);
        }
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
        SDL_RenderClear(renderer);
        if (texture) {
            SDL_RenderCopy(renderer, texture, nullptr, &rect);
        }
        SDL_RenderPresent(renderer);
    }

    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (font) {
        TTF_CloseFont(font);
    }
}

// --- Cleanup Function --------------------------------------------------------

static void cleanupSDL(SDL_Window *window, SDL_Renderer *renderer)
{
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char **argv)
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    int returnCode = 0;

    initLogging();
    printf("SDLReader for Wii U starting\n");

    if (SDL_Init(Renderer::getRequiredSDLInitFlags()) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        deinitLogging();
        return 1;
    }
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        cleanupSDL(window, renderer);
        deinitLogging();
        return 1;
    }

    window = SDL_CreateWindow("SDLReader", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              1280, 720, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanupSDL(window, renderer);
        deinitLogging();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanupSDL(window, renderer);
        deinitLogging();
        return 1;
    }
    printf("SDL window and renderer created\n");

    std::string pdfPath = findFirstPdf();
    if (pdfPath.empty()) {
        showMessage(renderer, "No PDF found.\n\nCopy PDFs to sd:/wiiu/apps/SDLReader/\n\nPress HOME to exit.");
        returnCode = 1;
    } else {
        printf("Opening %s\n", pdfPath.c_str());
        try {
            App app(pdfPath, window, renderer, FONT_PATH);
            printf("SDLREADER_RUNNING\n");
            app.run();
        } catch (const std::exception &e) {
            printf("Application Error: %s\n", e.what());
            showMessage(renderer, std::string("Error: ") + e.what() + "\n\nPress HOME to exit.");
            returnCode = 1;
        }
    }

    printf("SDLReader exiting\n");
    cleanupSDL(window, renderer);
    deinitLogging();
    return returnCode;
}
