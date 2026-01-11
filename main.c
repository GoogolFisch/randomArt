
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

#include<math.h>
#include<time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


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
int32_t imageSize = 400;
int32_t stackDepth = 10;
char *fileName = "filename.png";


typedef struct Color{
	float r;
	float g;
	float b;
	// set a to 255 / 0xFF
} Color ;
typedef enum OP{
	OP_RAW,
	OP_X,
	OP_Y,
	//
	OP_TRI,
	//
	OP_SIN,
	//
	OP_ADD,
	OP_SUB,
	//
	OP_MUL,
	OP_DIV,
	OP_MOD,
	//
	OP_DOT,
	OP_CROSS,
	//
	OP_UPPER_BOUND,
	//
	/*
	OP_MUL,
	OP_DIV,
	OP_MOD,
	// */
} OP;
typedef struct Node{
	OP operation;
	union{
		Color color;
		struct Node *down[3];
	};
} Node;

Node *createTree(int32_t depth,int32_t flags){
	Node *out = malloc(sizeof(Node));
	int32_t rnd = random();
	int32_t rnd2;// = random();
	float col;
	if(depth <= 0)
		out->operation = rnd % OP_TRI;
	else if(depth >= stackDepth - 1)
		out->operation = OP_SIN + rnd % (OP_UPPER_BOUND - OP_SIN);
	else
		out->operation = rnd % OP_UPPER_BOUND;
	if(flags & 1 == 0)
		out->operation = OP_RAW;
	//
	if(out->operation == OP_RAW){
		// normalise the float?
		rnd2 = (random() & 0xbf7fffff) | 0x3f000000;
		col = *(float*)(&rnd2);
		out->color.r = col;
		out->color.g = col;
		out->color.b = col;
	} else if(out->operation == OP_TRI){
		out->down[0] = createTree(depth - 1,flags & ~1);
		out->down[1] = createTree(depth - 1,flags & ~1);
		out->down[2] = createTree(depth - 1,flags & ~1);
	} else if(out->operation >= OP_DOT){
		out->down[0] = createTree(depth - 1,flags | 1);
		out->down[1] = createTree(depth - 1,flags | 1);
	} else if(out->operation >= OP_ADD){
		out->down[0] = createTree(depth - 1,flags);
		out->down[1] = createTree(depth - 1,flags);
	} else if(out->operation >= OP_SIN){
		out->down[0] = createTree(depth - 1,flags);
	}
	return out;
}
void freeTree(Node *tree){
	if(tree->operation == OP_TRI){
		freeTree(tree->down[0]);
		freeTree(tree->down[1]);
		freeTree(tree->down[2]);
	}else if(tree->operation >= OP_ADD){
		freeTree(tree->down[0]);
		freeTree(tree->down[1]);
	}else if(tree->operation >= OP_SIN){
		freeTree(tree->down[0]);
	}
	free(tree);
}
Color collapsTree(Node *tree,float x,float y){
	Color out = {0};
	Color c1,c2,c3;
	if(tree->operation >= OP_ADD){
		c1 = collapsTree(tree->down[0],x,y);
		c2 = collapsTree(tree->down[1],x,y);
	}
	if(tree->operation == OP_RAW){
		out.r = tree->color.r;
		out.g = tree->color.g;
		out.b = tree->color.b;
	} else if(tree->operation == OP_X){
		out.r = x; out.g = x; out.b = x;
	} else if(tree->operation == OP_Y){
		out.r = y; out.g = y; out.b = y;
	} else if(tree->operation == OP_TRI){
		c1 = collapsTree(tree->down[0],x,y);
		c2 = collapsTree(tree->down[1],x,y);
		c3 = collapsTree(tree->down[2],x,y);
		out.r = c1.r;
		out.g = c2.r;
		out.b = c3.r;
	} else if(tree->operation == OP_SIN){
		c1 = collapsTree(tree->down[0],x,y);
		out.r = sinf(c1.r);
		out.g = sinf(c1.g);
		out.b = sinf(c1.b);
	} else if(tree->operation == OP_ADD){
		out.r = c1.r + c2.r;
		out.g = c1.g + c2.g;
		out.b = c1.b + c2.b;
	} else if(tree->operation == OP_SUB){
		out.r = c1.r - c2.r;
		out.g = c1.g - c2.g;
		out.b = c1.b - c2.b;
	} else if(tree->operation == OP_MUL){
		out.r = c1.r * c2.r;
		out.g = c1.g * c2.g;
		out.b = c1.b * c2.b;
	} else if(tree->operation == OP_DIV){
		if(c2.r == 0) out.r = 0;
		else out.r = c1.r / c2.r;
		if(c2.g == 0) out.g = 0;
		else out.g = c1.g / c2.g;
		if(c2.b == 0) out.b = 0;
		else out.b = c1.b / c2.b;
	} else if(tree->operation == OP_MOD){
		if(c2.r == 0) out.r = 0;
		else out.r = fmodf(c1.r,c2.r);
		if(c2.g == 0) out.g = 0;
		else out.g = fmodf(c1.g,c2.g);
		if(c2.b == 0) out.b = 0;
		else out.b = fmodf(c1.b,c2.b);
	} else if(tree->operation == OP_DOT){
		out.r = c1.r * c2.r + c1.g * c2.g + c1.b * c2.b;
		out.g = out.r;
		out.b = out.r;
	} else if(tree->operation == OP_CROSS){
		out.r = c1.g * c2.b - c2.g * c1.b;
		out.g = c1.b * c2.r - c2.b * c1.r;
		out.b = c1.r * c2.g - c2.r * c1.g;
	}
	return out;
}
void printTree(Node *tree){
	if(tree->operation == OP_RAW){
		printf("OP_RAW(%f,%f,%f)",tree->color.r,tree->color.g,tree->color.b);
	}else if(tree->operation == OP_X){
		fputs("X",stdout);
	}else if(tree->operation == OP_Y){
		fputs("Y",stdout);
	}else if(tree->operation == OP_TRI){
		fputs("OP_TRI(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(",",stdout);
		printTree(tree->down[2]);
		fputs(")",stdout);
	}else if(tree->operation == OP_SIN){
		fputs("OP_SIN(",stdout);
		printTree(tree->down[0]);
		fputs(")",stdout);
	}else if(tree->operation == OP_ADD){
		fputs("OP_ADD(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(")",stdout);
	}else if(tree->operation == OP_SUB){
		fputs("OP_SUB(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(")",stdout);
	}else if(tree->operation == OP_MUL){
		fputs("OP_MUL(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(")",stdout);
	}else if(tree->operation == OP_DIV){
		fputs("OP_DIV(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(")",stdout);
	}else if(tree->operation == OP_MOD){
		fputs("OP_MOD(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(")",stdout);
	}else if(tree->operation == OP_DOT){
		fputs("OP_DOT(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(")",stdout);
	}else if(tree->operation == OP_CROSS){
		fputs("OP_CROSS(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(")",stdout);
	}
}

void printHelp(char *fileName){
	printf("Usage of %s\n",fileName);
	printf("This will genereate random art and store it as an png\n");
	printf("use %s -h for help\n",fileName);
	printf("%s [fileName] [stackDepth=10] [imageSize=400]\n",fileName);
	printf("fileName - to where the generated file should be stored\n");
	printf("stackDepth - how deep the nested expressions should get\n");
	printf("imageSize - how big the output image should be\n");
	printf("NOTE: doesn't support output to stdout\n");
}


int main(int32_t argc, char **argv){
	if(argc > 1){
		if(argv[1][0] == 0){
			printHelp(argv[0]);
			return 0;
		}
		if(argv[1][0] == '-' && argv[1][1] == 'h'){
			printHelp(argv[0]);
			return 0;
		}
		fileName = argv[1];
	}
	int32_t sd;
	char *agv;
	if(argc > 2){
		sd = 0;
		agv = argv[2];
		while(*agv != 0){
			if(*agv < '0' || *agv > '9'){
				agv++;
				continue;
			}
			sd *= 10;
			sd += *agv - '0';
			agv++;
		}
		if(sd != 0)
			stackDepth = sd;
	}
	if(argc > 3){
		sd = 0;
		agv = argv[3];
		while(*agv != 0){
			if(*agv < '0' || *agv > '9'){
				agv++;
				continue;
			}
			sd *= 10;
			sd += *agv - '0';
			agv++;
		}
		if(sd != 0)
			imageSize = sd;
	}
	printf("stack: %i\n",stackDepth);
	printf("filename: %s\n",fileName);
	printf("imageSize: %i\n",imageSize);
	uint32_t seed = time(0);
	srandom(seed);
	Node *tree = createTree(stackDepth,1);
	printTree(tree);
	puts("");
	Color c;
	float curX;
	float curY;
	pixels = malloc(sizeof(uint32_t) * imageSize * imageSize);
	for(int32_t ox = 0;ox < imageSize;ox++){
		printf("%i\b\b\b\b\b\b",ox);
		for(int32_t oy = 0;oy < imageSize;oy++){
			curX = (((float)ox) / imageSize) * 2.0F - 1.0F;
			curY = (((float)oy) / imageSize) * 2.0F - 1.0F;
			c = collapsTree(tree,curX,curY);
			pixels[ox + oy * imageSize].r =
				(uint8_t)(((c.r + 1.0F) * 0.5F) * 255);
			pixels[ox + oy * imageSize].g =
				(uint8_t)(((c.g + 1.0F) * 0.5F) * 255);
			pixels[ox + oy * imageSize].b =
				(uint8_t)(((c.b + 1.0F) * 0.5F) * 255);
			pixels[ox + oy * imageSize].a = 0xff;
		}
	}
	stbi_write_png(fileName,imageSize,imageSize,4 /*RGBA*/,pixels,imageSize * sizeof(uint32_t));
}
