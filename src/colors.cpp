#include "colors.h"
#include <iostream>
#include <cmath>

uint8_t colorToGrayscale(const ColorRGB& color) {
    double grayValue = 0.299 * static_cast<double>(color.r) +
                       0.587 * static_cast<double>(color.g) +
                       0.114 * static_cast<double>(color.b);
    return static_cast<uint8_t>(std::round(grayValue));
   
}

ColorRGB readFromFileStream(std::fstream& stream) {
    ColorRGB color;
    stream.read(reinterpret_cast<char*>(&color.r), sizeof(uint8_t));
    stream.read(reinterpret_cast<char*>(&color.g), sizeof(uint8_t));
    stream.read(reinterpret_cast<char*>(&color.b), sizeof(uint8_t));

    if (stream.fail()) {
        std::cerr << "Ошибка при чтении цвета из потока.\n";
        return ColorRGB{0, 0, 0};
    }

    return color;
   
    return {};
}

int64_t colorDistanceSq(const ColorRGB& color1, const ColorRGB& color2) {
    int64_t dr = static_cast<int64_t>(color1.r) - static_cast<int64_t>(color2.r);
    int64_t dg = static_cast<int64_t>(color1.g) - static_cast<int64_t>(color2.g);
    int64_t db = static_cast<int64_t>(color1.b) - static_cast<int64_t>(color2.b);
    return dr * dr + dg * dg + db * db;
}
