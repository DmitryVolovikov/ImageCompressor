#include "image_transforms.h"
#include "error_handlers.h"
#include "images.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

static void rotate90(UncompressedImage& img, ColorRGB fill_color, bool smart_gap_interpolation) {
    uint32_t original_width = img.getWidth();
    uint32_t original_height = img.getHeight();

    std::vector<Pixel> rotated_pixels(original_width * original_height, Pixel{fill_color.r, fill_color.g, fill_color.b});
    uint32_t new_width = original_height;
    uint32_t new_height = original_width;

    for (uint32_t y = 0; y < original_height; ++y) {
        for (uint32_t x = 0; x < original_width; ++x) {
            uint32_t new_x = original_height - 1 - y;
            uint32_t new_y = x;
            rotated_pixels[new_y * new_width + new_x] = img.getPixel(x, y);
        }
    }

    img.setWidth(new_width);
    img.setHeight(new_height);
    img.setPixels(rotated_pixels);

    if (smart_gap_interpolation) {
    }
}

void rotate(UncompressedImage& img, int angle, ColorRGB fill_color, bool smart_gap_interpolation) {
    angle = angle % 360;
    if (angle < 0) angle += 360;

    if (angle % 90 != 0) {
        handleLogMessage("Вращение на произвольный угол не поддерживается. Пожалуйста, используйте кратные 90 градусов.", Severity::WARNING);
        return;
    }

    int rotations = angle / 90;
    for (int i = 0; i < rotations; ++i) {
        rotate90(img, fill_color, smart_gap_interpolation);
    }

    handleLogMessage("Вращение изображения выполнено на " + std::to_string(angle) + " градусов.", Severity::INFO);
}

void applyKernel(UncompressedImage& img, const std::vector<std::vector<int>>& kernel, int divisor) {
    if (kernel.empty() || kernel.size() != kernel[0].size() || kernel.size() % 2 == 0) {
        handleLogMessage("Некорректный размер ядра. Ядро должно быть квадратным и иметь нечётный размер.", Severity::ERROR, 1);
        return;
    }

    int kernel_size = kernel.size();
    int offset = kernel_size / 2;

    uint32_t width = img.getWidth();
    uint32_t height = img.getHeight();
    std::vector<Pixel> original_pixels = img.getPixels();
    std::vector<Pixel> new_pixels(width * height, Pixel{0, 0, 0});

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            int sum_r = 0, sum_g = 0, sum_b = 0;

            for (int ky = 0; ky < kernel_size; ++ky) {
                for (int kx = 0; kx < kernel_size; ++kx) {
                    int ix = x + kx - offset;
                    int iy = y + ky - offset;

                    ix = std::clamp(ix, 0, static_cast<int>(width) - 1);
                    iy = std::clamp(iy, 0, static_cast<int>(height) - 1);

                    Pixel p = original_pixels[iy * width + ix];
                    sum_r += p.r * kernel[ky][kx];
                    sum_g += p.g * kernel[ky][kx];
                    sum_b += p.b * kernel[ky][kx];
                }
            }

            sum_r = std::clamp(sum_r / divisor, 0, 255);
            sum_g = std::clamp(sum_g / divisor, 0, 255);
            sum_b = std::clamp(sum_b / divisor, 0, 255);

            new_pixels[y * width + x] = Pixel{static_cast<uint8_t>(sum_r), static_cast<uint8_t>(sum_g), static_cast<uint8_t>(sum_b)};
        }
    }

    img.setPixels(new_pixels);
    handleLogMessage("Применение ядра фильтра выполнено.", Severity::INFO);
}

void sharpen(UncompressedImage& img) {
    std::vector<std::vector<int>> sharpen_kernel = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };
    applyKernel(img, sharpen_kernel, 1);
    handleLogMessage("Фильтр резкости применён.", Severity::INFO);
}

void gaussianBlurApprox(UncompressedImage& img, bool hard_blur) {
    std::vector<std::vector<int>> gaussian_kernel;
    int divisor = 1;

    if (!hard_blur) {
        gaussian_kernel = {
            {1, 2, 1},
            {2, 4, 2},
            {1, 2, 1}
        };
        divisor = 16;
    } else {
        gaussian_kernel = {
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1}
        };
        divisor = 9;
    }

    applyKernel(img, gaussian_kernel, divisor);
    handleLogMessage("Гауссово размытие применено.", Severity::INFO);
}

void edgeDetect(UncompressedImage& img) {
    std::vector<std::vector<int>> edge_kernel = {
        {-1, -1, -1},
        {-1,  8, -1},
        {-1, -1, -1}
    };
    applyKernel(img, edge_kernel, 1);
    handleLogMessage("Обнаружение краёв выполнено.", Severity::INFO);
}

void negative(UncompressedImage& img) {
    std::vector<Pixel> pixels = img.getPixels();
    for (auto& pixel : pixels) {
        pixel.r = 255 - pixel.r;
        pixel.g = 255 - pixel.g;
        pixel.b = 255 - pixel.b;
    }
    img.setPixels(pixels);
    handleLogMessage("Инверсия цветов (UncompressedImage) выполнена.", Severity::INFO);
}

void negative(CompressedImage& img) {
    std::map<uint8_t, ColorRGB> colorTable = img.getColorTable();
    for (auto& [id, color] : colorTable) {
        color.r = 255 - color.r;
        color.g = 255 - color.g;
        color.b = 255 - color.b;
    }
    img.setColorTable(colorTable);
    handleLogMessage("Инверсия цветов (CompressedImage) выполнена.", Severity::INFO);
}

void toGrayscale(UncompressedImage& img) {
    if (img.isGrayscale()) {
        handleLogMessage("Изображение уже в градациях серого.", Severity::INFO);
        return;
    }

    std::vector<Pixel> pixels = img.getPixels();
    for (auto& pixel : pixels) {
        uint8_t gray = colorToGrayscale(pixel);
        pixel.r = pixel.g = pixel.b = gray;
    }
    img.setPixels(pixels);
    img.setGrayscale(true);
    handleLogMessage("Преобразование в градации серого (UncompressedImage) выполнено.", Severity::INFO);
}

void toGrayscale(CompressedImage& img) {
    std::map<uint8_t, ColorRGB> colorTable = img.getColorTable();
    for (auto& [id, color] : colorTable) {
        uint8_t gray = colorToGrayscale(color);
        color.r = color.g = color.b = gray;
    }
    img.setColorTable(colorTable);
    handleLogMessage("Преобразование в градации серого (CompressedImage) выполнено.", Severity::INFO);
}

template <typename Image>
void mirror(Image& img, bool horizontal) {
    uint32_t width = img.getWidth();
    uint32_t height = img.getHeight();

    std::vector<Pixel> pixels = img.getPixels();

    if (horizontal) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width / 2; ++x) {
                std::swap(pixels[y * width + x], pixels[y * width + (width - 1 - x)]);
            }
        }
        handleLogMessage("Зеркальное отражение по горизонтали выполнено.", Severity::INFO);
    } else {
        for (uint32_t y = 0; y < height / 2; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                std::swap(pixels[y * width + x], pixels[(height - 1 - y) * width + x]);
            }
        }
        handleLogMessage("Зеркальное отражение по вертикали выполнено.", Severity::INFO);
    }

    img.setPixels(pixels);
}

template void mirror(UncompressedImage& img, bool horizontal);

template void mirror(CompressedImage& img, bool horizontal);
