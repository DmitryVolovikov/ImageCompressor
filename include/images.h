#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include "colors.h"

class UncompressedImage {
private:
    uint32_t width;
    uint32_t height;
    bool is_grayscale;
    std::vector<std::vector<ColorRGB>> image_data;

public:
    UncompressedImage();
    UncompressedImage(uint32_t width, uint32_t height, bool is_grayscale = false);

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    bool getIsGrayscale() const;
    const std::vector<std::vector<ColorRGB>>& getImageData() const;

    void setWidth(uint32_t w);
    void setHeight(uint32_t h);
    void setIsGrayscale(bool gray);
    void setImageData(const std::vector<std::vector<ColorRGB>>& data);
    void setPixel(uint32_t x, uint32_t y, const ColorRGB& color);

    bool readFromFile(const std::string& filename);
    bool writeToFile(const std::string& filename) const;
};

class CompressedImage {
private:
    uint32_t width;
    uint32_t height;
    std::map<uint8_t, ColorRGB> id_to_color;                    
    std::unordered_map<ColorRGB, uint8_t, ColorHash> color_to_id; 
    std::vector<std::vector<uint8_t>> image_data;

public:
    CompressedImage();
    CompressedImage(uint32_t width, uint32_t height);

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    const std::map<uint8_t, ColorRGB>& getIdToColor() const;
    const std::unordered_map<ColorRGB, uint8_t, ColorHash>& getColorToId() const;
    const std::vector<std::vector<uint8_t>>& getImageData() const;

    void setWidth(uint32_t w);
    void setHeight(uint32_t h);
    void setIdToColor(const std::map<uint8_t, ColorRGB>& table);
    void setColorToId(const std::unordered_map<ColorRGB, uint8_t, ColorHash>& table);
    void setImageData(const std::vector<std::vector<uint8_t>>& data);
    void setPixel(uint32_t x, uint32_t y, uint8_t color_id);

    bool readFromFile(const std::string& filename);
    bool writeToFile(const std::string& filename) const;
};

bool matchUncompressedImages(const UncompressedImage& img1, const UncompressedImage& img2, bool verbose = true);
