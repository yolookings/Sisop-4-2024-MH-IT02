#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wand/magick_wand.h>
#include <unistd.h>

void add_watermark(const char* input_image_path, const char* output_image_path, const char* watermark_text) {
    MagickWand *wand;
    MagickBooleanType status;
    
    MagickWandGenesis();
    wand = NewMagickWand();
    
    status = MagickReadImage(wand, input_image_path);
    if (status == MagickFalse) {
        exit(EXIT_FAILURE);
    }
    
    Draw *draw;
    draw = NewDrawingWand();
    PixelWand *pixel;
    pixel = NewPixelWand();
    PixelSetColor(pixel, "white");
    DrawSetFillColor(draw, pixel);
    DrawSetFont(draw, "Arial");
    DrawSetFontSize(draw, 36);
    DrawAnnotation(draw, 10, 10, (const unsigned char*)watermark_text);
    MagickDrawImage(wand, draw);
    DestroyPixelWand(pixel);
    DestroyDrawingWand(draw);
    
    status = MagickWriteImage(wand, output_image_path);
    if (status == MagickFalse) {
        exit(EXIT_FAILURE);
    }
    
    wand = DestroyMagickWand(wand);
    MagickWandTerminus();
}

void remove_directory_content(const char* directory_path) {
    if (system("rm -rf gallery/*") == -1) {
        exit(EXIT_FAILURE);
    }
}

void reverse_file_content(const char* file_path) {
    FILE *file = fopen(file_path, "r+");
    if (file == NULL) {
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(fileSize + 1);
    if (content == NULL) {
        exit(EXIT_FAILURE);
    }
    fread(content, 1, fileSize, file);
    content[fileSize] = '\0';

    for (long i = 0, j = fileSize - 1; i < j; i++, j--) {
        char temp = content[i];
        content[i] = content[j];
        content[j] = temp;
    }

    fseek(file, 0, SEEK_SET);
    fwrite(content, 1, fileSize, file);
    fclose(file);
    free(content);
}

int main() {
    const char* input_image_path = "gallery/ikk.jpeg";
    const char* output_image_path = "gallery/wm-foto/ikk_watermarked.jpeg";
    const char* watermark_text = "inikaryakita.id";
    add_watermark(input_image_path, output_image_path, watermark_text);
    
    const char* directory_path = "gallery";
    remove_directory_content(directory_path);
    
    const char* file_path = "test_example.txt";
    reverse_file_content(file_path);
    
    return 0;
}
