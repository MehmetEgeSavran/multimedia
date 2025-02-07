#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <math.h>

#define COLOR_BACKGROUND 0xFF808080
#define COLOR_TEXT_PRIMARY 0xFFFFFFFF
#define COLOR_TEXT_SECONDARY 0xFFA9A9A9
#define COLOR_WIDGE 0xFFD2D2D2

static int SCREEN_WIDTH = 640 * 1.75, SCREEN_HEIGHT = 360 * 1.75;
static int imageOffsetX = 0, imageOffsetY = 0;
static float imageZoom = 1.0f;
static uint32_t EightBitPalette[256];

static float alphaScale = 1.0f;
static float redScale = 1.0f;
static float greenScale = 1.0f;
static float blueScale = 1.0f;

static float yScale = 1.0f;
static float uScale = 1.0f;
static float vScale = 1.0f;

static float yiqYScale = 1.0f;
static float iScale = 1.0f;
static float qScale = 1.0f;

static float cScale = 1.0f;
static float mScale = 1.0f;
static float yCmyScale = 1.0f;

typedef enum {
    FALSE,
    TRUE
} boolean;

static boolean showHistogram = FALSE; 

typedef enum {
    BUTTON_IDLE,
    BUTTON_PRESSED
} MouseState;

typedef enum {
    DISPLAY_ARGB,
    DISPLAY_YUV,
    DISPLAY_YIQ,
    DISPLAY_CMY,
    DISPLAY_MONOCHROME,
    DISPLAY_DITHERED,
    DISPLAY_8BIT
} DisplayMode;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint32 *pixels;
    boolean programSuccess;
} API;

typedef struct {
    uint16_t x;
    uint16_t y;
} Point;

typedef struct {
    Point mouseLocation;
    MouseState _MouseButtonLeft;
    MouseState _MouseButtonRight;
} Mouse;

typedef struct {
    Point position;
    boolean checked;
} Checkbox;

static Checkbox checkboxes[7] = {
    {{(uint16_t)(SCREEN_WIDTH - 50), 20}, TRUE},  // ARGB (default)
    {{(uint16_t)(SCREEN_WIDTH - 50), 40}, FALSE}, // YUV
    {{(uint16_t)(SCREEN_WIDTH - 50), 60}, FALSE}, // YIQ
    {{(uint16_t)(SCREEN_WIDTH - 50), 80}, FALSE}, // CMY
    {{(uint16_t)(SCREEN_WIDTH - 50), 100}, FALSE}, // Monochrome
    {{(uint16_t)(SCREEN_WIDTH - 50), 120}, FALSE}, // Dithered
    {{(uint16_t)(SCREEN_WIDTH - 50), 140}, FALSE}  // 8-bit mode
};
static DisplayMode currentDisplay = DISPLAY_ARGB;

typedef struct
{
    Point position;
    char type;
} Button;
static Button buttons[26];

void initializeAPI(API *);
void disposeAPI(API *);
void iterativeFunction(API *);
void handleAPI(API *, Mouse);

int main(int argc, char *args[]) {
    API _API;
    _API.programSuccess = TRUE;
    initializeAPI(&_API);
    if (_API.programSuccess == TRUE) {
        iterativeFunction(&_API);
    }
    else {
        printf("Program Success has failed, SDL Error: %s\n", SDL_GetError());
    }
    disposeAPI(&_API);
    return 0;
}

void InitializeEightBitPalette() {
    for (int i = 0; i < 256; i++) {
        uint8_t r = (i & 0xE0);      
        uint8_t g = (i & 0x1C) << 3; 
        uint8_t b = (i & 0x03) << 6; 
        EightBitPalette[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
}

void initializeAPI(API *_API){
    InitializeEightBitPalette();
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Initialization has failed, SDL Error: %s\n", SDL_GetError());
        _API->programSuccess = FALSE;
        return;
    }
    _API->window = SDL_CreateWindow(
        "Multimedia Systems Project",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (_API->window == NULL) {
        printf("SDL Window Initialization has failed, SDL Error: %s\n", SDL_GetError());
        _API->programSuccess = FALSE;
        return;
    }
    _API->renderer = SDL_CreateRenderer(
        _API->window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    if (_API->renderer == NULL) {
        printf("SDL Renderer Initialization has failed, SDL Error: %s\n", SDL_GetError());
        _API->programSuccess = FALSE;
        return;
    }
    _API->texture = SDL_CreateTexture(
        _API->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    if (_API->texture == NULL) {
        printf("SDL Texture Initialization has failed, SDL Error: %s\n", SDL_GetError());
        _API->programSuccess = FALSE;
        return;
    }

    _API->pixels = (Uint32 *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Uint32));
    if (_API->pixels == NULL) {
        printf("Memory allocation for pixels failed.\n");
        _API->programSuccess = FALSE;
        return;
    }
    memset(_API->pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Uint32));
}

void disposeAPI(API *_API) {
    if (_API->pixels)
        free(_API->pixels);
    if (_API->texture)
        SDL_DestroyTexture(_API->texture);
    if (_API->renderer)
        SDL_DestroyRenderer(_API->renderer);
    if (_API->window)
        SDL_DestroyWindow(_API->window);
    SDL_Quit();
}

void adjustParameter(int index) {
    float step = 0.1f;
    int mode = -1;
    int component = -1;
    boolean increase = (index % 2) ? TRUE : FALSE;
    if (index >= 0 && index <= 7) { mode = 0; component = (index / 2); }  
    else if (index >= 8 && index <= 13) { mode = 1; component = ((index - 8) / 2); }  
    else if (index >= 14 && index <= 19) { mode = 2; component = ((index - 14) / 2); } 
    else if (index >= 20 && index <= 25) { mode = 3; component = ((index - 20) / 2); } 
    else { return; }
    float* target = NULL;
    const char* modeName = "";
    const char* componentName = "";
    if (mode == 0) {
        modeName = "ARGB";
        if (component == 0) { target = &alphaScale; componentName = "Alpha"; }
        else if (component == 1) { target = &redScale; componentName = "Red"; }
        else if (component == 2) { target = &greenScale; componentName = "Green"; }
        else { target = &blueScale; componentName = "Blue"; }
    } else if (mode == 1) {
        modeName = "YUV";
        if (component == 0) { target = &yScale; componentName = "Y"; }
        else if (component == 1) { target = &uScale; componentName = "U"; }
        else { target = &vScale; componentName = "V"; }
    } else if (mode == 2) {
        modeName = "YIQ";
        if (component == 0) { target = &yiqYScale; componentName = "Y"; }
        else if (component == 1) { target = &iScale; componentName = "I"; }
        else { target = &qScale; componentName = "Q"; }
    } else if (mode == 3) {
        modeName = "CMY";
        if (component == 0) { target = &cScale; componentName = "C"; }
        else if (component == 1) { target = &mScale; componentName = "M"; }
        else { target = &yCmyScale; componentName = "Y"; }
    } if (target) {
        float oldValue = *target;
        if (increase) {
            *target += step;
        } else {
            *target -= step;
        }
    }
}

void updateMouseState(Mouse *_Mouse, SDL_Event *event, Checkbox *checkboxes, DisplayMode *displayMode) {
    if (event->type == SDL_MOUSEMOTION) {
        _Mouse->mouseLocation.x = (uint16_t)event->motion.x;
        _Mouse->mouseLocation.y = (uint16_t)event->motion.y;
    } else if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            _Mouse->_MouseButtonLeft = BUTTON_PRESSED;
            for (int i = 0; i < 7; i++) {
                if (_Mouse->mouseLocation.x >= checkboxes[i].position.x &&
                    _Mouse->mouseLocation.x < checkboxes[i].position.x + 16 &&
                    _Mouse->mouseLocation.y >= checkboxes[i].position.y &&
                    _Mouse->mouseLocation.y < checkboxes[i].position.y + 16)
                {
                    for (int j = 0; j < 7; j++) {
                        checkboxes[j].checked = FALSE;
                    }
                    checkboxes[i].checked = TRUE;
                    *displayMode = (DisplayMode)i;
                }
            }
            for (int i = 0; i < 26; i++) {
                if (_Mouse->mouseLocation.x >= buttons[i].position.x &&
                    _Mouse->mouseLocation.x < buttons[i].position.x + 16 &&
                    _Mouse->mouseLocation.y >= buttons[i].position.y &&
                    _Mouse->mouseLocation.y < buttons[i].position.y + 16)
                {
                    adjustParameter(i);  
                }
            }
        }
    }
    else if (event->type == SDL_MOUSEBUTTONUP) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            _Mouse->_MouseButtonLeft = BUTTON_IDLE;
        }
    }
}

void imageMovement(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
        case SDLK_LEFT:
            imageOffsetX -= 10;
            break;
        case SDLK_RIGHT:
            imageOffsetX += 10;
            break;
        case SDLK_UP:
            imageOffsetY -= 10;
            break;
        case SDLK_DOWN:
            imageOffsetY += 10;
            break;
        case SDLK_z:
            if (imageZoom < 5.0f)
                imageZoom += 0.1f;
            break;
        case SDLK_x:
            if (imageZoom > 0.1f)
                imageZoom -= 0.1f;
            break;
        case SDLK_h: 
            if(showHistogram == TRUE) 
                showHistogram = FALSE;
            else
                showHistogram = TRUE;
            break;
        }
    }
}

void iterativeFunction(API *_API) {
    SDL_Event event;
    boolean quitRequest = FALSE;
    Mouse _Mouse = {{0, 0}, BUTTON_IDLE, BUTTON_IDLE};

    while (!quitRequest) {
        handleAPI(_API, _Mouse);
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quitRequest = TRUE;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                quitRequest = TRUE;
            } else {
                updateMouseState(&_Mouse, &event, checkboxes, &currentDisplay);
                imageMovement(&event);
            }
        }
    }
}

void drawMouse(API *_API, Mouse _Mouse) {
    uint32_t mouseColor = (_Mouse._MouseButtonLeft == BUTTON_PRESSED) ? 0xFFFF0000 : 0xFFFFFFFF;
    for (uint16_t y = _Mouse.mouseLocation.y - 3; y < _Mouse.mouseLocation.y + 3; y++) {
        if (y < 0 || y >= SCREEN_HEIGHT)
            continue;
        for (uint16_t x = _Mouse.mouseLocation.x - 3; x < _Mouse.mouseLocation.x + 3; x++) {
            if (x < 0 || x >= SCREEN_WIDTH)
                continue;
            _API->pixels[y * SCREEN_WIDTH + x] = mouseColor;
        }
    }
}

typedef struct {
    int width;
    int height;
    uint32_t *pixelArray;
} image;

image loadImage(const char *filePath) {
    SDL_Surface *imageSurface = SDL_LoadBMP(filePath);
    if (imageSurface == NULL) {
        printf("The image failed to load, SDL_ERROR: %s\n", SDL_GetError());
        return (image){0, 0, NULL};
    }
    uint16_t width = imageSurface->w;
    uint16_t height = imageSurface->h;

    uint32_t *pixelArray = (uint32_t *)malloc(width * height * sizeof(uint32_t));
    if (pixelArray == NULL) {
        printf("Memory allocation failed for the loaded image!\n");
        SDL_FreeSurface(imageSurface);
        return (image){0, 0, NULL};
    }

    Uint32 *pixelBuffer = (Uint32 *)imageSurface->pixels;
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++)
        {
            Uint32 pixel = pixelBuffer[y * width + x];
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, imageSurface->format, &r, &g, &b, &a);
            pixelArray[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    SDL_FreeSurface(imageSurface);
    return (image){width, height, pixelArray};
}

void applyImageMovement(uint32_t *pixels, image _image, Point point, uint32_t (*colorFunction)(uint32_t)) {
    int scaledWidth = (int)(_image.width * imageZoom);
    int scaledHeight = (int)(_image.height * imageZoom);

    for (int screenY = 0; screenY < scaledHeight; screenY++) {
        int finalY = point.y + screenY + imageOffsetY;
        if (finalY < 0)
            continue;
        if (finalY >= SCREEN_HEIGHT)
            break;
        for (int screenX = 0; screenX < scaledWidth; screenX++) {
            int finalX = point.x + screenX + imageOffsetX;
            if (finalX < 0)
                continue;
            if (finalX >= SCREEN_WIDTH / 2)
                continue;
            int srcX = (int)((screenX) / imageZoom);
            int srcY = (int)((screenY) / imageZoom);
            if (srcX >= 0 && srcX < _image.width && srcY >= 0 && srcY < _image.height) {
                uint32_t pixel = _image.pixelArray[srcY * _image.width + srcX];
                if (finalY * SCREEN_WIDTH + finalX < SCREEN_WIDTH * SCREEN_HEIGHT) {
                    pixels[finalY * SCREEN_WIDTH + finalX] = colorFunction(pixel);
                }
            }
        }
    }
}

uint32_t ARGBColor(uint32_t pixel) {
    uint8_t a = (pixel >> 24) & 0xFF;
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8) & 0xFF;
    uint8_t b = pixel & 0xFF;
    a = (uint8_t)(a * alphaScale);
    r = (uint8_t)(r * redScale);
    g = (uint8_t)(g * greenScale);
    b = (uint8_t)(b * blueScale);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t YUVColor(uint32_t pixel) {
    uint8_t a = (pixel >> 24) & 0xFF;
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8) & 0xFF;
    uint8_t b = pixel & 0xFF;
    uint8_t Y = (uint8_t)((0.299 * r + 0.587 * g + 0.114 * b) * yScale);
    uint8_t U = (uint8_t)((0.492 * (b - Y) + 128) * uScale);
    uint8_t V = (uint8_t)((0.877 * (r - Y) + 128) * vScale);
    return (a << 24) | (Y << 16) | (U << 8) | V;
}

uint32_t YIQColor(uint32_t pixel) {
    uint8_t a = (pixel >> 24) & 0xFF;
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8) & 0xFF;
    uint8_t b = pixel & 0xFF;
    uint8_t Y = (uint8_t)((0.299 * r + 0.587 * g + 0.114 * b) * yiqYScale);
    uint8_t I = (uint8_t)((0.596 * r - 0.275 * g - 0.321 * b + 128) * iScale);
    uint8_t Q = (uint8_t)((0.212 * r - 0.523 * g + 0.311 * b + 128) * qScale);
    return (a << 24) | (Y << 16) | (I << 8) | Q;
}

uint32_t CMYColor(uint32_t pixel) {
    uint8_t a = (pixel >> 24) & 0xFF;
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8) & 0xFF;
    uint8_t b = pixel & 0xFF;
    uint8_t C = 255 - r;
    uint8_t M = 255 - g;
    uint8_t Y = 255 - b;
    C = (uint8_t)(C * cScale);
    M = (uint8_t)(M * mScale);
    Y = (uint8_t)(Y * yCmyScale);
    r = 255 - C;
    g = 255 - M;
    b = 255 - Y;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t MonochromeColor(uint32_t pixel) {
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8) & 0xFF;
    uint8_t b = pixel & 0xFF;
    uint8_t Y = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
    return (Y > 128) ? 0xFFFFFFFF : 0xFF000000;
}

uint32_t *Dithered1BitColor(uint32_t *pixels, int width, int height) {
    int newWidth = width * 2;
    int newHeight = height * 2;
    uint32_t *ditheredPixels = (uint32_t *)malloc(newWidth * newHeight * sizeof(uint32_t));
    if (!ditheredPixels) {
        printf("Memory allocation failed for dithering.");
        return NULL;
    }
    float errorMatrix[height + 1][width + 1];
    memset(errorMatrix, 0, sizeof(errorMatrix));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            uint32_t pixel = pixels[index];
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            float gray = 0.299f * r + 0.587f * g + 0.114f * b + errorMatrix[y][x];
            uint8_t quantized = (gray >= 128) ? 255 : 0;
            float error = gray - quantized;
            uint32_t outputColor = (quantized == 255) ? 0xFFFFFFFF : 0xFF000000;
            ditheredPixels[(y * 2) * newWidth + (x * 2)] = outputColor;
            ditheredPixels[(y * 2) * newWidth + (x * 2 + 1)] = outputColor;
            ditheredPixels[(y * 2 + 1) * newWidth + (x * 2)] = outputColor;
            ditheredPixels[(y * 2 + 1) * newWidth + (x * 2 + 1)] = outputColor;
            if (x + 1 < width) errorMatrix[y][x + 1] += error * 7.0f / 16.0f;
            if (y + 1 < height && x > 0) errorMatrix[y + 1][x - 1] += error * 3.0f / 16.0f;
            if (y + 1 < height) errorMatrix[y + 1][x] += error * 5.0f / 16.0f;
            if (y + 1 < height && x + 1 < width) errorMatrix[y + 1][x + 1] += error * 1.0f / 16.0f;
        }
    }
    return ditheredPixels;
}

uint32_t EightBitColor(uint32_t pixel) {
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8) & 0xFF;
    uint8_t b = pixel & 0xFF;
    uint8_t index = ((r & 0xE0) >> 5) << 5 | ((g & 0xE0) >> 5) << 2 | ((b & 0xC0) >> 6);
    return EightBitPalette[index];
}

void displayImageInARGB(API *_API, image _image, Point point) {
    applyImageMovement(_API->pixels, _image, point, ARGBColor);
}

void displayImageInYUV(API *_API, image _image, Point point) {
    applyImageMovement(_API->pixels, _image, point, YUVColor);
}

void displayImageInYIQ(API *_API, image _image, Point point) {
    applyImageMovement(_API->pixels, _image, point, YIQColor);
}

void displayImageInCMY(API *_API, image _image, Point point) {
    applyImageMovement(_API->pixels, _image, point, CMYColor);
}

void displayImageInMonochrome(API *_API, image _image, Point point) {
    applyImageMovement(_API->pixels, _image, point, MonochromeColor);
}

void displayImageInDithered1Bit(API *_API, image _image, Point point) {
    uint32_t *ditheredPixels = Dithered1BitColor(_image.pixelArray, _image.width, _image.height);
    if (ditheredPixels) {
        image ditheredImage = {_image.width * 2, _image.height * 2, ditheredPixels};
        applyImageMovement(_API->pixels, ditheredImage, point, ARGBColor); 
        free(ditheredPixels);
    }
}

void displayImageIn8Bit(API *_API, image _image, Point point) {
    applyImageMovement(_API->pixels, _image, point, EightBitColor);
}

void computeHistogram(image img, int histogram[256]) {
    memset(histogram, 0, sizeof(int) * 256);
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            uint32_t pixel = img.pixelArray[y * img.width + x];
            uint8_t intensity = 0;
            switch (currentDisplay) {
            case DISPLAY_ARGB: {
                    uint8_t r = (uint8_t)(((pixel >> 16) & 0xFF) * redScale);
                    uint8_t g = (uint8_t)(((pixel >> 8) & 0xFF) * greenScale);
                    uint8_t b = (uint8_t)((pixel & 0xFF) * blueScale);
                    intensity = (r + g + b) / 3;
                }
                break;
            case DISPLAY_YUV: {
                    uint8_t r = (uint8_t)(((pixel >> 16) & 0xFF) * redScale);
                    uint8_t g = (uint8_t)(((pixel >> 8) & 0xFF) * greenScale);
                    uint8_t b = (uint8_t)((pixel & 0xFF) * blueScale);
                    intensity = (uint8_t)((0.299 * r + 0.587 * g + 0.114 * b) * yScale);
                }
                break;
            case DISPLAY_YIQ: {
                    uint8_t r = (uint8_t)(((pixel >> 16) & 0xFF) * redScale);
                    uint8_t g = (uint8_t)(((pixel >> 8) & 0xFF) * greenScale);
                    uint8_t b = (uint8_t)((pixel & 0xFF) * blueScale);
                    intensity = (uint8_t)((0.299 * r + 0.587 * g + 0.114 * b) * yiqYScale);
                }
                break;
            case DISPLAY_CMY: {
                    uint8_t r = (uint8_t)(((pixel >> 16) & 0xFF) * redScale);
                    uint8_t g = (uint8_t)(((pixel >> 8) & 0xFF) * greenScale);
                    uint8_t b = (uint8_t)((pixel & 0xFF) * blueScale);
                    intensity = (uint8_t)((255 - r + 255 - g + 255 - b) / 3);
                }
                break;
            case DISPLAY_MONOCHROME:
            case DISPLAY_DITHERED:
                intensity = (uint8_t)((pixel & 0xFF) * yScale);
                break;
            case DISPLAY_8BIT:
                intensity = (pixel & 0xFF);
                break;
            }
            histogram[intensity]++;
        }
    }
}

void drawNumber(API *_API, image numbers, Point start, const char *text) {
    if (numbers.pixelArray == NULL) {
        printf("Number image not loaded.\n");
        return;
    }
    int charWidth = numbers.width / 10;
    int charHeight = numbers.height;
    int spaceWidth = charWidth * 1.25;
    Point position = start;
    for (const char *c = text; *c != '\0'; c++) {
        if (*c == ' ') {
            position.x += spaceWidth;
            continue;
        }
        if (*c >= '0' && *c <= '9') {
            int index = *c - '0';
            int rightMostPixel = 0;
            for (uint16_t y = 0; y < charHeight; y++) {
                for (uint16_t x = 0; x < charWidth; x++) {
                    uint32_t screenX = position.x + x;
                    uint32_t screenY = position.y + y;
                    uint32_t srcX = index * charWidth + x;
                    uint32_t srcY = y;
                    uint32_t pixel = numbers.pixelArray[srcY * numbers.width + srcX];
                    uint8_t r = (pixel >> 16) & 0xFF;
                    uint8_t g = (pixel >> 8) & 0xFF;
                    uint8_t b = pixel & 0xFF;
                    if (r > 200 && g > 200 && b > 200) {
                        continue;
                    }
                    if (screenX < SCREEN_WIDTH && screenY < SCREEN_HEIGHT) {
                        _API->pixels[screenY * SCREEN_WIDTH + screenX] = 0xFF000000;
                        if (x > rightMostPixel) {
                            rightMostPixel = x;
                        }
                    }
                }
            }
            position.x += rightMostPixel + 2;
        }
    }
}

void drawText(API *_API, image alphabet, Point start, const char *text) {
    if (alphabet.pixelArray == NULL) {
        printf("Alphabet image not loaded.\n");
        return;
    }
    int charWidth = alphabet.width / 26;
    int charHeight = alphabet.height;
    int spaceWidth = charWidth * 1.25;
    Point position = start;
    for (const char *c = text; *c != '\0'; c++) {
        if (*c == ' ') {
            position.x += spaceWidth;
            continue;
        } if (*c >= 'a' && *c <= 'z') {
            int index = *c - 'a';
            int rightMostPixel = 0;
            for (uint16_t y = 0; y < charHeight; y++) {
                for (uint16_t x = 0; x < charWidth; x++) {
                    uint32_t screenX = position.x + x;
                    uint32_t screenY = position.y + y;
                    uint32_t srcX = index * charWidth + x;
                    uint32_t srcY = y;
                    uint32_t pixel = alphabet.pixelArray[srcY * alphabet.width + srcX];
                    uint8_t r = (pixel >> 16) & 0xFF;
                    uint8_t g = (pixel >> 8) & 0xFF;
                    uint8_t b = pixel & 0xFF;
                    if (r > 200 && g > 200 && b > 200) {
                        continue;
                    } if (screenX < SCREEN_WIDTH && screenY < SCREEN_HEIGHT) {
                        _API->pixels[screenY * SCREEN_WIDTH + screenX] = 0xFF000000;
                        if (x > rightMostPixel) {
                            rightMostPixel = x;
                        }
                    }
                }
            }
            position.x += rightMostPixel + 2;
        }
    }
}

void drawHistogram(API *_API, int histogram[256], image numbers) {
    int histX = 40;
    int histY = 80;
    int histWidth = SCREEN_WIDTH - 60;
    int histHeight = 160;
    int maxFrequency = 1;
    for (int i = 0; i < 256; i++) {
        if (histogram[i] > maxFrequency)
            maxFrequency = histogram[i];
    }
    for (int y = histY - 30; y < histY + histHeight + 40; y++) {
        for (int x = histX - 30; x < histX + histWidth + 30; x++) {
            _API->pixels[y * SCREEN_WIDTH + x] = 0xFFFFFFFF;  
        }
    }
    for (int i = 0; i <= 255; i += 32) {
        int gridX = histX + (i * histWidth / 256);
        for (int y = histY; y < histY + histHeight; y++) {
            _API->pixels[y * SCREEN_WIDTH + gridX] = 0xFFAAAAAA; 
        }
    }
    for (int i = 0; i <= 4; i++) {
        int gridY = histY + histHeight - (i * histHeight / 4);
        for (int x = histX; x < histX + histWidth; x++) {
            _API->pixels[gridY * SCREEN_WIDTH + x] = 0xFFAAAAAA;
        }
    }
    int barWidth = (histWidth / 256) - 1;
    for (int i = 0; i < 256; i++) {
        int barHeight = (histogram[i] * histHeight) / maxFrequency;
        for (int y = histY + histHeight - barHeight; y < histY + histHeight - 1; y++) {
            for (int x = histX + i * (barWidth + 1); x < histX + i * (barWidth + 1) + barWidth; x++) {
                _API->pixels[y * SCREEN_WIDTH + x] = 0xFFFF0000;
            }
        }
    }
    for (int i = 0; i <= 255; i += 64) {
        char label[4];
        sprintf(label, "%d", i);  
        int xPos = histX + (i * histWidth / 256) - 10;
        int yPos = histY + histHeight + 12; 
        Point textPos = {(uint16_t)xPos, (uint16_t)yPos};
        drawNumber(_API, numbers, textPos, label);
    }
    for (int i = 0; i <= 4; i++) {
        char label[5];
        int value = (maxFrequency * i) / 4;  
        sprintf(label, "%d", value);  
        int xPos = histX - 25;
        int yPos = histY + histHeight - (i * histHeight / 4) - 5;
        Point textPos = {(uint16_t)xPos, (uint16_t)yPos};
        drawNumber(_API, numbers, textPos, label);
    }
}
    
void drawCheckbox(API *_API, Checkbox checkbox) {
    uint32_t boxColor = checkbox.checked ? 0xFF00AA00 : 0xFFFFFFFF;
    uint32_t shadowColor = 0xFF404040;
    int boxSize = 16;
    for (int y = 2; y < boxSize + 2; y++) {
        for (int x = 2; x < boxSize + 2; x++) {
            uint32_t screenX = checkbox.position.x + x;
            uint32_t screenY = checkbox.position.y + y;
            if (screenX < SCREEN_WIDTH && screenY < SCREEN_HEIGHT) {
                _API->pixels[screenY * SCREEN_WIDTH + screenX] = shadowColor;
            }
        }
    }
    for (int y = 0; y < boxSize; y++) {
        for (int x = 0; x < boxSize; x++) {
            uint32_t screenX = checkbox.position.x + x;
            uint32_t screenY = checkbox.position.y + y;
            if ((x == 0 && y == 0) || (x == 0 && y == boxSize - 1) || (x == boxSize - 1 && y == 0) || (x == boxSize - 1 && y == boxSize - 1))
                continue; 
            if (screenX < SCREEN_WIDTH && screenY < SCREEN_HEIGHT) {
                _API->pixels[screenY * SCREEN_WIDTH + screenX] = boxColor;
            }
        }
    }
    if (checkbox.checked) {
        for (int i = 3; i < boxSize - 4; i++) {
            _API->pixels[(checkbox.position.y + i) * SCREEN_WIDTH + (checkbox.position.x + i)] = 0xFF000000;
            _API->pixels[(checkbox.position.y + i) * SCREEN_WIDTH + (checkbox.position.x + boxSize - i - 1)] = 0xFF000000;
        }
    }
}

void drawButton(API *_API, Button button) {
    uint32_t boxColor = 0xFFDDDDDD;    
    uint32_t borderColor = 0xFFAAAAAA; 
    uint32_t symbolColor = 0xFF000000; 
    uint32_t shadowColor = 0xFF505050;
    int boxSize = 16;
    for (int y = 2; y < boxSize + 2; y++) {
        for (int x = 2; x < boxSize + 2; x++) {
            uint32_t screenX = button.position.x + x;
            uint32_t screenY = button.position.y + y;
            if (screenX < SCREEN_WIDTH && screenY < SCREEN_HEIGHT) {
                _API->pixels[screenY * SCREEN_WIDTH + screenX] = shadowColor;
            }
        }
    }
    for (int y = 0; y < boxSize; y++) {
        for (int x = 0; x < boxSize; x++) {
            uint32_t screenX = button.position.x + x;
            uint32_t screenY = button.position.y + y;
            if (screenX < SCREEN_WIDTH && screenY < SCREEN_HEIGHT) {
                if ((x == 0 && y == 0) || (x == 0 && y == boxSize - 1) || (x == boxSize - 1 && y == 0) || (x == boxSize - 1 && y == boxSize - 1))
                    continue;
                if (x == 0 || x == boxSize - 1 || y == 0 || y == boxSize - 1) {
                    _API->pixels[screenY * SCREEN_WIDTH + screenX] = borderColor;
                }
                else {
                    _API->pixels[screenY * SCREEN_WIDTH + screenX] = boxColor;
                }
            }
        }
    }
    int centerX = button.position.x + boxSize / 2;
    int centerY = button.position.y + boxSize / 2;
    if (button.type == '+') {
        for (int i = -4; i <= 4; i++) {
            _API->pixels[(centerY + i) * SCREEN_WIDTH + centerX] = symbolColor;
            _API->pixels[centerY * SCREEN_WIDTH + (centerX + i)] = symbolColor;
        }
    } else if (button.type == '-') {
        for (int i = -4; i <= 4; i++) {
            _API->pixels[centerY * SCREEN_WIDTH + (centerX + i)] = symbolColor;
        }
    }
}

void drawUI(API *_API, image alphabet) {
    const int NUM_MODES = 7; 
    const int NUM_COMPONENTS[NUM_MODES] = {4, 3, 3, 3, 0, 0, 0}; 
    const int MARGIN = 8;  
    const int BUTTON_SIZE = 16;
    const int TEXT_OFFSET_Y = 5;
    int startX = SCREEN_WIDTH / 2 + MARGIN;
    int cursorY = MARGIN; 
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = SCREEN_WIDTH / 2; x < SCREEN_WIDTH; x++) {
            _API->pixels[y * SCREEN_WIDTH + x] = 0xFF505050;
        }
    }
    Point titlePosition = {(uint16_t)(startX + 20), (uint16_t)(cursorY + TEXT_OFFSET_Y)};
    drawText(_API, alphabet, titlePosition, "image display modes");
    cursorY += 24;
    const char *modeLabels[NUM_MODES] = {
        "argb mode", "yuv mode", "yiq mode", "cmy mode", "monochrome mode", "dithered mode", "eight bit mode"};
    const char *componentLabels[NUM_MODES][4] = {
        {"alpha component", "red component", "green component", "blue component"},
        {"y component", "u component", "v component"},
        {"y component", "i component", "q component"},
        {"c component", "m component", "y component"},
        {""}, 
        {""},  
        {""}  
    };
    int buttonIndex = 0;
    for (int i = 0; i < NUM_MODES; i++) {
        cursorY += MARGIN;
        if (cursorY + 20 >= SCREEN_HEIGHT) break;
        uint32_t highlightColor = (checkboxes[i].checked) ? 0xFF77AAFF : 0xFF606060;
        for (int y = cursorY; y < cursorY + 18; y++) {
            for (int x = startX; x < SCREEN_WIDTH - MARGIN; x++) {
                _API->pixels[y * SCREEN_WIDTH + x] = highlightColor;
            }
        }
        checkboxes[i].position.x = startX + MARGIN;
        checkboxes[i].position.y = cursorY + 3;
        drawCheckbox(_API, checkboxes[i]);
        Point textPosition = {(uint16_t)(checkboxes[i].position.x + 25), (uint16_t)(cursorY + TEXT_OFFSET_Y)};
        drawText(_API, alphabet, textPosition, modeLabels[i]);
        cursorY += 20;
        if (cursorY >= SCREEN_HEIGHT) break;
        for (int j = 0; j < NUM_COMPONENTS[i]; j++) {
            if (cursorY + 18 >= SCREEN_HEIGHT) break;
            int minusButtonX = startX + MARGIN;
            int plusButtonX = minusButtonX + BUTTON_SIZE + MARGIN;
            int textX = plusButtonX + BUTTON_SIZE + MARGIN;
            buttons[buttonIndex].position.x = minusButtonX;
            buttons[buttonIndex].position.y = cursorY + 3;
            buttons[buttonIndex].type = '-';
            drawButton(_API, buttons[buttonIndex]);
            buttonIndex++;
            buttons[buttonIndex].position.x = plusButtonX;
            buttons[buttonIndex].position.y = cursorY + 3;
            buttons[buttonIndex].type = '+';
            drawButton(_API, buttons[buttonIndex]);
            buttonIndex++;
            Point compTextPos = {(uint16_t)textX, (uint16_t)(cursorY + TEXT_OFFSET_Y)};
            drawText(_API, alphabet, compTextPos, componentLabels[i][j]);
            cursorY += 18;
            if (cursorY >= SCREEN_HEIGHT) break;
        }
        cursorY += MARGIN;
    }
}

void handleAPI(API *_API, Mouse _Mouse) {
    memset(_API->pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Uint32));
    image image1 = loadImage("images\\FELV-cat.bmp");
    image alphabet = loadImage("images\\alphabet_revised.bmp");
    image numbers = loadImage("images\\numbers.bmp");
    if (image1.pixelArray) {
        Point point = {10, 10};
        switch (currentDisplay) {
        case DISPLAY_ARGB:
            displayImageInARGB(_API, image1, point);
            break;
        case DISPLAY_YUV:
            displayImageInYUV(_API, image1, point);
            break;
        case DISPLAY_YIQ:
            displayImageInYIQ(_API, image1, point);
            break;
        case DISPLAY_CMY:
            displayImageInCMY(_API, image1, point);
            break;
        case DISPLAY_MONOCHROME:
            displayImageInMonochrome(_API, image1, point);
            break;
        case DISPLAY_DITHERED:
            displayImageInDithered1Bit(_API, image1, point);
            break;
        case DISPLAY_8BIT:
            displayImageIn8Bit(_API, image1, point);
            break;
        }
    }
    drawUI(_API, alphabet);
    if (showHistogram) {
        int histogram[256] = {0};
        computeHistogram(image1, histogram);
        drawHistogram(_API, histogram, numbers);
    }
    drawMouse(_API, _Mouse);
    SDL_UpdateTexture(_API->texture, NULL, _API->pixels, SCREEN_WIDTH * sizeof(Uint32));
    SDL_RenderClear(_API->renderer);
    SDL_RenderCopy(_API->renderer, _API->texture, NULL, NULL);
    SDL_RenderPresent(_API->renderer);
    if (image1.pixelArray)
        free(image1.pixelArray);
}
