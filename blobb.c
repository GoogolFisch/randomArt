
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<assert.h>

#include<math.h>
#include<time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define ARG_PARSE_IMPLEMENTATION
#include "arg_parse.h"

typedef struct Color{
	float r;
	float g;
	float b;
	// set a to 255 / 0xFF
} Color ;
typedef union PixelStr{
	uint32_t raw;
	struct{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};
} PixelStr;

PixelStr *pixels;
PixelStr *pixels2;
bool beVerbouse     = false;

const int imageScale = 400;
int32_t imageWidth, imageHeight;
int32_t stackDepth  = 10;
int32_t delta       = 3;
uint32_t repeatCount = 100;
char *fileName      = "filename.png";
char *treeName      = NULL;

void printHelp(char *fileName){
	printf("Usage of %s\n",fileName);
	printf("Version: 0.0.0\n");
	printf("This will genereate random art and store it as an png\n");
	printf("use %s -h for help\n",fileName);
	printf("%s [OPTION]\n",fileName);
	printf("file=filename.png -(fl) to where the generated file should be stored\n");
	printf("size=..x..        - the output size of the image\n");
	printf("rep=..            - the repition count\n");
}

void setRandom(){
	for(int32_t x = 0;x < imageWidth;x++){
		for(int32_t y = 0;y < imageHeight;y++){
			pixels[x + imageWidth * y].raw = (int32_t)random();
		}
	}
}
void forEach(void (*fn)(int32_t x,int32_t y)){
	for(int32_t x = 0;x < imageWidth;x++){
		for(int32_t y = 0;y < imageHeight;y++){
			fn(x,y);
		}
	}
}
void doBlur(int32_t x,int32_t y){
	uint64_t count = 0;
	uint64_t value = 0;
	for(int32_t dx = -delta;dx <= delta;dx++){
		for(int32_t dy = -delta;dy <= delta;dy++){
			int32_t px = x + dx;
			int32_t py = y + dy;
			if(px < 0 || px >= imageWidth)continue;
			if(py < 0 || py >= imageHeight)continue;
			count++;
			value += (uint64_t)pixels[px + imageWidth * py].raw;
		}
	}
	pixels2[x + imageWidth * y].raw = (uint32_t)(value / count);
}
void doDiff(int32_t x,int32_t y){
	int64_t count = 1;
	uint64_t value = (uint64_t)pixels2[x + imageWidth * y].raw;
	value -= (uint64_t)pixels[x + imageWidth * imageHeight].raw;
	pixels[x + imageWidth * y].raw = (uint32_t)(value / count);
}
void toOutput(int32_t x,int32_t y){
	uint32_t v = (uint32_t)pixels[x + imageWidth * y].raw;
	pixels[x + imageWidth * y].a = 0xff;
	pixels[x + imageWidth * y].g = (uint8_t)(v >> 20);
	pixels[x + imageWidth * y].r = (uint8_t)(v >> 16);
	pixels[x + imageWidth * y].b = (uint8_t)(v >> 12);
}
void executeRun(){
	forEach(&doBlur);
	forEach(&doDiff);
}
int main(int32_t argc, char **argv){
	if(getArgumentExists(argc,argv,"-h")){
		printHelp(argv[0]);
		return 0;
	}
	//
	if(	getArgumentExists(argc,argv,"-v") || 
		getArgumentExists(argc,argv,"-V")){
		beVerbouse = true;
	}
	uint32_t seed = time(0);
	getArgumentString(argc,argv,"fl=",&fileName);
	getArgumentString(argc,argv,"file=",&fileName);
	getArgumentUInt32(argc,argv,"rep=",&repeatCount);
	getArgumentInt32(argc,argv,"del=",&delta);
	imageWidth = imageScale;
	imageHeight = imageScale;
	getArgumentUInt32Range(argc,argv,"size=",&imageWidth,&imageHeight);
	srandom(seed);
	pixels = malloc(sizeof(uint32_t) * imageWidth * imageHeight);
	pixels2 = malloc(sizeof(uint32_t) * imageWidth * imageHeight);
	setRandom();
	for(uint32_t counter = 0;counter < repeatCount;counter++){
		executeRun();
	}
	forEach(&toOutput);
	stbi_write_png(fileName,imageWidth,imageHeight,4 /*RGBA*/,pixels,imageWidth * sizeof(uint32_t));
}
