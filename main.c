
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

#ifndef NO_JIT_
#define JIT_IMPLEMENTATION
#include "jit.h"
#endif
#define TREE_IMPLEMENTATION
#include "tree.h"


PixelStr *pixels;
bool doJit          = false;
bool specialTree    = false;
bool doSimplifyTree = false;
bool beVerbouse     = false;
int32_t acceptMap = 0;

int32_t imageScale = 400;
int32_t imageWidth, imageHeight;
int32_t stackDepth = 10;
char *fileName = "filename.png";
char *treeName = NULL;


// 
void printHelp(char *fileName){
	printf("Usage of %s\n",fileName);
	printf("Version: 0.0.0\n");
	printf("This will genereate random art and store it as an png\n");
	printf("use %s -h for help\n",fileName);
	printf("%s [OPTION]\n",fileName);
	printf("file=filename.png -(fl) to where the generated file should be stored\n");
	printf("stack=10          -(st) how deep the nested expressions should get\n");
	printf("scale=400         -(sz) how big the scale should be\n");
	printf("seed=???          - set a seed to controll the generator\n");
	printf("tree=?            - if an tree file should be used\n");
	printf("size=..x..        - the output size of the image\n");
	printf("--jit             - to enable jit compiler\n");
	printf("--simpl           - remove bad paths\n");
	printf("--with-feat       - regenerate the tree if it is boring\n");
	printf("--with-color      - regenerate the tree if it doesn't have color\n");
	printf("--with-depth      - regenerate the tree if it doesn't have the depth\n");
	printf("--with-all        - regenerate the tree with every feature\n");
	printf("-v                - show extra information\n");
}

Node *customTreeMy(void){
	Node *out = malloc(sizeof(Node));
	out->operation = OP_ADD;
	out->down[0] = malloc(sizeof(Node));
	out->down[0]->operation = OP_SIN;
	out->down[0]->down[0] = malloc(sizeof(Node));
	out->down[0]->down[0]->operation = OP_X;
	out->down[1] = malloc(sizeof(Node));
	out->down[1]->operation = OP_Y;
	return out;
}

int main(int32_t argc, char **argv){
	if(getArgumentExists(argc,argv,"-h")){
		printHelp(argv[0]);
		return 0;
	}
	if(getArgumentExists(argc,argv,"--jit"))
		doJit = true;
	if(getArgumentExists(argc,argv,"--tree"))
		specialTree = true;
	if(getArgumentExists(argc,argv,"--simpl"))
		doSimplifyTree = true;
	//
	if(getArgumentExists(argc,argv,"--with-feat"))
		acceptMap |= 1;
	if(getArgumentExists(argc,argv,"--with-color"))
		acceptMap |= 2;
	if(getArgumentExists(argc,argv,"--with-depth"))
		acceptMap |= 4;
	if(getArgumentExists(argc,argv,"--with-all"))
		acceptMap |= 7;
	//
	if(	getArgumentExists(argc,argv,"-v") || 
		getArgumentExists(argc,argv,"-V")){
		beVerbouse = true;
	}
	uint32_t seed = time(0);
	getArgumentUInt32(argc,argv,"st=",&stackDepth);
	getArgumentUInt32(argc,argv,"sz=",&imageScale);
	getArgumentString(argc,argv,"fl=",&fileName);
	getArgumentUInt32(argc,argv,"stack=",&stackDepth);
	getArgumentUInt32(argc,argv,"scale=",&imageScale);
	getArgumentString(argc,argv,"file=",&fileName);
	getArgumentUInt32(argc,argv,"seed=",&seed);
	getArgumentString(argc,argv,"tree=",&treeName);
	imageWidth = imageScale;
	imageHeight = imageScale;
	getArgumentUInt32Range(argc,argv,"size=",&imageWidth,&imageHeight);
	srandom(seed);
	Node *tree = NULL;
	int32_t isBoring = 0;
	// gen tree
	do{
		if(tree != NULL){
			seed = random();
			freeTree(tree);
			srandom(seed);
			if(beVerbouse){
				printf("tree was boring\n");
			}
		}
		// create tree
		if(treeName != NULL)
			tree = customTreeFFile(treeName);
		else if(specialTree)
			tree = customTreeMy();
		else
			tree = createTree(stackDepth,0,stackDepth);
		// bail out
		if(tree == NULL){
			printf("ERROR! no tree could be created!\n");
			return 0;
		}
		if(doSimplifyTree)
			simplifyTree(tree);
		if(acceptMap)
			isBoring = howInterestingTree(tree,stackDepth);
		if(stackDepth < 3)isBoring |= 7;
	} while((isBoring & acceptMap) != acceptMap);
	// after gen tree
	if(beVerbouse){
		printf("stack: %i\n",stackDepth);
		printf("filename: %s\n",fileName);
		printf("scale: %i\n",imageScale);
		printf("seed: %u\n",seed);
		printf("size: %ix%i\n",imageWidth, imageHeight);
	}
	printTree(tree);
	puts("");
	Color c;
	//
#ifndef NO_JIT
	JitCode *jitCall;
	if(doJit){
		jitCall = compileTree(tree);
		if(jitCall == NULL){
			assert(0 && "error!\n");
		}
	}
#endif
	//
	float curX;
	float curY;
	pixels = malloc(sizeof(uint32_t) * imageWidth * imageHeight);
	for(int32_t ox = 0;ox < imageWidth;ox++){
		if(beVerbouse){
			printf("%i\b\b\b\b\b\b",ox);
		}
		for(int32_t oy = 0;oy < imageHeight;oy++){
			curX = (((float)ox - imageWidth / 2) / imageScale) * 2.0F;
			curY = (((float)oy - imageHeight / 2) / imageScale) * 2.0F;
#ifndef NO_JIT
			if(doJit){
				c = (jitCall->run)(tree,curX,curY);
			}
			else
#endif
				c = collapsTree(tree,curX,curY);
			pixels[ox + oy * imageWidth].r =
				(uint8_t)(((c.r + 1.0F) * 0.5F) * 256);
			pixels[ox + oy * imageWidth].g =
				(uint8_t)(((c.g + 1.0F) * 0.5F) * 256);
			pixels[ox + oy * imageWidth].b =
				(uint8_t)(((c.b + 1.0F) * 0.5F) * 256);
			pixels[ox + oy * imageWidth].a = 0xff;
		}
	}
	stbi_write_png(fileName,imageWidth,imageHeight,4 /*RGBA*/,pixels,imageWidth * sizeof(uint32_t));
	//
	if(doJit){
		if(jit_free_code(jitCall)){
			assert(0 && "freeing fail!\n");
		}
		// why does this fail?
		//free(jitCall->run);
		free(jitCall);
		//free(jitBuf);
	}
}
