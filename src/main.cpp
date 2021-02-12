#include <Arduino.h>

#include <TFT_eSPI.h> 
#include <Button2.h>

#include <tuple>

constexpr struct {
    uint8_t button_1 = 35;
    uint8_t button_2 = 0;
} pins;

auto button_1 = Button2{ pins.button_1 };
auto button_2 = Button2{ pins.button_2 };

union color
{
    uint8_t arr[4];
    uint32_t u32;
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    } vec;
};

constexpr auto display_width = 240;
constexpr auto display_height = 135;
constexpr auto display_pixels = display_width * display_height;

auto tft = TFT_eSPI{ display_height, display_width };

// When i say "pixel" from here on out i mean something in beetween
// [0, display_pixels] that has a color, a screen position and a real-plane position.

// Represents some coordinate on the real plane.
using real_coord = std::pair<double, double>;
// Represents some coordinate on the screen.
using screen_coord = std::pair<uint8_t, uint8_t>;

// Maps a pixel (0, 1, 2, 3, 4...) to a screen position ({0, 0}, {0, 1}...).
// This is largely static.
screen_coord pixel_xy[display_width * display_height];

/// These change when you move the view or change zoom, respectively.
// The center of the screen, in the real-plane.
auto real_center = real_coord{0, 0};
// The distance between each pixel, in the real-plane.
auto real_pixeldist = 0.015;

auto mandelbrot_iterations = 20;

// Maps a pixel to real-plane coordinates.
inline real_coord real_pixel_coord(uint8_t x, uint8_t y)
{
    // Purposefully switch coordinates, since we want to display in landscape mode.
    return real_coord{
        -(real_center.first + real_pixeldist * ((double)(display_width)/2  - y)),
        real_center.second  + real_pixeldist * ((double)(display_height)/2 - x)
    };
}

void zoom_in(Button2&)
{
    real_pixeldist *= 0.5;
    mandelbrot_iterations *= 1.3;
}
void zoom_out(Button2&)
{
    real_pixeldist /= 0.5;
    mandelbrot_iterations /= 1.3;
}
void move_right(Button2&)
{
    real_center.first += real_pixeldist * 50;
}
void move_left(Button2&)
{
    real_center.first -= real_pixeldist * 50;
}
void move_up(Button2&)
{
    real_center.second -= real_pixeldist * 50;
}
void move_down(Button2&)
{
    real_center.second += real_pixeldist * 50;
}

void set_pixel_xy_to_scanline(bool draw = false)
{
    auto display_color = color{};
    display_color.vec.r = 255;
    display_color.vec.g = 255;
    display_color.vec.b = 255;

    for(auto pixel = 0; pixel < display_pixels; ++pixel)
    {
        auto& coords = pixel_xy[pixel];
        coords.first  = pixel / display_width;
        coords.second = pixel % display_width;

        if(draw) tft.drawPixel(coords.first, coords.second, display_color.u32);
    }
}

inline int mandelbrot(real_coord z, real_coord c, int max_iterations)
{
    auto iterations = 0;

    while((z.first * z.first + z.second * z.second) < 4 && iterations < max_iterations)
    {
        const auto _z = z;

        // z = z*z + c, where z and c are complex numbers
        // z = (a+b*i)(a+b*i) + c
        // z = a*a + 2*a*b*i + b*b*i*i + c
        // z = a*a + 2*a*b*i - b*b + c
        // zr = a*a - b*b + cr
        z.first = _z.first * _z.first - _z.second * _z.second     + c.first;
        // zi = 2*a*b*i + ci
        z.second = 2 * _z.first * _z.second                       + c.second;

        ++iterations;
    }

    return iterations;
}

auto white = color{};
auto black = color{};

void setup()
{
    Serial.begin(115200);

    tft.init();
    tft.fillScreen(TFT_BLACK);

    set_pixel_xy_to_scanline();

    white.u32 = 0xffffffff;
    black.u32 = 0;

    button_1.setClickHandler(&move_up);
    button_1.setLongClickHandler(&move_right);
    button_1.setDoubleClickHandler(&zoom_in);
    button_2.setClickHandler(&move_down);
    button_2.setLongClickHandler(&move_left);
    button_2.setDoubleClickHandler(&zoom_out);
}

constexpr auto mandelbrot_z = real_coord{0, 0};

auto pixel = 0;

void loop()
{
    button_1.loop();
    button_2.loop();

    auto start = millis();
    while(start - millis() < 5)
    {
        const auto screen_coords = pixel_xy[pixel];

        const auto real_coords = real_pixel_coord(screen_coords.first, screen_coords.second);

        if(mandelbrot(mandelbrot_z, real_coords, mandelbrot_iterations) == mandelbrot_iterations) {
            tft.drawPixel(screen_coords.first, screen_coords.second, black.u32);
        } else {
            tft.drawPixel(screen_coords.first, screen_coords.second, white.u32);
        }

        pixel = (pixel + 1) % display_pixels;
    }
}
