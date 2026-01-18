
#ifndef TREE_H_
#define TREE_H_
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
Node *createTree(int32_t depth,int32_t flags,int32_t stackDepth);
void freeTree(Node *tree);
Color collapsTree(Node *tree,float x,float y);
void printTree(Node *tree);
void compileTreeInsert(JitBuffer *buf,Node *tree);
JitCode *compileTree(Node *tree);
Node *customTreeFFile(char *name);

#endif
#ifdef TREE_IMPLEMENTATION
Node *createTree(int32_t depth,int32_t flags,int32_t stackDepth){
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
	if(flags & 1 == 1 && out->operation == OP_TRI){
		out->operation = OP_SIN + rnd % (OP_UPPER_BOUND - OP_SIN);
	}
	if(flags & 2 == 2 && out->operation <= OP_Y && depth > 0){
		rnd = random();
		out->operation = OP_SIN + rnd % (OP_UPPER_BOUND - OP_SIN);
	}
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
		flags |= 1;
		flags &= ~2;
		out->down[0] = createTree(depth - 1,flags,stackDepth);
		out->down[1] = createTree(depth - 1,flags,stackDepth);
		out->down[2] = createTree(depth - 1,flags,stackDepth);
	} else if(out->operation >= OP_DOT){
		flags |= 2;
		flags &= ~1;
		out->down[0] = createTree(depth - 1,flags,stackDepth);
		out->down[1] = createTree(depth - 1,flags,stackDepth);
	} else if(out->operation >= OP_ADD){
		out->down[0] = createTree(depth - 1,flags,stackDepth);
		out->down[1] = createTree(depth - 1,flags,stackDepth);
	} else if(out->operation >= OP_SIN){
		out->down[0] = createTree(depth - 1,flags,stackDepth);
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

void compileTreeInsert(JitBuffer *buf,Node *tree){
	code_append_stack_down(buf);
	if(tree->operation == OP_RAW){
		code_append_fetch_raw(buf,tree->color.r);
	}else if(tree->operation == OP_X){
		code_append_fetch_x(buf);
	}else if(tree->operation == OP_Y){
		code_append_fetch_y(buf);
	}else if(tree->operation == OP_TRI){
		//movss DWORD [rsp],xmm0
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		//jit_append_cStr(buf,"\xf3\x0f\x11\x04\x24");
		//movss DWORD [rsp+0x4],xmm0
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04");
		//movss DWORD [rsp+0x8],xmm0
		compileTreeInsert(buf,tree->down[2]);
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04");
		//movaps xmm0,[rsp]
		jit_append_cStr(buf,"\x0f\x28\x04\x24");
	}else if(tree->operation == OP_SIN){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		jit_append_cStr(buf,"\x48\xbb"); // mov rbx, &sinf
		float (*refing)(float);
		refing = sinf;
		jit_append_lenStr(buf,(char*)&refing,sizeof(size_t));
		jit_append_cStr(buf,"\xf3\x0f\x10\x04\x24"); // movss xmm0,[rsp]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx
		jit_append_cStr(buf,"\xf3\x0f\x11\x04\x24"); // movss [rsp],xmm0
		jit_append_cStr(buf,"\xf3\x0f\x10\x44\x24\x04"); // movss xmm0,[rsp + 4]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04"); // movss [rsp + 4],xmm0
		jit_append_cStr(buf,"\xf3\x0f\x10\x44\x24\x08"); // movss xmm0,[rsp + 8]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x08"); // movss [rsp + 8],xmm0
		//movaps xmm0,[rsp]
		jit_append_cStr(buf,"\x0f\x28\x04\x24");
	}else if(tree->operation == OP_SQRT){
		compileTreeInsert(buf,tree->down[0]);
		jit_append_cStr(buf,"\x0f\x51\xc0"); // sqrtps xmm1,xmm1
	}else if(tree->operation == OP_ADD){
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x58\xc1"); // addss xmm1,xmm0
	}else if(tree->operation == OP_SUB){
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x5c\xc1"); // subps xmm0,xmm1
	}else if(tree->operation == OP_MUL){
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x59\xc1"); // mulps xmm0,xmm1
	}else if(tree->operation == OP_DIV){
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x5e\xc1"); // divps xmm0,xmm1
	}else if(tree->operation == OP_MOD){
		//
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
		code_append_stack_down(buf);
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		//jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x48\xbb"); // mov rbx, &sinf
		float (*refing)(float,float);
		refing = fmodf;
		jit_append_lenStr(buf,(char*)&refing,sizeof(size_t));
		//jit_append_cStr(buf,"\xff\xd3"); // call rax

		jit_append_cStr(buf,"\xf3\x0f\x10\x04\x24");//movss  xmm0,[rsp]
		jit_append_cStr(buf,"\xf3\x0f\x10\x4c\x24\x10");//movss  xmm1,[rsp+0x10]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx (&fmodf)
		jit_append_cStr(buf,"\xf3\x0f\x11\x04\x24");//movss  [rsp],xmm0

		jit_append_cStr(buf,"\xf3\x0f\x10\x44\x24\x04");//movss  xmm0,[rsp+0x04]
		jit_append_cStr(buf,"\xf3\x0f\x10\x4c\x24\x14");//movss  xmm1,[rsp+0x14]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx (&fmodf)
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04");//movss  [rsp+0x04],xmm0

		jit_append_cStr(buf,"\xf3\x0f\x10\x44\x24\x08");//movss  xmm0,[rsp+0x04]
		jit_append_cStr(buf,"\xf3\x0f\x10\x4c\x24\x18");//movss  xmm1,[rsp+0x14]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx (&fmodf)
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x08");//movss  [rsp+0x08],xmm0
		//movaps xmm0,[rsp]
		jit_append_cStr(buf,"\x0f\x28\x04\x24");
		// TODO
		code_append_stack_up(buf);
	}else if(tree->operation == OP_DOT){
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
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
		compileTreeInsert(buf,tree->down[0]);
		// TODO
	}
	code_append_stack_up(buf);
}

JitCode *compileTree(Node *tree){
	JitBuffer jitBuf = {0};

	JitCode *jitCall = NULL;
	code_append_stack_init(&jitBuf);
	code_append_stack_xy(&jitBuf);

	compileTreeInsert(&jitBuf,tree);

	code_append_stack_end(&jitBuf);
	// finalise code!
	jitCall = jit_create_code(&jitBuf);
	// ending
	free(jitBuf.memory);
	return jitCall;
}

Node *customTreeWithFile(FILE *fptr){
	Node *outp = malloc(sizeof(Node));
	int32_t calc = 0;
	int32_t got = 0;
	while((got = fgetc(fptr)) != -1 && (got != '(' || calc == 0)){
		if(got == ' ' || got == ',')continue;
		calc = calc * 16 + (got & 0x0f);
		if(got == '_' || got == ')'){
			calc = 0;
			continue;
		}
		if((got & 0x1f) >= ('X' & 0x1f))
			break;
		if(got <= '9'){
			calc = 0x217;
			break;
		}
	}
	if(calc == 0x0){ // RAW
		outp->operation = OP_RAW;
		outp->color.r = 0;
		outp->color.g = 0;
		outp->color.b = 0;
	}else if(calc == 0x217){ // RAW
		outp->operation = OP_RAW;
		float flt = 0.0F;
		float scale = 1.0F;
		int32_t sign = 0;
		while((got = fgetc(fptr)) != -1){
			if(got == ')' || got == ',')break;
			if(got == '-'){
				sign |= 1;
				continue;
			}
			if(got == '.'){
				sign |= 2;
				continue;
			}
			if(sign & 2){
				scale *= 0.1F;
				flt += scale * (got - '0');
			}else
				flt = flt * 10.0F + (got - '0');
		}
		if(sign & 1 == 1)flt = -flt;
		outp->color.r = flt;
		outp->color.g = flt;
		outp->color.b = flt;
	}else if(calc == 0x8)outp->operation = OP_X;
	else if(calc == 0x9)outp->operation = OP_Y;
	else if(calc == 0x429){
		outp->operation = OP_TRI;
		outp->down[0] = customTreeWithFile(fptr);
		outp->down[1] = customTreeWithFile(fptr);
		outp->down[2] = customTreeWithFile(fptr);
	} else if(calc == 0x39e) outp->operation = OP_SIN;
	else if(calc == 0x3124) outp->operation = OP_SQRT;
	else if(calc == 0x144) outp->operation = OP_ADD;
	else if(calc == 0x352) outp->operation = OP_SUB;
	else if(calc == 0xd5c) outp->operation = OP_MUL;
	else if(calc == 0x496) outp->operation = OP_DIV;
	else if(calc == 0xdf4) outp->operation = OP_MOD;
	else if(calc == 0x4f4) outp->operation = OP_DOT;
	else if(calc == 0x32f33) outp->operation = OP_CROSS;
	if(outp->operation >= OP_SIN){
		outp->down[0] = customTreeWithFile(fptr);
	}
	if(outp->operation >= OP_ADD){
		outp->down[1] = customTreeWithFile(fptr);
	}

	return outp;
}

Node *customTreeFFile(char *name){
	FILE *fptr = fopen(name,"r");
	Node *out = customTreeWithFile(fptr);
	fclose(fptr);
	return out;
}

#endif
