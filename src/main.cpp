#include "error_handlers.h"
#include "images.h"
#include "image_transforms.h"
#include "libbmp.h"
#include <iostream>
#include <exception>
#include <unordered_map>
#include <map>

UncompressedImage convertBMPToUncompressed(const BMP& bmp) {
    int width = bmp.get_width();
    int height = bmp.get_height();
    bool is_grayscale = false; 

    UncompressedImage img(width, height, is_grayscale);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t r, g, b;
            bmp.get_pixel(x, y, r, g, b);
            img.setPixel(x, y, ColorRGB{r, g, b});
        }
    }
    return img;
}

BMP convertUncompressedToBMP(const UncompressedImage& img) {
    BMP bmp_saver(img.getWidth(), img.getHeight(), img.getIsGrayscale());
    for (int y = 0; y < img.getHeight(); ++y) {
        for (int x = 0; x < img.getWidth(); ++x) {
            ColorRGB color = img.getImageData()[y][x];
            bmp_saver.set_pixel(x, y, color.r, color.g, color.b);
        }
    }
    return bmp_saver;
}

int main() {
    openLogFile("log.txt", true);

    try {
        BMP bmp_loader("images/sample.bmp");
        int width = bmp_loader.get_width();
        int height = bmp_loader.get_height();

        UncompressedImage img1 = convertBMPToUncompressed(bmp_loader);
        handleLogMessage("BMP изображение загружено и сконвертировано в UncompressedImage.", Severity::INFO);

        rotate(img1, 90);
        handleLogMessage("Изображение повернуто на 90 градусов.", Severity::INFO);

        sharpen(img1);
        handleLogMessage("Фильтр резкости применён.", Severity::INFO);

        toGrayscale(img1);
        handleLogMessage("Изображение преобразовано в градации серого.", Severity::INFO);

        img1.writeToFile("output.raw");
        handleLogMessage("UncompressedImage сохранено в output.raw.", Severity::INFO);

        UncompressedImage img2;
        img2.readFromFile("output.raw");
        handleLogMessage("UncompressedImage прочитано из output.raw.", Severity::INFO);

        bool images_match = matchUncompressedImages(img1, img2, true);
        if (images_match) {
            handleLogMessage("Изображения совпадают.", Severity::INFO);
        } else {
            handleLogMessage("Изображения не совпадают.", Severity::WARNING);
        }

        CompressedImage cImg;
        std::map<uint8_t, ColorRGB> id_to_color;
        std::unordered_map<ColorRGB, uint8_t, ColorHash> color_to_id;
        uint8_t current_id = 0;

        for (int y = 0; y < img1.getHeight(); ++y) {
            for (int x = 0; x < img1.getWidth(); ++x) {
                ColorRGB color = img1.getImageData()[y][x];
                if (color_to_id.find(color) == color_to_id.end()) {
                    if (current_id >= 256) { 
                        handleLogMessage("Превышено максимальное количество цветов (256).", Severity::WARNING);
                        break;
                    }
                    color_to_id[color] = current_id;
                    id_to_color[current_id] = color;
                    current_id++;
                }
            }
        }

        cImg.setWidth(img1.getWidth());
        cImg.setHeight(img1.getHeight());
        cImg.setIdToColor(id_to_color);
        cImg.setColorToId(color_to_id);

        std::vector<std::vector<uint8_t>> compressed_data(img1.getHeight(), std::vector<uint8_t>(img1.getWidth(), 0));
        for (int y = 0; y < img1.getHeight(); ++y) {
            for (int x = 0; x < img1.getWidth(); ++x) {
                ColorRGB color = img1.getImageData()[y][x];
                uint8_t id = color_to_id[color];
                compressed_data[y][x] = id;
            }
        }
        cImg.setImageData(compressed_data);
        handleLogMessage("UncompressedImage конвертировано в CompressedImage.", Severity::INFO);

        mirror(cImg, true);
        handleLogMessage("Изображение отражено по горизонтали.", Severity::INFO);

        cImg.writeToFile("output.cmpr");
        handleLogMessage("CompressedImage сохранено в output.cmpr.", Severity::INFO);

        CompressedImage cImg_loaded;
        cImg_loaded.readFromFile("output.cmpr");
        handleLogMessage("CompressedImage прочитано из output.cmpr.", Severity::INFO);

        UncompressedImage img_reconstructed(cImg_loaded.getWidth(), cImg_loaded.getHeight(), img1.getIsGrayscale());
        for (int y = 0; y < cImg_loaded.getHeight(); ++y) {
            for (int x = 0; x < cImg_loaded.getWidth(); ++x) {
                uint8_t id = cImg_loaded.getImageData()[y][x];
                ColorRGB color = cImg_loaded.getIdToColor().at(id);
                img_reconstructed.setPixel(x, y, color);
            }
        }
        handleLogMessage("CompressedImage конвертировано обратно в UncompressedImage.", Severity::INFO);

        BMP bmp_saver = convertUncompressedToBMP(img_reconstructed);
        bmp_saver.write("reconstructed.bmp");
        handleLogMessage("Восстановленное изображение сохранено в reconstructed.bmp.", Severity::INFO);
    }
    catch (const std::exception& e) {
        handleLogMessage(std::string("Исключение: ") + e.what(), Severity::CRITICAL, 1);
    }

    closeLogFile();

    return 0;
}
