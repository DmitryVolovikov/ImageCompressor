#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <fstream>
#include <iostream>

#include "compressor_funcs.h"
#include "image_transforms.h"
#include "images.h"
#include "libbmp.h"
#include "colors.h"
#include "error_handlers.h"

std::vector<uint8_t> loadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        file.close();
        handleLogMessage("Cannot open file " + filename, Severity::CRITICAL, 1);
    }
    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return data;
}

template<typename T>
bool matchVectors(const std::vector<T>& v1, const std::vector<T>& v2) {
    return v1 == v2;
}

struct TestAwarder {
  public:
    TestAwarder(const std::string& filename = "", bool verbose = true) : verbose_(verbose) {
        if (filename.empty()) {
            return;
        }
        file_.open(filename, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            throw std::runtime_error("Cannot open points file");
        }
        std::cout.rdbuf(file_.rdbuf());
    }

    ~TestAwarder() {
        if (verbose_) {
            std::cout << "Total points awarded: ";
        }
        std::cout << secret_contest_seed << (compulsory_test_remaining_ ? 0 : totalPoints_) << std::endl;
        if (file_.is_open()) {
            file_.close();
        }
    }

    void awardPoints(size_t points, const std::string& test_name = "", bool passed_compulsory_test=false) {
        totalPoints_ += points;
        if (passed_compulsory_test) {
            --compulsory_test_remaining_;
        }
        if (verbose_) {
            std::cout << "Awarded: ";
        }
        std::cout << secret_contest_seed << points;
        if (test_name != "") {
            std::cout << " for " << test_name;
        }
        std::cout << std::endl;
    }

    static constexpr int NUMBER_OF_COMPULSORY_TESTS = 2;

  private:
    size_t totalPoints_ = 0;
    int compulsory_test_remaining_ = NUMBER_OF_COMPULSORY_TESTS;

    bool verbose_ = true;
    std::ofstream file_;
    std::string secret_contest_seed = "";
};

// START OF TESTS

TestAwarder awarder("", true);

TEST_CASE("BMP read") {
    constexpr size_t TEST_AWARD_POINTS = 1;
    openLogFile("logs/test_01.log", true);

    UncompressedImage img = loadFromBMP("images/red_cross.bmp");
    REQUIRE(img.width == 8);
    REQUIRE(img.height == 8);
    REQUIRE_FALSE(img.is_grayscale);

    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            if (i == j || i == img.height - j - 1) {
                REQUIRE(img.image_data[i][j] == ColorRGB{255, 0, 0});
            } else {
                REQUIRE(img.image_data[i][j] == ColorRGB{0, 0, 0});
            }
        }
    }
    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName(), true);
}

TEST_CASE("BMP save and read") {
    constexpr size_t TEST_AWARD_POINTS = 1;
    openLogFile("logs/test_02.log", true);

    UncompressedImage img = loadFromBMP("images/red_cross.bmp");
    saveAsBMP(img, "tmp_images/red_cross.bmp");

    UncompressedImage loaded_img = loadFromBMP("images/red_cross.bmp");
    REQUIRE(loaded_img.width == img.width);
    REQUIRE(loaded_img.height == img.height);
    REQUIRE(loaded_img.is_grayscale == img.is_grayscale);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(loaded_img.image_data[i][j] == img.image_data[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName(), true);
}

TEST_CASE("Simple horizontally mirrored image") {
    constexpr size_t TEST_AWARD_POINTS = 2;
    openLogFile("logs/test_03.log", true);

    const std::vector<std::vector<ColorRGB>> sample_3x3_image = {
        {ColorRGB{255, 0, 0}, ColorRGB{0, 255, 0}, ColorRGB{0, 0, 255}},
        {ColorRGB{0, 0, 255}, ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}},
        {ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}, ColorRGB{255, 0, 0}}};

    const std::vector<std::vector<ColorRGB>> sample_3x3_image_hor_flip = {
        {ColorRGB{0, 0, 255}, ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}},
        {ColorRGB{255, 0, 0}, ColorRGB{0, 255, 0}, ColorRGB{0, 0, 255}},
        {ColorRGB{255, 0, 0}, ColorRGB{255, 0, 0}, ColorRGB{0, 255, 0}}};

    UncompressedImage img;
    img.width = 3;
    img.height = 3;
    img.is_grayscale = false;
    img.image_data = sample_3x3_image;

    mirror(img, true);

    REQUIRE(img.width == 3);
    REQUIRE(img.height == 3);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(img.image_data[i][j] == sample_3x3_image_hor_flip[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Horizontally mirrored image") {
    constexpr size_t TEST_AWARD_POINTS = 1;
    openLogFile("logs/test_04.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    mirror(img, true);
    saveAsBMP(img, "tmp_images/kapibara_hor_flip.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_hor_flip.bmp"),
        loadFile("correct_images/kapibara_hor_flip.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Simple vertically mirrored image") {
    constexpr size_t TEST_AWARD_POINTS = 2;
    openLogFile("logs/test_05.log", true);

    const std::vector<std::vector<ColorRGB>> sample_3x3_image = {
        {ColorRGB{255, 0, 0}, ColorRGB{0, 255, 0}, ColorRGB{0, 0, 255}},
        {ColorRGB{0, 0, 255}, ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}},
        {ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}, ColorRGB{255, 0, 0}}};

    const std::vector<std::vector<ColorRGB>> sample_3x3_image_vert_flip = {
        {ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}, ColorRGB{255, 0, 0}},
        {ColorRGB{0, 0, 255}, ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}},
        {ColorRGB{255, 0, 0}, ColorRGB{0, 255, 0}, ColorRGB{0, 0, 255}}};

    UncompressedImage img;
    img.width = 3;
    img.height = 3;
    img.is_grayscale = false;
    img.image_data = sample_3x3_image;

    mirror(img);

    REQUIRE(img.width == 3);
    REQUIRE(img.height == 3);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(img.image_data[i][j] == sample_3x3_image_vert_flip[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Vertically mirrored image") {
    constexpr size_t TEST_AWARD_POINTS = 1;
    openLogFile("logs/test_06.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    mirror(img);
    saveAsBMP(img, "tmp_images/kapibara_vert_flip.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_vert_flip.bmp"),
        loadFile("correct_images/kapibara_vert_flip.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Simple grayscale conversion") {
    constexpr size_t TEST_AWARD_POINTS = 3;
    openLogFile("logs/test_07.log", true);

    const std::vector<std::vector<ColorRGB>> sample_3x3_image = {
        {ColorRGB{255, 0, 0}, ColorRGB{0, 255, 0}, ColorRGB{0, 0, 255}},
        {ColorRGB{0, 0, 255}, ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}},
        {ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}, ColorRGB{255, 0, 0}}};

    const std::vector<std::vector<ColorRGB>> sample_3x3_image_gray = {
        {ColorRGB{85, 85, 85}, ColorRGB{85, 85, 85}, ColorRGB{85, 85, 85}},
        {ColorRGB{85, 85, 85}, ColorRGB{85, 85, 85}, ColorRGB{85, 85, 85}},
        {ColorRGB{85, 85, 85}, ColorRGB{85, 85, 85}, ColorRGB{85, 85, 85}}};

    UncompressedImage img;
    img.width = 3;
    img.height = 3;
    img.is_grayscale = false;
    img.image_data = sample_3x3_image;

    toGrayscale(img);

    REQUIRE(img.width == 3);
    REQUIRE(img.height == 3);
    REQUIRE(img.is_grayscale);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(img.image_data[i][j] == sample_3x3_image_gray[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Grayscale conversion") {
    constexpr size_t TEST_AWARD_POINTS = 2;
    openLogFile("logs/test_08.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    toGrayscale(img);
    saveAsBMP(img, "tmp_images/kapibara_grayscale.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_grayscale.bmp"),
        loadFile("correct_images/kapibara_grayscale.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Simple negative conversion") {
    constexpr size_t TEST_AWARD_POINTS = 2;
    openLogFile("logs/test_09.log", true);

    const std::vector<std::vector<ColorRGB>> sample_3x3_image = {
        {ColorRGB{255, 0, 0}, ColorRGB{0, 255, 0}, ColorRGB{0, 0, 255}},
        {ColorRGB{0, 0, 255}, ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}},
        {ColorRGB{0, 255, 0}, ColorRGB{255, 0, 0}, ColorRGB{255, 0, 0}}};

    const std::vector<std::vector<ColorRGB>> sample_3x3_image_neg = {
        {ColorRGB{0, 255, 255}, ColorRGB{255, 0, 255}, ColorRGB{255, 255, 0}},
        {ColorRGB{255, 255, 0}, ColorRGB{255, 0, 255}, ColorRGB{0, 255, 255}},
        {ColorRGB{255, 0, 255}, ColorRGB{0, 255, 255}, ColorRGB{0, 255, 255}}};

    UncompressedImage img;
    img.width = 3;
    img.height = 3;
    img.is_grayscale = false;
    img.image_data = sample_3x3_image;

    negative(img);

    REQUIRE(img.width == 3);
    REQUIRE(img.height == 3);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(img.image_data[i][j] == sample_3x3_image_neg[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Negative conversion") {
    constexpr size_t TEST_AWARD_POINTS = 1;
    openLogFile("logs/test_10.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    negative(img);
    saveAsBMP(img, "tmp_images/kapibara_negative.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_negative.bmp"),
        loadFile("correct_images/kapibara_negative.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Right angles rotation (small image + no interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 5;
    openLogFile("logs/test_11.log", true);

    UncompressedImage img = loadFromBMP("images/seven.bmp");
    UncompressedImage img_90 = img;
    UncompressedImage img_180 = img;
    UncompressedImage img_270 = img;

    rotate(img_90, 90, {0, 255, 0}, false);
    rotate(img_180, 180, {0, 255, 0}, false);
    rotate(img_270, 270, {0, 255, 0}, false);

    saveAsBMP(img_90, "tmp_images/seven_rotated_90.bmp");
    saveAsBMP(img_180, "tmp_images/seven_rotated_180.bmp");
    saveAsBMP(img_270, "tmp_images/seven_rotated_270.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/seven_rotated_90.bmp"),
        loadFile("correct_images/rotated_images/seven_rotated_90.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/seven_rotated_180.bmp"),
        loadFile("correct_images/rotated_images/seven_rotated_180.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/seven_rotated_270.bmp"),
        loadFile("correct_images/rotated_images/seven_rotated_270.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Arbitrary angles rotation (small image + no interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 8;
    openLogFile("logs/test_12.log", true);

    const std::vector<int> angles = {27, 45, 60, 75, 105, 120, 135, 150, 165};

    UncompressedImage img = loadFromBMP("images/seven.bmp");

    for (int angle : angles) {
        handleLogMessage("Angle: " + std::to_string(angle), Severity::INFO);
        UncompressedImage img_copy = img;
        rotate(img_copy, angle, {0, 255, 0}, false);
        saveAsBMP(img_copy, "tmp_images/seven_rotated_" + std::to_string(angle) + ".bmp");
        REQUIRE(matchVectors(
            loadFile("tmp_images/seven_rotated_" + std::to_string(angle) + ".bmp"),
            loadFile(
                "correct_images/rotated_images/seven_rotated_" + std::to_string(angle) + ".bmp")));
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Right angles rotation (small image + with interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 5;
    openLogFile("logs/test_13.log", true);

    UncompressedImage img = loadFromBMP("images/seven.bmp");
    UncompressedImage img_90 = img;
    UncompressedImage img_180 = img;
    UncompressedImage img_270 = img;

    rotate(img_90, 90, {0, 255, 0}, true);
    rotate(img_180, 180, {0, 255, 0}, true);
    rotate(img_270, 270, {0, 255, 0}, true);

    saveAsBMP(img_90, "tmp_images/seven_rotated_90_interpolated.bmp");
    saveAsBMP(img_180, "tmp_images/seven_rotated_180_interpolated.bmp");
    saveAsBMP(img_270, "tmp_images/seven_rotated_270_interpolated.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/seven_rotated_90_interpolated.bmp"),
        loadFile("correct_images/rotated_images/seven_rotated_90_interpolated.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/seven_rotated_180_interpolated.bmp"),
        loadFile("correct_images/rotated_images/seven_rotated_180_interpolated.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/seven_rotated_270_interpolated.bmp"),
        loadFile("correct_images/rotated_images/seven_rotated_270_interpolated.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Arbitrary angles rotation (small image + with interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 8;
    openLogFile("logs/test_14.log", true);

    const std::vector<int> angles = {27, 45, 60, 75, 105, 120, 135, 150, 165};

    UncompressedImage img = loadFromBMP("images/seven.bmp");

    for (int angle : angles) {
        handleLogMessage("Angle: " + std::to_string(angle), Severity::INFO);
        UncompressedImage img_copy = img;
        rotate(img_copy, angle, {0, 255, 0}, true);
        saveAsBMP(
            img_copy, "tmp_images/seven_rotated_" + std::to_string(angle) + "_interpolated.bmp");
        REQUIRE(matchVectors(
            loadFile("tmp_images/seven_rotated_" + std::to_string(angle) + "_interpolated.bmp"),
            loadFile(
                "correct_images/rotated_images/seven_rotated_" + std::to_string(angle)
                + "_interpolated.bmp")));
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Right angles rotation (large image + no interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 3;
    openLogFile("logs/test_15.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    UncompressedImage img_90 = img;
    UncompressedImage img_180 = img;
    UncompressedImage img_270 = img;

    rotate(img_90, 90, {0, 255, 0}, false);
    rotate(img_180, 180, {0, 255, 0}, false);
    rotate(img_270, 270, {0, 255, 0}, false);

    saveAsBMP(img_90, "tmp_images/kapibara_rotated_90.bmp");
    saveAsBMP(img_180, "tmp_images/kapibara_rotated_180.bmp");
    saveAsBMP(img_270, "tmp_images/kapibara_rotated_270.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_rotated_90.bmp"),
        loadFile("correct_images/rotated_images/kapibara_rotated_90.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_rotated_180.bmp"),
        loadFile("correct_images/rotated_images/kapibara_rotated_180.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_rotated_270.bmp"),
        loadFile("correct_images/rotated_images/kapibara_rotated_270.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Arbitrary angles rotation (large image + no interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 6;
    openLogFile("logs/test_16.log", true);

    const std::vector<int> angles = {27, 45, 60, 75, 105, 120, 135, 150, 165};

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");

    for (int angle : angles) {
        handleLogMessage("Angle: " + std::to_string(angle), Severity::INFO);
        UncompressedImage img_copy = img;
        rotate(img_copy, angle, {0, 255, 0}, false);
        saveAsBMP(img_copy, "tmp_images/kapibara_rotated_" + std::to_string(angle) + ".bmp");
        REQUIRE(matchVectors(
            loadFile("tmp_images/kapibara_rotated_" + std::to_string(angle) + ".bmp"),
            loadFile(
                "correct_images/rotated_images/kapibara_rotated_" + std::to_string(angle)
                + ".bmp")));
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Right angles rotation (large image + with interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 3;
    openLogFile("logs/test_17.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    UncompressedImage img_90 = img;
    UncompressedImage img_180 = img;
    UncompressedImage img_270 = img;

    rotate(img_90, 90, {0, 255, 0}, true);
    rotate(img_180, 180, {0, 255, 0}, true);
    rotate(img_270, 270, {0, 255, 0}, true);

    saveAsBMP(img_90, "tmp_images/kapibara_rotated_90_interpolated.bmp");
    saveAsBMP(img_180, "tmp_images/kapibara_rotated_180_interpolated.bmp");
    saveAsBMP(img_270, "tmp_images/kapibara_rotated_270_interpolated.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_rotated_90_interpolated.bmp"),
        loadFile("correct_images/rotated_images/kapibara_rotated_90_interpolated.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_rotated_180_interpolated.bmp"),
        loadFile("correct_images/rotated_images/kapibara_rotated_180_interpolated.bmp")));

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_rotated_270_interpolated.bmp"),
        loadFile("correct_images/rotated_images/kapibara_rotated_270_interpolated.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Arbitrary angles rotation (large image + with interpolation)") {
    constexpr size_t TEST_AWARD_POINTS = 5;
    openLogFile("logs/test_18.log", true);

    const std::vector<int> angles = {27, 45, 60, 75, 105, 120, 135, 150, 165};

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");

    for (int angle : angles) {
        handleLogMessage("Angle: " + std::to_string(angle), Severity::INFO);
        UncompressedImage img_copy = img;
        rotate(img_copy, angle, {0, 255, 0}, true);
        saveAsBMP(
            img_copy, "tmp_images/kapibara_rotated_" + std::to_string(angle) + "_interpolated.bmp");
        REQUIRE(matchVectors(
            loadFile("tmp_images/kapibara_rotated_" + std::to_string(angle) + "_interpolated.bmp"),
            loadFile(
                "correct_images/rotated_images/kapibara_rotated_" + std::to_string(angle)
                + "_interpolated.bmp")));
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Identity kernel filter") {
    constexpr size_t TEST_AWARD_POINTS = 2;
    openLogFile("logs/test_19.log", true);

    const std::vector<std::vector<int>> identity_kernel = {{0, 0, 0}, {0, 1, 0}, {0, 0, 0}};

    UncompressedImage img = loadFromBMP("images/red_cross.bmp");
    UncompressedImage img_copy = img;
    applyKernel(img_copy, identity_kernel);
    REQUIRE(matchVectors(img.image_data, img_copy.image_data));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Edge detection kernel filter") {
    constexpr size_t TEST_AWARD_POINTS = 4;
    openLogFile("logs/test_20.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    edgeDetect(img);
    saveAsBMP(img, "tmp_images/kapibara_edge_detect.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_edge_detect.bmp"),
        loadFile("correct_images/kapibara_edge_detect.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Approximated gaussian blur kernel filter") {
    constexpr size_t TEST_AWARD_POINTS = 4;
    openLogFile("logs/test_21.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    UncompressedImage img_copy = img;
    gaussianBlurApprox(img, false);
    gaussianBlurApprox(img_copy, true);

    saveAsBMP(img, "tmp_images/kapibara_blur.bmp");
    saveAsBMP(img_copy, "tmp_images/kapibara_blur_hard.bmp");

    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_blur.bmp"), loadFile("correct_images/kapibara_blur.bmp")));
    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_blur_hard.bmp"),
        loadFile("correct_images/kapibara_blur_hard.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Sharp kernel filter") {
    constexpr size_t TEST_AWARD_POINTS = 4;
    openLogFile("logs/test_22.log", true);

    UncompressedImage img = loadFromBMP("images/kapibara.bmp");
    sharpen(img);

    saveAsBMP(img, "tmp_images/kapibara_sharp.bmp");
    REQUIRE(matchVectors(
        loadFile("tmp_images/kapibara_sharp.bmp"), loadFile("correct_images/kapibara_sharp.bmp")));

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Color distance calculation") {
    constexpr size_t TEST_AWARD_POINTS = 1;
    openLogFile("logs/test_23.log", true);

    ColorRGB c1{255, 0, 0};
    ColorRGB c2{0, 255, 0};
    ColorRGB c3{0, 0, 255};
    ColorRGB c4{255, 255, 255};
    ColorRGB c5{100, 100, 100};
    ColorRGB c6{0, 0, 0};

    REQUIRE(colorDistanceSq(c1, c1) == 0);
    REQUIRE(colorDistanceSq(c1, c2) == 130050);
    REQUIRE(colorDistanceSq(c1, c3) == 130050);
    REQUIRE(colorDistanceSq(c4, c5) == 72075);
    REQUIRE(colorDistanceSq(c6, c4) == 195075);

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Conversion to compressed image and back (with color add)") {
    constexpr size_t TEST_AWARD_POINTS = 7;
    openLogFile("logs/test_24.log", true);

    UncompressedImage img = loadFromBMP("images/red_cross.bmp");
    CompressedImage comp_img = toCompressed(img);

    REQUIRE(comp_img.width == img.width);
    REQUIRE(comp_img.height == img.height);
    REQUIRE(comp_img.id_to_color.size() == 2);
    REQUIRE(comp_img.color_to_id.size() == 2);
    REQUIRE(comp_img.image_data.size() == img.height);
    for (size_t i = 0; i < img.height; ++i) {
        REQUIRE(comp_img.image_data[i].size() == img.width);
    }

    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(comp_img.id_to_color[comp_img.image_data[i][j]] == img.image_data[i][j]);
            REQUIRE(comp_img.color_to_id[img.image_data[i][j]] == comp_img.image_data[i][j]);
        }
    }

    UncompressedImage img_copy = toUncompressed(comp_img);
    REQUIRE(img.width == img_copy.width);
    REQUIRE(img.height == img_copy.height);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(img.image_data[i][j] == img_copy.image_data[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Compressed image read and write (with color add)") {
    constexpr size_t TEST_AWARD_POINTS = 7;
    openLogFile("logs/test_25.log", true);

    UncompressedImage img = loadFromBMP("images/red_cross.bmp");
    CompressedImage comp_img = toCompressed(img);

    writeCompressedFile("tmp_images/red_cross_compressed.img", comp_img);
    CompressedImage comp_img_copy = readCompressedFile("tmp_images/red_cross_compressed.img");

    REQUIRE(comp_img.width == comp_img_copy.width);
    REQUIRE(comp_img.height == comp_img_copy.height);
    REQUIRE(comp_img.id_to_color.size() == comp_img_copy.id_to_color.size());
    REQUIRE(comp_img.color_to_id.size() == comp_img_copy.color_to_id.size());
    REQUIRE(comp_img.image_data.size() == comp_img_copy.image_data.size());
    for (size_t i = 0; i < comp_img.height; ++i) {
        REQUIRE(comp_img.image_data[i].size() == comp_img_copy.image_data[i].size());
    }

    for (size_t i = 0; i < comp_img.height; ++i) {
        for (size_t j = 0; j < comp_img.width; ++j) {
            REQUIRE(
                comp_img.id_to_color[comp_img.image_data[i][j]]
                == comp_img_copy.id_to_color[comp_img_copy.image_data[i][j]]);
            REQUIRE(
                comp_img.color_to_id[comp_img_copy.id_to_color[comp_img_copy.image_data[i][j]]]
                == comp_img_copy.image_data[i][j]);
        }
    }

    UncompressedImage img_copy = toUncompressed(comp_img_copy);
    REQUIRE(img.width == img_copy.width);
    REQUIRE(img.height == img_copy.height);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(img.image_data[i][j] == img_copy.image_data[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Conversion to compressed image and back (without color add)") {
    constexpr size_t TEST_AWARD_POINTS = 6;
    openLogFile("logs/test_26.log", true);

    UncompressedImage img = loadFromBMP("images/red_cross.bmp");
    std::map<uint8_t, ColorRGB> color_map = {{0, {0, 0, 0}}, {1, {255, 0, 0}}};
    CompressedImage comp_img = toCompressed(img, color_map, false, true);

    REQUIRE(comp_img.width == img.width);
    REQUIRE(comp_img.height == img.height);
    REQUIRE(comp_img.id_to_color.size() == 2);
    REQUIRE(comp_img.color_to_id.size() == 2);
    REQUIRE(comp_img.image_data.size() == img.height);
    for (size_t i = 0; i < img.height; ++i) {
        REQUIRE(comp_img.image_data[i].size() == img.width);
    }

    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(
                comp_img.id_to_color[comp_img.image_data[i][j]]
                == color_map[comp_img.image_data[i][j]]);
            REQUIRE(
                comp_img.color_to_id[color_map[comp_img.image_data[i][j]]]
                == comp_img.image_data[i][j]);
        }
    }

    UncompressedImage img_copy = toUncompressed(comp_img);
    REQUIRE(img.width == img_copy.width);
    REQUIRE(img.height == img_copy.height);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            REQUIRE(img.image_data[i][j] == img_copy.image_data[i][j]);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}

TEST_CASE("Conversion to compressed image and back (without color add, with approximation)") {
    constexpr size_t TEST_AWARD_POINTS = 6;
    openLogFile("logs/test_26.log", true);

    UncompressedImage img = loadFromBMP("images/red_cross.bmp");
    std::map<uint8_t, ColorRGB> color_map = {{0, {10, 10, 10}}, {1, {200, 10, 10}}};
    CompressedImage comp_img = toCompressed(img, color_map, true, true);

    REQUIRE(comp_img.width == img.width);
    REQUIRE(comp_img.height == img.height);
    REQUIRE(comp_img.id_to_color.size() == 2);
    REQUIRE(comp_img.color_to_id.size() == 2);
    REQUIRE(comp_img.image_data.size() == img.height);
    for (size_t i = 0; i < img.height; ++i) {
        REQUIRE(comp_img.image_data[i].size() == img.width);
    }

    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            if (i == j || i == img.height - j - 1) {
                REQUIRE(comp_img.id_to_color[comp_img.image_data[i][j]] == color_map[1]);
            } else {
                REQUIRE(comp_img.id_to_color[comp_img.image_data[i][j]] == color_map[0]);
            }
        }
    }

    UncompressedImage img_copy = toUncompressed(comp_img);
    REQUIRE(img.width == img_copy.width);
    REQUIRE(img.height == img_copy.height);
    for (size_t i = 0; i < img.height; ++i) {
        for (size_t j = 0; j < img.width; ++j) {
            int64_t dist_sq = colorDistanceSq(img.image_data[i][j], img_copy.image_data[i][j]);
            REQUIRE(dist_sq < 3500);
        }
    }

    closeLogFile();
    awarder.awardPoints(TEST_AWARD_POINTS, Catch::getResultCapture().getCurrentTestName());
}
