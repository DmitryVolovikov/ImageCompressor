#include "libbmp.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <algorithm>


BMP::BMP(int width, int height, bool has_alpha) {
    if (width <= 0 || height == 0) { 
        throw std::runtime_error("The image width must be positive and height cannot be zero.");
    }

    bmp_info_header.size = sizeof(BMPInfoHeader);
    bmp_info_header.width = width;
    bmp_info_header.height = height;
    bmp_info_header.bit_count = has_alpha ? 32 : 24;
    bmp_info_header.planes = 1;
    bmp_info_header.compression = has_alpha ? 3 : 0; 
    bmp_info_header.size_image = 0; 
    bmp_info_header.x_pixels_per_meter = 0;
    bmp_info_header.y_pixels_per_meter = 0;
    bmp_info_header.colors_used = has_alpha ? 0 : 0;
    bmp_info_header.colors_important = 0;

    if (has_alpha) {
        bmp_color_header.red_mask = 0x00ff0000;
        bmp_color_header.green_mask = 0x0000ff00;
        bmp_color_header.blue_mask = 0x000000ff;
        bmp_color_header.alpha_mask = 0xff000000;
        bmp_color_header.color_space_type = 0x73524742;
    }

    file_header.offset_data = sizeof(BMPHeader) + bmp_info_header.size;
    if (has_alpha) {
        file_header.offset_data += sizeof(BMPColorHeader);
        bmp_info_header.size += sizeof(BMPColorHeader);
    }
    file_header.file_size = file_header.offset_data;

    row_stride = width * bmp_info_header.bit_count / 8;
    data.resize(row_stride * std::abs(bmp_info_header.height), 0);

    file_header.file_size += data.size();
}

BMP::BMP(const char* fname) {
    read(fname);
}


void BMP::read(const char* fname) {
    std::ifstream inp{fname, std::ios_base::binary};
    if (!inp) {
        throw std::runtime_error("Unable to open the input image file.");
    }

    inp.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (file_header.file_type != 0x4D42) {
        throw std::runtime_error("Error! Unrecognized file format.");
    }

    inp.read(reinterpret_cast<char*>(&bmp_info_header), sizeof(bmp_info_header));

    bool has_alpha = (bmp_info_header.bit_count == 32);
    if (has_alpha) {
        if (bmp_info_header.size >= sizeof(BMPInfoHeader) + sizeof(BMPColorHeader)) {
            inp.read(reinterpret_cast<char*>(&bmp_color_header), sizeof(bmp_color_header));
        } else {
            std::cerr << "Error! The file \"" << fname
                      << "\" does not seem to contain bit mask information\n";
            throw std::runtime_error("Error! Unrecognized file format.");
        }
    }

    bool is_bottom_up = true;
    if (bmp_info_header.height < 0) {
        is_bottom_up = false;
        bmp_info_header.height = std::abs(bmp_info_header.height);
    }

    row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
    uint32_t new_stride = make_stride_aligned(4);
    size_t padding = new_stride - row_stride;

    data.resize(row_stride * bmp_info_header.height, 0);

    for (int y = 0; y < bmp_info_header.height; ++y) {
        int read_y = is_bottom_up ? (bmp_info_header.height - 1 - y) : y;
        inp.read(reinterpret_cast<char*>(data.data() + read_y * row_stride), row_stride);
        if (padding > 0) {
            inp.ignore(padding); 
        }
    }

    file_header.file_size = file_header.offset_data + row_stride * bmp_info_header.height + padding * bmp_info_header.height;

    inp.peek();
    if (!inp.eof()) {
        std::cerr << "Warning: Extra data found in the BMP file \"" << fname << "\".\n";
    }

    inp.close();
}

void BMP::write(const char* fname) {
    std::ofstream of{fname, std::ios_base::binary};
    if (!of) {
        throw std::runtime_error("Unable to open the output image file.");
    }

    write_headers(of);

    uint32_t new_stride = make_stride_aligned(4);
    size_t padding = new_stride - row_stride;
    std::vector<uint8_t> padding_bytes(padding, 0);

    bool is_bottom_up = (bmp_info_header.height > 0);

    for (int y = 0; y < bmp_info_header.height; ++y) {
        int write_y = is_bottom_up ? (bmp_info_header.height - 1 - y) : y;
        of.write(reinterpret_cast<const char*>(data.data() + write_y * row_stride), row_stride);
        if (padding > 0) {
            of.write(reinterpret_cast<const char*>(padding_bytes.data()), padding);
        }
    }

    file_header.file_size = file_header.offset_data + row_stride * bmp_info_header.height + padding * bmp_info_header.height;
    of.seekp(2, std::ios::beg);
    of.write(reinterpret_cast<const char*>(&file_header.file_size), sizeof(file_header.file_size));

    of.close();
}


void BMP::write_headers(std::ofstream& of) {
    of.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    of.write(reinterpret_cast<const char*>(&bmp_info_header), sizeof(bmp_info_header));
    if (bmp_info_header.bit_count == 32) {
        of.write(reinterpret_cast<const char*>(&bmp_color_header), sizeof(bmp_color_header));
    }
}


uint32_t BMP::make_stride_aligned(uint32_t align_stride) {
    uint32_t new_stride = row_stride;
    while (new_stride % align_stride != 0) {
        new_stride++;
    }
    return new_stride;
}

void BMP::set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || y < 0 || x >= bmp_info_header.width || y >= std::abs(bmp_info_header.height)) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    uint32_t channels = bmp_info_header.bit_count / 8;
    uint32_t index = y * row_stride + x * channels;
    data[index] = b;
    data[index + 1] = g;
    data[index + 2] = r;
}

void BMP::set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || y < 0 || x >= bmp_info_header.width || y >= std::abs(bmp_info_header.height)) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    if (bmp_info_header.bit_count != 32) {
        throw std::runtime_error("Alpha channel is only supported for 32-bit BMP images.");
    }
    uint32_t channels = bmp_info_header.bit_count / 8;
    uint32_t index = y * row_stride + x * channels;
    data[index] = b;
    data[index + 1] = g;
    data[index + 2] = r;
    data[index + 3] = a;
}

void BMP::get_pixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b) const {
    if (x < 0 || y < 0 || x >= bmp_info_header.width || y >= std::abs(bmp_info_header.height)) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    uint32_t channels = bmp_info_header.bit_count / 8;
    uint32_t index = y * row_stride + x * channels;
    b = data[index];
    g = data[index + 1];
    r = data[index + 2];
}

void BMP::get_pixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {
    if (x < 0 || y < 0 || x >= bmp_info_header.width || y >= std::abs(bmp_info_header.height)) {
        throw std::out_of_range("Pixel coordinates are out of bounds.");
    }
    if (bmp_info_header.bit_count != 32) {
        throw std::runtime_error("Alpha channel is only supported for 32-bit BMP images.");
    }
    uint32_t channels = bmp_info_header.bit_count / 8;
    uint32_t index = y * row_stride + x * channels;
    b = data[index];
    g = data[index + 1];
    r = data[index + 2];
    a = data[index + 3];
}

int BMP::get_width() const { return bmp_info_header.width; }

int BMP::get_height() const { return bmp_info_header.height; }
