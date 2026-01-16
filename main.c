
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

#define JIT_IMPLEMENTATION
#include "jit.h"


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
bool doJit = false;
bool specialTree = false;
int32_t imageSize = 400;
int32_t stackDepth = 10;
char *fileName = "filename.png";


typedef enum OP{
	OP_RAW,
	OP_X,
	OP_Y,
	//
	OP_TRI,
	//
	OP_SIN,
	OP_SQRT,
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
		rnd2 = (random() & 0x407fffff) | 0x40000000;
		col = *(float*)(&rnd2);
		col -= 3.0;
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
		out.g = tree->color.r;
		out.b = tree->color.r;
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
	} else if(tree->operation == OP_SQRT){
		c1 = collapsTree(tree->down[0],x,y);
		if(c1.r > 0)out.r = sqrtf(c1.r);
		else out.r = -sqrtf(-c1.r);
		if(c1.g > 0)out.g = sqrtf(c1.g);
		else out.g = -sqrtf(-c1.g);
		if(c1.b > 0)out.b = sqrtf(c1.b);
		else out.b = -sqrtf(-c1.b);
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
		printf("RAW(%f)",tree->color.r);
	}else if(tree->operation == OP_X){
		fputs("X",stdout);
	}else if(tree->operation == OP_Y){
		fputs("Y",stdout);
	}else if(tree->operation == OP_TRI){
		fputs("TRI(",stdout);
		printTree(tree->down[0]);
		fputs(",",stdout);
		printTree(tree->down[1]);
		fputs(",",stdout);
		printTree(tree->down[2]);
		fputs(")",stdout);
	}else if(tree->operation == OP_SIN){
		fputs("SIN(",stdout);
		printTree(tree->down[0]);
		fputs(")",stdout);
	}else if(tree->operation == OP_SQRT){
		fputs("SQRT(",stdout);
		printTree(tree->down[0]);
		fputs(")",stdout);
	}else if(tree->operation == OP_ADD){
		fputs("ADD(",stdout);
	}else if(tree->operation == OP_SUB){
		fputs("SUB(",stdout);
	}else if(tree->operation == OP_MUL){
		fputs("MUL(",stdout);
	}else if(tree->operation == OP_DIV){
		fputs("DIV(",stdout);
	}else if(tree->operation == OP_MOD){
		fputs("MOD(",stdout);
	}else if(tree->operation == OP_DOT){
		fputs("DOT(",stdout);
	}else if(tree->operation == OP_CROSS){
		fputs("CROSS(",stdout);
	}
	if(tree->operation >= OP_ADD){
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
	printf("%s [OPTION]\n",fileName);
	printf("file=filename.png -(fl) to where the generated file should be stored\n");
	printf("stack=10          -(st) how deep the nested expressions should get\n");
	printf("size=400          -(sz) how big the output image should be\n");
	printf("seed=???          - set a seed to controll the generator\n");

}

void compileTreeInsert(JitBuffer *buf,Node *tree){
	jit_buffer_mark(buf,128 | (buf->flags & ~256));
	jit_append_cStr(buf,"\x48\x83\xec\x10"); // sub rsp,0x10
	// \x48\x81\xec\x??\x??\x??\x?? // sub rsp,0x????????
	if(tree->operation == OP_RAW){
		jit_append_cStr(buf,"\xc7\x04\x24"); // mov DWORD [rsp     ], ??
		jit_append_lenStr(buf,(char*)&(tree->color.r),sizeof(float));
		jit_append_cStr(buf,"\xc7\x44\x24\x04"); // mov DWORD [rsp +  4], ??
		jit_append_lenStr(buf,(char*)&(tree->color.r),sizeof(float));
		jit_append_cStr(buf,"\xc7\x44\x24\x08"); // mov DWORD [rsp +  8], ??
		jit_append_lenStr(buf,(char*)&(tree->color.r),sizeof(float));
		jit_append_cStr(buf,"\xc7\x44\x24\x0c"); // mov DWORD [rsp + 12], ??
		jit_append_lenStr(buf,(char*)&(tree->color.r),sizeof(float));
		//jit_append_lenStr(buf,(char*)&(tree->color.r),sizeof(float));
		jit_append_cStr(buf,"\x0f\x28\x04\x24"); // movaps xmm0,[rsp]
		// \xc7\x04\x24\x??\x??\x??\x??
		// \xc7\x44\x24\x04\x??\x??\x??\x??
		// \xc7\x44\x24\x08\x??\x??\x??\x??
		// \xc7\x44\x24\x0c\x??\x??\x??\x??
	}else if(tree->operation == OP_X){
		jit_append_cStr(buf,"\x0f\x28\x45\xe0"); // movaps xmm0,[rsp - 32]
	}else if(tree->operation == OP_Y){
		jit_append_cStr(buf,"\x0f\x28\x45\xf0"); // movaps xmm0,[rsp - 16]
	}else if(tree->operation == OP_TRI){
		//movss DWORD [rsp],xmm0
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\xf3\x0f\x11\x04\x24");
		//movss DWORD [rsp+0x4],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04");
		//movss DWORD [rsp+0x8],xmm0
		compileTreeInsert(buf,tree->down[2]);
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04");
		//movaps xmm0,XMMWORD PTR [rsp]
		jit_append_cStr(buf,"\x0f\x28\x04\x24");
	}else if(tree->operation == OP_SIN){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x48\xb8"); // mov rax, &sinf
		float (*refing)(float);
		refing = sinf;
		jit_append_lenStr(buf,(char*)&refing,sizeof(size_t));
		jit_append_cStr(buf,"\xff\xd0"); // call rax
	}else if(tree->operation == OP_SQRT){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x51\xc0"); // sqrtps xmm1,xmm1
	}else if(tree->operation == OP_ADD){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x58\xc1"); // addss xmm1,xmm0
	}else if(tree->operation == OP_SUB){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x5c\xc1"); // subps xmm0,xmm1
	}else if(tree->operation == OP_MUL){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x59\xc1"); // mulps xmm0,xmm1
	}else if(tree->operation == OP_DIV){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x5e\xc1"); // divps xmm0,xmm1
	}else if(tree->operation == OP_MOD){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		//
		jit_append_cStr(buf,"\x48\xb8"); // mov rax, &sinf
		float (*refing)(float,float);
		refing = fmodf;
		jit_append_lenStr(buf,(char*)&refing,sizeof(size_t));
		jit_append_cStr(buf,"\xff\xd0"); // call rax
		// TODO
		fputs("MOD(",stdout);
	}else if(tree->operation == OP_DOT){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x59\xc8"); // mulps xmm1,xmm0
		// boring way!
		jit_append_cStr(buf,"\x0f\x28\xc1"); // movaps xmm0,xmm1
		jit_append_cStr(buf,"\x0f\xc6\xc1\x55"); // shufps xmm0,xmm1,0x55
		jit_append_cStr(buf,"\xf3\x0f\x58\xc1"); // addss  xmm0,xmm1
		jit_append_cStr(buf,"\x0f\x15\xc9"); // unpckhps xmm1,xmm1
		jit_append_cStr(buf,"\xf3\x0f\x58\xc1"); // addss  xmm0,xmm1
	}else if(tree->operation == OP_CROSS){
		fputs("CROSS(",stdout);
	}
	jit_buffer_mark(buf,256 | (buf->flags & ~128));
	jit_append_cStr(buf,"\x48\x83\xc4\x10"); // add rsp,0x10
	// \x48\x81\xc4\x??\x??\x??\x?? // add rsp,0x????????
}

JitCode *compileTree(Node *tree){
	JitBuffer jitBuf = {0};

	JitCode *jitCall = NULL;
	jit_append_cStr(&jitBuf,"\x55"); // push rbp
	jit_append_cStr(&jitBuf,"\x48\x89\xe5"); // mov rbp,rsp
	jit_append_cStr(&jitBuf,"\x48\xff\xcc"); // dec rsp
	jit_append_cStr(&jitBuf,"\x48\x83\xe4\xf0"); // and rsp,-0x10
	jit_append_cStr(&jitBuf,"\x48\x89\x2c\x24"); // mov [rsp],rbp
	jit_append_cStr(&jitBuf,"\x48\x89\xe5"); // mov rbp,rsp
	// put x on the stack
	jit_append_cStr(&jitBuf,"\x48\x83\xec\x10"); // sub rsp,0x10
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x04\x24"    ); // movps DW [rsp     ], xmm0
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x44\x24\x04"); // movps DW [rsp +  4], xmm0
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x44\x24\x08"); // movps DW [rsp +  8], xmm0
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x44\x24\x0c"); // movps DW [rsp + 12], xmm0
	// put y on the stack
	jit_append_cStr(&jitBuf,"\x48\x83\xec\x10"); // sub rsp,0x10
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x0c\x24"    ); // movps DW [rsp     ], xmm0
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x4c\x24\x04"); // movps DW [rsp +  4], xmm0
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x4c\x24\x08"); // movps DW [rsp +  8], xmm0
	jit_append_cStr(&jitBuf,"\xf3\x0f\x11\x4c\x24\x0c"); // movps DW [rsp + 12], xmm0
	// hello!

	compileTreeInsert(&jitBuf,tree);

	// return out
	// un store x and y from the stack
	jit_append_cStr(&jitBuf,"\x0f\x28\xc8"); // movaps xmm1, xmm0
	/* /
	float val = 1.0;
	jit_append_cStr(&jitBuf,"\xc7\x04\x24"); // mov DWORD [rsp     ], ??
	jit_append_lenStr(&jitBuf,(char*)&val,sizeof(float));
	jit_append_cStr(&jitBuf,"\xc7\x44\x24\x04"); // mov DWORD [rsp +  4], ??
	jit_append_lenStr(&jitBuf,(char*)&val,sizeof(float));
	jit_append_cStr(&jitBuf,"\xc7\x44\x24\x08"); // mov DWORD [rsp +  8], ??
	jit_append_lenStr(&jitBuf,(char*)&val,sizeof(float));
	jit_append_cStr(&jitBuf,"\xc7\x44\x24\x0c"); // mov DWORD [rsp + 12], ??
	jit_append_lenStr(&jitBuf,(char*)&val,sizeof(float));
	jit_append_cStr(&jitBuf,"\x0f\x28\x04\x24"); // movaps xmm0,[rsp]
	// */
	jit_append_cStr(&jitBuf,"\x48\x83\xc4\x20"); // add rsp,0x20
	jit_append_cStr(&jitBuf,"\x5d"); // pop rbp
	jit_append_cStr(&jitBuf,"\x48\x89\xec"); // mov rsp,rbp
	jit_append_cStr(&jitBuf,"\x5d"); // pop rbp
	jit_append_cStr(&jitBuf,"\xc3"); // ret
	// finalise code!
	jitCall = jit_create_code(&jitBuf);
	// ending
	free(jitBuf.memory);
	return jitCall;
}

Node *customTree(void){
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
	uint32_t seed = time(0);
	getArgumentUInt32(argc,argv,"st=",&stackDepth);
	getArgumentUInt32(argc,argv,"sz=",&imageSize);
	getArgumentString(argc,argv,"fl=",&fileName);
	getArgumentUInt32(argc,argv,"stack=",&stackDepth);
	getArgumentUInt32(argc,argv,"size=",&imageSize);
	getArgumentString(argc,argv,"file=",&fileName);
	getArgumentUInt32(argc,argv,"seed=",&seed);
	printf("stack: %i\n",stackDepth);
	printf("filename: %s\n",fileName);
	printf("imageSize: %i\n",imageSize);
	printf("seed: %u\n",seed);
	srandom(seed);
	Node *tree;
	if(specialTree) tree = customTree();
	else tree = createTree(stackDepth,1);
	printTree(tree);
	puts("");
	Color c;
	//
	JitCode *jitCall;
	if(doJit){
		jitCall = compileTree(tree);
		if(jitCall == NULL){
			assert(0 && "error!\n");
		}
	}
	//
	float curX;
	float curY;
	pixels = malloc(sizeof(uint32_t) * imageSize * imageSize);
	for(int32_t ox = 0;ox < imageSize;ox++){
		printf("%i\b\b\b\b\b\b",ox);
		for(int32_t oy = 0;oy < imageSize;oy++){
			curX = (((float)ox) / imageSize) * 2.0F - 1.0F;
			curY = (((float)oy) / imageSize) * 2.0F - 1.0F;
			if(doJit){
				c = (jitCall->run)(tree,curX,curY);
			}
			else
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
