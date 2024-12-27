#include "images.h"
#include "error_handlers.h"
#include "compressor_funcs.h" 
#include <fstream>
#include <iostream>

UncompressedImage::UncompressedImage()
    : width(0), height(0), is_grayscale(false), image_data() {}

UncompressedImage::UncompressedImage(uint32_t w, uint32_t h, bool gray)
    : width(w), height(h), is_grayscale(gray),
      image_data(h, std::vector<ColorRGB>(w, ColorRGB{0, 0, 0})) {}

uint32_t UncompressedImage::getWidth() const { return width; }
uint32_t UncompressedImage::getHeight() const { return height; }
bool UncompressedImage::getIsGrayscale() const { return is_grayscale; }
const std::vector<std::vector<ColorRGB>>& UncompressedImage::getImageData() const { return image_data; }

void UncompressedImage::setWidth(uint32_t w) { width = w; }
void UncompressedImage::setHeight(uint32_t h) { height = h; }
void UncompressedImage::setIsGrayscale(bool gray) { is_grayscale = gray; }
void UncompressedImage::setImageData(const std::vector<std::vector<ColorRGB>>& data) { image_data = data; }

void UncompressedImage::setPixel(uint32_t x, uint32_t y, const ColorRGB& color) {
    if (x >= width || y >= height) {
        handleLogMessage("Попытка доступа к пикселю вне границ изображения.", Severity::WARNING);
        return;
    }
    image_data[y][x] = color;
}

bool UncompressedImage::readFromFile(const std::string& filename) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        handleLogMessage("Не удалось открыть файл для чтения: " + filename, Severity::ERROR);
        return false;
    }

    char format[10];
    infile.read(format, 10);
    if (std::string(format, 10) != "RAWIMAGE\0") {
        handleLogMessage("Неверный формат файла: " + filename, Severity::ERROR);
        return false;
    }
    unsigned char version[3];
    infile.read(reinterpret_cast<char*>(version), 3);
    if (version[0] != 1 || version[1] != 0 || version[2] != 0) {
        handleLogMessage("Неверная версия формата файла: " + filename, Severity::ERROR);
        return false;
    }

    infile.read(reinterpret_cast<char*>(&width), 4);
    infile.read(reinterpret_cast<char*>(&height), 4);

    unsigned char gray_flag;
    infile.read(reinterpret_cast<char*>(&gray_flag), 1);
    is_grayscale = (gray_flag == 1);

    image_data.assign(height, std::vector<ColorRGB>(width, ColorRGB{0, 0, 0}));

    if (is_grayscale) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                uint8_t gray;
                infile.read(reinterpret_cast<char*>(&gray), 1);
                image_data[y][x] = ColorRGB{gray, gray, gray};
            }
        }
    } else {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                ColorRGB color = readFromFileStream(infile);
                image_data[y][x] = color;
            }
        }
    }

    char end[10];
    infile.read(end, 10);
    if (std::string(end, 10) != "RAWIMGEND\0") {
        handleLogMessage("Отсутствует завершающая подпись в файле: " + filename, Severity::ERROR);
        return false;
    }

    infile.close();
    handleLogMessage("Файл успешно прочитан: " + filename, Severity::INFO);
    return true;
}

bool UncompressedImage::writeToFile(const std::string& filename) const {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        handleLogMessage("Не удалось открыть файл для записи: " + filename, Severity::ERROR);
        return false;
    }

    char format[10] = "RAWIMAGE\0";
    outfile.write(format, 10);

    unsigned char version[3] = {1, 0, 0};
    outfile.write(reinterpret_cast<const char*>(version), 3);

    outfile.write(reinterpret_cast<const char*>(&width), 4);
    outfile.write(reinterpret_cast<const char*>(&height), 4);

    unsigned char gray_flag = is_grayscale ? 1 : 0;
    outfile.write(reinterpret_cast<const char*>(&gray_flag), 1);

    if (is_grayscale) {
        for (const auto& row : image_data) {
            for (const auto& pixel : row) {
                uint8_t gray = pixel.r; 
                outfile.write(reinterpret_cast<const char*>(&gray), 1);
            }
        }
    } else {
        for (const auto& row : image_data) {
            for (const auto& pixel : row) {
                outfile.write(reinterpret_cast<const char*>(&pixel.r), 1);
                outfile.write(reinterpret_cast<const char*>(&pixel.g), 1);
                outfile.write(reinterpret_cast<const char*>(&pixel.b), 1);
            }
        }
    }

    char end[10] = "RAWIMGEND\0";
    outfile.write(end, 10);

    outfile.close();
    handleLogMessage("Файл успешно записан: " + filename, Severity::INFO);
    return true;
}

CompressedImage::CompressedImage()
    : width(0), height(0), id_to_color(), color_to_id(), image_data() {}

CompressedImage::CompressedImage(uint32_t w, uint32_t h)
    : width(w), height(h), id_to_color(), color_to_id(),
      image_data(h, std::vector<uint8_t>(w, 0)) {}

uint32_t CompressedImage::getWidth() const { return width; }
uint32_t CompressedImage::getHeight() const { return height; }
const std::map<uint8_t, ColorRGB>& CompressedImage::getIdToColor() const { return id_to_color; }
const std::unordered_map<ColorRGB, uint8_t, ColorHash>& CompressedImage::getColorToId() const { return color_to_id; }
const std::vector<std::vector<uint8_t>>& CompressedImage::getImageData() const { return image_data; }

void CompressedImage::setWidth(uint32_t w) { width = w; }
void CompressedImage::setHeight(uint32_t h) { height = h; }
void CompressedImage::setIdToColor(const std::map<uint8_t, ColorRGB>& table) { id_to_color = table; }
void CompressedImage::setColorToId(const std::unordered_map<ColorRGB, uint8_t, ColorHash>& table) { color_to_id = table; }
void CompressedImage::setImageData(const std::vector<std::vector<uint8_t>>& data) { image_data = data; }

void CompressedImage::setPixel(uint32_t x, uint32_t y, uint8_t color_id) {
    if (x >= width || y >= height) {
        handleLogMessage("Попытка доступа к пикселю вне границ изображения.", Severity::WARNING);
        return;
    }
    image_data[y][x] = color_id;
}

bool CompressedImage::readFromFile(const std::string& filename) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        handleLogMessage("Не удалось открыть файл для чтения: " + filename, Severity::ERROR);
        return false;
    }

    char format[10];
    infile.read(format, 10);
    if (std::string(format, 10) != "CMPRIMAGE\0") {
        handleLogMessage("Неверный формат файла: " + filename, Severity::ERROR);
        return false;
    }
    unsigned char version[3];
    infile.read(reinterpret_cast<char*>(version), 3);
    if (version[0] != 6 || version[1] != 6 || version[2] != 6) {
        handleLogMessage("Неверная версия формата файла: " + filename, Severity::ERROR);
        return false;
    }

    infile.read(reinterpret_cast<char*>(&width), 4);
    infile.read(reinterpret_cast<char*>(&height), 4);

    unsigned char pow;
    infile.read(reinterpret_cast<char*>(&pow), 1);
    size_t colorTableSize = std::pow(2, pow);

    id_to_color.clear();
    color_to_id.clear();
    for (size_t i = 0; i < colorTableSize; ++i) {
        ColorRGB color = readFromFileStream(infile);
        id_to_color[static_cast<uint8_t>(i)] = color;
        color_to_id[color] = static_cast<uint8_t>(i);
    }

    image_data.assign(height, std::vector<uint8_t>(width, 0));

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint8_t color_id;
            infile.read(reinterpret_cast<char*>(&color_id), 1);
            if (infile.eof() || infile.fail()) {
                handleLogMessage("Некорректный размер данных пикселей в файле: " + filename, Severity::ERROR);
                return false;
            }
            image_data[y][x] = color_id;
        }
    }

    char end[10];
    infile.read(end, 10);
    if (std::string(end, 10) != "CMPRIMGEND\0") {
        handleLogMessage("Отсутствует завершающая подпись в файле: " + filename, Severity::ERROR);
        return false;
    }

    infile.close();
    handleLogMessage("Файл успешно прочитан: " + filename, Severity::INFO);
    return true;
}

bool CompressedImage::writeToFile(const std::string& filename) const {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        handleLogMessage("Не удалось открыть файл для записи: " + filename, Severity::ERROR);
        return false;
    }

    char format[10] = "CMPRIMAGE\0";
    outfile.write(format, 10);

    unsigned char version[3] = {6, 6, 6};
    outfile.write(reinterpret_cast<const char*>(version), 3);

    outfile.write(reinterpret_cast<const char*>(&width), 4);
    outfile.write(reinterpret_cast<const char*>(&height), 4);

    size_t colorTableSize = id_to_color.size();
    unsigned char pow = 0;
    while (std::pow(2, pow) < colorTableSize) {
        pow++;
    }
    outfile.write(reinterpret_cast<const char*>(&pow), 1);

    for (const auto& [id, color] : id_to_color) {
        outfile.write(reinterpret_cast<const char*>(&color.r), 1);
        outfile.write(reinterpret_cast<const char*>(&color.g), 1);
        outfile.write(reinterpret_cast<const char*>(&color.b), 1);
    }

    for (const auto& row : image_data) {
        for (const auto& color_id : row) {
            outfile.write(reinterpret_cast<const char*>(&color_id), 1);
        }
    }

    char end[10] = "CMPRIMGEND\0";
    outfile.write(end, 10);

    outfile.close();
    handleLogMessage("Файл успешно записан: " + filename, Severity::INFO);
    return true;
}


bool matchUncompressedImages(const UncompressedImage& img1, const UncompressedImage& img2, bool verbose) {
    if (img1.getWidth() != img2.getWidth() || img1.getHeight() != img2.getHeight()) {
        if (verbose) {
            std::cerr << "Размеры изображений не совпадают.\n";
            std::cerr << "Изображение 1: " << img1.getWidth() << "x" << img1.getHeight() << "\n";
            std::cerr << "Изображение 2: " << img2.getWidth() << "x" << img2.getHeight() << "\n";
        }
        return false;
    }

    if (img1.getIsGrayscale() != img2.getIsGrayscale()) {
        if (verbose) {
            std::cerr << "Изображения имеют разную цветовую палитру (градации серого vs цветные).\n";
        }
        return false;
    }

    for (uint32_t y = 0; y < img1.getHeight(); ++y) {
        for (uint32_t x = 0; x < img1.getWidth(); ++x) {
            const ColorRGB& color1 = img1.getImageData()[y][x];
            const ColorRGB& color2 = img2.getImageData()[y][x];
            if (color1 != color2) {
                if (verbose) {
                    std::cerr << "Несоответствие пикселей на координатах (" << y << ", " << x << "): ";
                    std::cerr << "ожидалось " << color1 << ", получили " << color2 << "\n";
                }
                return false;
            }
        }
    }

    if (verbose) {
        std::cout << "Изображения совпадают.\n";
    }
    return true;
}
