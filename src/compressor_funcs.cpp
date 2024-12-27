#include "compressor_funcs.h"
#include "error_handlers.h"
#include "libbmp.h"
#include "images.h"
#include <cmath>
#include <limits>
#include <iostream>
/*
* Implement all the functions declared in the header file here.
* Use the BMP class from libbmp.h to save and load BMP files.

* These function will read, write, convert and compress images across different formats.
*/

void saveAsBMP(const UncompressedImage& img, const std::string& filename) {
    BMP bmp(img.getWidth(), img.getHeight());

    for (uint32_t y = 0; y < img.getHeight(); ++y) {
        for (uint32_t x = 0; x < img.getWidth(); ++x) {
            Pixel pixel = img.getPixel(x, y);
            if (img.isGrayscale()) {
                uint8_t gray = colorToGrayscale(pixel);
                bmp.set_pixel(x, y, gray, gray, gray);
            } else {
                bmp.set_pixel(x, y, pixel.r, pixel.g, pixel.b);
            }
        }
    }

    if (!bmp.write(filename.c_str())) {
        std::cerr << "Не удалось сохранить BMP файл: " << filename << std::endl;
    }
}

UncompressedImage loadFromBMP(const std::string& filename) {
    BMP bmp;
    if (!bmp.read(filename.c_str())) {
        std::cerr << "Не удалось загрузить BMP файл: " << filename << std::endl;
        return UncompressedImage(); 
    }

    UncompressedImage img;
    img.setWidth(bmp.get_width());
    img.setHeight(bmp.get_height());
    img.setGrayscale(false);

    std::vector<Pixel> pixels;
    pixels.reserve(img.getWidth() * img.getHeight());

    for (uint32_t y = 0; y < img.getHeight(); ++y) {
        for (uint32_t x = 0; x < img.getWidth(); ++x) {
            uint8_t r, g, b;
            bmp.get_pixel(x, y, r, g, b);
            pixels.emplace_back(Pixel{r, g, b});
        }
    }

    img.setPixels(pixels);

    return img;
    return {};
}

UncompressedImage readUncompressedFile(const std::string& filename) {
    UncompressedImage img;
    if (!img.readFromFile(filename)) {
        std::cerr << "Не удалось прочитать UncompressedImage файл: " << filename << std::endl;
    }
    return img;
    return {};
}

void writeUncompressedFile(const std::string& filename, const UncompressedImage& image) {
    if (!image.writeToFile(filename)) {
        std::cerr << "Не удалось записать UncompressedImage файл: " << filename << std::endl;
    }
}

uint8_t findClosestColorId(const ColorRGB& color, const std::map<uint8_t, ColorRGB>& colorTable) {
    if (colorTable.empty()) {
        std::cerr << "Таблица цветов пуста.\n";
        return 0;
    }
    uint8_t closestId = 0;
    int64_t minDistance = std::numeric_limits<int64_t>::max();

    for (const auto& [id, tableColor] : colorTable) {
        int64_t distance = colorDistanceSq(color, tableColor);
        if (distance < minDistance) {
            minDistance = distance;
            closestId = id;
        }
    }

    return closestId;
}

CompressedImage toCompressed(
    const UncompressedImage& img, const std::map<uint8_t, ColorRGB>& color_table, bool approximate,
    bool allow_color_add) {

    CompressedImage cImg;
    cImg.setWidth(img.getWidth());
    cImg.setHeight(img.getHeight());
    cImg.setGrayscale(img.isGrayscale());

    std::map<uint8_t, ColorRGB> table = color_table;

    if (table.empty()) {
        table = std::map<uint8_t, ColorRGB>();
        uint8_t currentId = 0;
        for (const auto& pixel : img.getPixels()) {
            bool exists = false;
            for (const auto& [id, tableColor] : table) {
                if (tableColor == pixel) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                if (currentId >= 256) {
                    std::cerr << "Таблица цветов переполнена. Максимум 256 цветов.\n";
                    break;
                }
                table[currentId++] = pixel;
            }
        }
    }

    cImg.setColorTable(table);

    std::vector<uint8_t> pixelIds;
    pixelIds.reserve(img.getWidth() * img.getHeight());

    for (const auto& pixel : img.getPixels()) {
        uint8_t id = findClosestColorId(pixel, table);
        pixelIds.push_back(id);
    }

    cImg.setPixelIds(pixelIds);

    return cImg;
}

UncompressedImage toUncompressed(const CompressedImage& img) {
    UncompressedImage uImg;
    uImg.setWidth(img.getWidth());
    uImg.setHeight(img.getHeight());
    uImg.setGrayscale(img.isGrayscale());

    const auto& colorTable = img.getColorTable();
    const auto& pixelIds = img.getPixelIds();

    std::vector<Pixel> pixels;
    pixels.reserve(pixelIds.size());

    for (const auto& id : pixelIds) {
        auto it = colorTable.find(id);
        if (it != colorTable.end()) {
            pixels.emplace_back(Pixel{it->second.r, it->second.g, it->second.b});
        } else {
            std::cerr << "ID цвета " << static_cast<int>(id) << " не найден в цветовой таблице.\n";
            pixels.emplace_back(Pixel{0, 0, 0}); 
        }
    }

    uImg.setPixels(pixels);

    return uImg;
}

ColorRGB getColor(const CompressedImage& img, int x, int y) {
    if (x < 0 || y < 0 || static_cast<uint32_t>(x) >= img.getWidth() || static_cast<uint32_t>(y) >= img.getHeight()) {
        std::cerr << "Координаты пикселя (" << x << ", " << y << ") выходят за пределы изображения.\n";
        return ColorRGB{0, 0, 0}; 
    }

    uint32_t index = y * img.getWidth() + x;
    const auto& pixelIds = img.getPixelIds();

    if (index >= pixelIds.size()) {
        std::cerr << "Индекс пикселя " << index << " выходит за пределы массива пикселей.\n";
        return ColorRGB{0, 0, 0};
    }

    uint8_t id = pixelIds[index];
    const auto& colorTable = img.getColorTable();
    auto it = colorTable.find(id);
    if (it != colorTable.end()) {
        return it->second;
    } else {
        std::cerr << "ID цвета " << static_cast<int>(id) << " не найден в цветовой таблице.\n";
        return ColorRGB{0, 0, 0}; 
    }
}

CompressedImage readCompressedFile(const std::string& filename) {
    CompressedImage cImg;
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        std::cerr << "Не удалось открыть CompressedImage файл для чтения: " << filename << std::endl;
        return cImg; 
    }

    char format[10];
    infile.read(format, 10);
    if (std::string(format, 10) != "CMPRIMAGE\0") {
        std::cerr << "Неверный формат CompressedImage файла: " << filename << std::endl;
        return cImg;
    }

    unsigned char version[3];
    infile.read(reinterpret_cast<char*>(version), 3);
    if (version[0] != 6 || version[1] != 6 || version[2] != 6) {
        std::cerr << "Неверная версия CompressedImage файла: " << filename << std::endl;
        return cImg;
    }

    uint32_t width, height;
    infile.read(reinterpret_cast<char*>(&width), 4);
    infile.read(reinterpret_cast<char*>(&height), 4);
    cImg.setWidth(width);
    cImg.setHeight(height);
    cImg.setGrayscale(false); 

    unsigned char pow;
    infile.read(reinterpret_cast<char*>(&pow), 1);
    size_t colorTableSize = std::pow(2, pow);
    std::map<uint8_t, ColorRGB> colorTable;

    for (size_t i = 0; i < colorTableSize; ++i) {
        ColorRGB color = readFromFileStream(infile);
        colorTable[i] = color;
    }

    cImg.setColorTable(colorTable);

    std::vector<uint8_t> pixelIds(width * height);
    infile.read(reinterpret_cast<char*>(pixelIds.data()), width * height);
    if (infile.gcount() != static_cast<std::streamsize>(width * height)) {
        std::cerr << "Некорректный размер пикселей в CompressedImage файле: " << filename << std::endl;
        return cImg;
    }
    cImg.setPixelIds(pixelIds);

    char end[10];
    infile.read(end, 10);
    if (std::string(end, 10) != "CMPRIMGEND\0") {
        std::cerr << "Отсутствует завершающая подпись в CompressedImage файле: " << filename << std::endl;
    }

    infile.close();
    return cImg;
}

void writeCompressedFile(const std::string& filename, const CompressedImage& image) {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        std::cerr << "Не удалось открыть CompressedImage файл для записи: " << filename << std::endl;
        return;
    }

    char format[10] = "CMPRIMAGE\0";
    outfile.write(format, 10);

    unsigned char version[3] = {6, 6, 6};
    outfile.write(reinterpret_cast<char*>(version), 3);

    uint32_t width = image.getWidth();
    uint32_t height = image.getHeight();
    outfile.write(reinterpret_cast<const char*>(&width), 4);
    outfile.write(reinterpret_cast<const char*>(&height), 4);

    const auto& colorTable = image.getColorTable();
    size_t colorTableSize = colorTable.size();
    unsigned char pow = 0;
    while (std::pow(2, pow) < colorTableSize) {
        pow++;
    }
    outfile.write(reinterpret_cast<char*>(&pow), 1);

    for (const auto& [id, color] : colorTable) {
        outfile.write(reinterpret_cast<const char*>(&color.r), 1);
        outfile.write(reinterpret_cast<const char*>(&color.g), 1);
        outfile.write(reinterpret_cast<const char*>(&color.b), 1);
    }

    const auto& pixelIds = image.getPixelIds();
    outfile.write(reinterpret_cast<const char*>(pixelIds.data()), pixelIds.size());

    char end[10] = "CMPRIMGEND\0";
    outfile.write(end, 10);

    outfile.close();
}
