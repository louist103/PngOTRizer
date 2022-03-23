#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <png.h>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>

enum class TextureType
{
    Error,
    RGBA32bpp,
    RGBA16bpp,
    Palette4bpp,
    Palette8bpp,
    Grayscale4bpp,
    Grayscale8bpp,
    GrayscaleAlpha4bpp,
    GrayscaleAlpha8bpp,
    GrayscaleAlpha16bpp,
};

class Texture {
public:
    std::vector<uint8_t> processedTexture;
    uint8_t** rawPixels;
    size_t rowBytes;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t colorType;
};


static float GetPixelMultiplyer(TextureType format)
{
    switch (format)
    {
    case TextureType::Grayscale4bpp:
    case TextureType::GrayscaleAlpha4bpp:
    case TextureType::Palette4bpp:
        return 0.5f;
    case TextureType::Grayscale8bpp:
    case TextureType::GrayscaleAlpha8bpp:
    case TextureType::Palette8bpp:
        return 1;
    case TextureType::GrayscaleAlpha16bpp:
    case TextureType::RGBA16bpp:
        return 2;
    case TextureType::RGBA32bpp:
        return 4;
    default:
        return -1;
    }
}

static double GetBytesPerPixel(uint32_t colorType, uint32_t bitDepth)
{
    switch (colorType)
    {
    case PNG_COLOR_TYPE_RGBA:
        return 4 * bitDepth / 8;

    case PNG_COLOR_TYPE_RGB:
        return 3 * bitDepth / 8;

    case PNG_COLOR_TYPE_PALETTE:
        return 1 * bitDepth / 8;
    default:
        puts("Invalid colorType");
        exit(1);
        
    }
}

static void processrgba16(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);

    for (unsigned int y2 = 0; y2 < tex.height; y2++) {
        for (unsigned int x = 0; x < tex.width; x++) {
            size_t pos = ((y2 * tex.width) + x) * 2;
            
            uint8_t pixelR = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 0];
            uint8_t pixelG = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 1];
            uint8_t pixelB = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 2];
            uint8_t pixelA = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 2];

            pixelR /= 8;
            pixelG /= 8;
            pixelB /= 8;

            pixelA = (!!pixelA);
            uint16_t data = (pixelR << 11) + (pixelG << 6) + (pixelB << 1) + pixelA;
            tex.processedTexture[pos + 0] = (data & 0xFF00) >> 8;
            tex.processedTexture[pos + 1] = (data & 0xFF);
        }
    }
}

static void processRgba32(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);

    for (unsigned int y = 0; y < tex.height; y++) {
        for (unsigned int x = 0; x < tex.width; x++) {
            size_t pos = ((y * tex.width) + x) * 4;
            tex.processedTexture[pos + 0] = tex.rawPixels[y][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 0];
            tex.processedTexture[pos + 1] = tex.rawPixels[y][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 1];
            tex.processedTexture[pos + 2] = tex.rawPixels[y][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 2];
            tex.processedTexture[pos + 3] = tex.rawPixels[y][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 3];
        }
    }
}

static void processIa16(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height );
    
    for (unsigned int y2 = 0; y2 < tex.height; y2++) {
        for (unsigned int x = 0; x < tex.width; x++) {
            size_t pos = ((y2 * tex.width) + x) * 2;
            uint8_t cR = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 0];
            uint8_t aR = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 3];
            
            tex.processedTexture[pos + 0] = cR;
            tex.processedTexture[pos + 1] = aR;

        }
    }
}

static void processI4(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);
    
    for (unsigned int y2 = 0; y2 < tex.height; y2++) {
        for (unsigned int x = 0; x < tex.width; x+=2) {
            size_t pos = ((y2 * tex.width) + x) / 2;

            uint8_t r1 = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 0];
            uint8_t r2 = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 4];

            tex.processedTexture[pos] = (uint8_t)(((r1 / 16) << 4) + (r2 / 16));
        }
    }
}

static void processI8(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);

    for (unsigned int y = 0; y < tex.height; y++) {
        for (unsigned int x = 0; x < tex.width; x++) {
            size_t pos = (y * tex.width) + x;

            tex.processedTexture[pos] = tex.rawPixels[y][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 0];
        }
    }
}

static void processIA4(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);

    for (unsigned int y = 0; y < tex.height; y++) {
        for (unsigned int x = 0; x < tex.width; x += 2) {
            size_t pos = ((y * tex.width) + x) / 2;
            uint8_t data = 0;

            for (unsigned int i = 0; i < 2; i++) {
                uint8_t cR = tex.rawPixels[y][(i + x) * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 0];
                uint8_t aR = tex.rawPixels[y][(i + x) * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 3];

                uint8_t alphaBit = aR != 0;

                if (i == 0) {
                    data |= (((cR / 32) << 1) + alphaBit) << 4;
                } else {
                    data |= ((cR / 32) << 1) + alphaBit;
                }
            }
            tex.processedTexture[pos] = data;

        }
    }
}

static void processIA8(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);

    for (unsigned int y2 = 0; y2 < tex.height; y2++) {
        for (unsigned int x = 0; x < tex.width; x++) {
            size_t pos = (y2 * tex.width) + x;

            uint8_t r = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 0];
            uint8_t a = tex.rawPixels[y2][x * (int)GetBytesPerPixel(tex.colorType, tex.depth) + 3];

            tex.processedTexture[pos] = (uint8_t)(((r / 16) << 4) + (a / 16));
        }
    }
}

static void processCi4(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);

    for (unsigned int y = 0; y < tex.height; y++) {
        for (unsigned int x = 0; x < tex.width; x += 2) {
            size_t pos = ((y * tex.width) + x) / 2;

            uint8_t cR1 = tex.rawPixels[y][x];
            uint8_t cR2 = tex.rawPixels[y][x + 1];

            tex.processedTexture[pos] = (cR1 << 4) | cR2;
        }
    }
}

static void processCi8(Texture& tex) {
    tex.processedTexture.resize(tex.rowBytes * tex.height);

    for (unsigned int y = 0; y < tex.height; y++) {
        for (unsigned int x = 0; x < tex.width; x++) {
            size_t pos = ((y * tex.width) + x);
            uint8_t cR = tex.rawPixels[y][x];

            tex.processedTexture[pos] = cR;
        }
    }
}


static TextureType getTexType(const char* cStr) {
    std::string_view str = cStr;

    if (str == "rgba32")
        return TextureType::RGBA32bpp;
    else if (str == "rgba16")
        return TextureType::RGBA16bpp;
    else if (str == "i4")
        return TextureType::Grayscale4bpp;
    else if (str == "i8")
        return TextureType::Grayscale8bpp;
    else if (str == "ia4")
        return TextureType::GrayscaleAlpha4bpp;
    else if (str == "ia8")
        return TextureType::GrayscaleAlpha8bpp;
    else if (str == "ia16")
        return TextureType::GrayscaleAlpha16bpp;
    else if (str == "ci4")
        return TextureType::Palette4bpp;
    else if (str == "ci8")
        return TextureType::Palette8bpp;
    else
        return TextureType::Error;

}

int main(int argc, char** argv) {
    Texture tex;
    TextureType texType;
    if ((argc != 4) && (argc != 3)) {
        puts("Invalid arguments");
        puts("Expecting: input file name, input file type [rgba16, rgba32, i4,i8,ia4,ia8,ia16,ci4,ci8");
        return 1;
    }

    const char* inFile = argv[1];
    FILE* fp = fopen(inFile, "rb");
    const char* format = argv[2];

    texType = getTexType(format);

    bool isColorIndexed;

    if (fp == nullptr) {
        puts("Could not open input file");
            exit(1);
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr) {
        puts("png_create_read_struct failed");
        exit(1);
    }

    png_infop info = png_create_info_struct(png);
    if (info == nullptr) {
        puts("png_create_info_struct failed");
        exit(1);
    }
    /*
    if (!setjmp(png_jmpbuf(png))) {
        puts("setjmp(png_jmpbuf(png)) failed");
        exit(1);
    }*/

    png_init_io(png, fp);

    png_read_info(png, info);

    tex.width = png_get_image_width(png, info);
    tex.height = png_get_image_height(png, info);
    tex.colorType = png_get_color_type(png, info);
    tex.depth = png_get_bit_depth(png, info);


    if (tex.depth == 16) {
        png_set_strip_16(png);
    }

    if (tex.colorType == PNG_COLOR_TYPE_PALETTE) {
        isColorIndexed = true;
    }

    if (tex.colorType == PNG_COLOR_TYPE_GRAY && tex.depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (tex.colorType == PNG_COLOR_TYPE_GRAY || tex.colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    tex.rowBytes = png_get_rowbytes(png, info);

    tex.rawPixels = (uint8_t**)malloc(sizeof(uint8_t*) * tex.height);
    
    for (size_t y = 0; y < tex.height; y++) {
        tex.rawPixels[y] = (uint8_t*)malloc(tex.rowBytes);
    }

    png_read_image(png, tex.rawPixels);

    switch (texType) {
        case TextureType::RGBA16bpp:
            processrgba16(tex);
            break;
        case TextureType::RGBA32bpp:
            processRgba32(tex);
            break;
        case TextureType::Grayscale4bpp:
            processI4(tex);
            break;
        case TextureType::Grayscale8bpp:
            processI8(tex);
            break;
        case TextureType::GrayscaleAlpha4bpp:
            processIA4(tex);
            break;
        case TextureType::GrayscaleAlpha8bpp:
            processIA8(tex);
            break;
        case TextureType::GrayscaleAlpha16bpp:
            processIa16(tex);
            break;
        case TextureType::Palette4bpp:
            processCi4(tex);
            break;
        case TextureType::Palette8bpp:
            processCi8(tex);
            break;
        case TextureType::Error:
        default:
            puts("Error");
    }
    //processrgba16();
    if (argc == 3) {
        FILE* outFile = fopen("texture", "wb+");
        uint32_t zero = 0;
        uint32_t type = 0x4F544558; //OTEX
        uint64_t id = 0x0715112907151129;
        size_t size = tex.width * tex.height * GetPixelMultiplyer(texType);

        fwrite(&zero, 4, 1, outFile); //Endianness | 0x00
        fwrite(&type, 4, 1, outFile); //Res type | 0x04
        fwrite(&zero, 4, 1, outFile); //Major version | 0x08
        fwrite(&id, 8, 1, outFile); //Asset ID | 0x0C
        fwrite(&zero, 4, 1, outFile); //Small version | 0x10
        fwrite(&zero, 8, 1, outFile); // ROM CRC | 0x14
        fwrite(&zero, 4, 1, outFile); //ROM Enum version | 0x1C

        fwrite(&zero, 28, 1, outFile); // Reserved

        fwrite(&format, 4, 1, outFile);
        fwrite(&tex.width, 4, 1, outFile);
        fwrite(&tex.height, 4, 1, outFile);

        fwrite(&size, 4, 1, outFile);

        fwrite(tex.processedTexture.data(), size, 1, outFile);

        fclose(outFile);
        fclose(fp);
    }
    else if ((argc == 4) && (strcmp(argv[3], "out_C") == 0)) {
        FILE* outFile = fopen("texture.c", "w+");
        fprintf(outFile, "unsigned char tex[] = {\n\t");
        for (auto& c : tex.processedTexture) {
            fprintf(outFile, "0x%x,", c);
        }
        fprintf(outFile, "\n};");
        fclose(outFile);
    }

    png_destroy_read_struct(&png, &info, nullptr);

}