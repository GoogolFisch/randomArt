
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
int32_t simplifyTree(Node *tree);
JitCode *compileTree(Node *tree);
Node *customTreeFFile(char *name);

// 0 => very un interessting
// 1 => interesting
// |2 => with color
// |4 => with good tree depth
int32_t howInterestingTree(Node *tree,int32_t intrestDepth);

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
		out->operation = OP_SIN + rnd % (OP_DOT - OP_SIN);
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

int32_t simplifyTree(Node *tree){
	int32_t flags = 0;
	int32_t f0,f1,f2;
	float fl0,fl1,fl2;
	Node *refrence;
	if(tree->operation >= OP_ADD){
		f0 = simplifyTree(tree->down[0]);
		f1 = simplifyTree(tree->down[1]);
		flags = ((f0 | f1) & 0xff) | (f0 & f1);
	} else if(tree->operation >= OP_SIN){
		f0 = simplifyTree(tree->down[0]);
		flags = f0;
	}
	if(tree->operation == OP_RAW){flags = 1;}
	else if(tree->operation <= OP_Y){ flags = 2; }
	else if(tree->operation == OP_TRI){
		f0 = simplifyTree(tree->down[0]);
		f1 = simplifyTree(tree->down[1]);
		f2 = simplifyTree(tree->down[2]);
		flags = 128;
		flags |= ((f0 | f1 | f2) & 0xff) | (f0 & f1 & f2);
	}
	else if(tree->operation == OP_SIN){
		if(tree->down[0]->operation == OP_RAW){
			fl0 = tree->down[0]->color.r;
			fl0 = sinf(fl0);
			free(tree->down[0]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		}
	} else if(tree->operation == OP_SQRT){
		if(tree->down[0]->operation == OP_RAW){
			fl0 = tree->down[0]->color.r;
			if(fl0 > 0) fl0 = sqrtf(fl0);
			else fl0 = -sqrtf(-fl0);
			free(tree->down[0]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		}
	}
	else if(tree->operation == OP_ADD){
		if(	tree->down[0]->operation == OP_RAW &&
			tree->down[1]->operation == OP_RAW
		){
			fl0 = tree->down[0]->color.r;
			fl0 += tree->down[1]->color.r;
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		}else if(tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 0.0F
		){
			refrence = tree->down[0];
			free(tree->down[1]);
			tree->operation = refrence->operation;
			tree->down[0] = refrence->down[0];
			tree->down[1] = refrence->down[1];
			tree->down[2] = refrence->down[2];
			free(refrence);
		}else if(tree->down[0]->operation == OP_RAW &&
			tree->down[0]->color.r == 0.0F
		){
			refrence = tree->down[1];
			free(tree->down[0]);
			tree->operation = refrence->operation;
			tree->down[0] = refrence->down[0];
			tree->down[1] = refrence->down[1];
			tree->down[2] = refrence->down[2];
			free(refrence);
		}
	}else if(tree->operation == OP_SUB){
		if(	tree->down[0]->operation == OP_RAW &&
			tree->down[1]->operation == OP_RAW
		){
			fl0 = tree->down[0]->color.r;
			fl0 -= tree->down[1]->color.r;
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		}else if(tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 0.0F
		){
			refrence = tree->down[0];
			free(tree->down[1]);
			tree->operation = refrence->operation;
			tree->down[0] = refrence->down[0];
			tree->down[1] = refrence->down[1];
			tree->down[2] = refrence->down[2];
			free(refrence);
		} else if(tree->down[0]->operation < OP_TRI &&
			tree->down[1]->operation ==
			tree->down[0]->operation
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 0;
			tree->color.g = 0;
			tree->color.b = 0;
		}
	}else if(tree->operation == OP_MUL){
		if((tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 0.0F) ||
			(tree->down[0]->operation == OP_RAW &&
			tree->down[0]->color.r == 0.0F)
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 0.0f;
			tree->color.g = 0.0f;
			tree->color.b = 0.0f;
		}else if(tree->down[0]->operation == OP_RAW &&
			tree->down[1]->operation == OP_RAW
		){
			fl0 = tree->down[0]->color.r;
			fl0 *= tree->down[1]->color.r;
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		}else if(tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 1.0F
		){
			refrence = tree->down[0];
			free(tree->down[1]);
			tree->operation = refrence->operation;
			tree->down[0] = refrence->down[0];
			tree->down[1] = refrence->down[1];
			tree->down[2] = refrence->down[2];
			free(refrence);
		}else if(tree->down[0]->operation == OP_RAW &&
			tree->down[0]->color.r == 1.0F
		){
			refrence = tree->down[1];
			free(tree->down[0]);
			tree->operation = refrence->operation;
			tree->down[0] = refrence->down[0];
			tree->down[1] = refrence->down[1];
			tree->down[2] = refrence->down[2];
			free(refrence);
		}
	}else if(tree->operation == OP_DIV){
		if((tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 0.0F) ||
			(tree->down[0]->operation == OP_RAW &&
			tree->down[0]->color.r == 0.0F)
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 0.0f;
			tree->color.g = 0.0f;
			tree->color.b = 0.0f;
		}else if(tree->down[0]->operation == OP_RAW &&
			tree->down[1]->operation == OP_RAW
		){
			fl0 = tree->down[0]->color.r;
			fl0 /= tree->down[1]->color.r;
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		}else if(tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 1.0F
		){
			refrence = tree->down[0];
			free(tree->down[1]);
			tree->operation = refrence->operation;
			tree->down[0] = refrence->down[0];
			tree->down[1] = refrence->down[1];
			tree->down[2] = refrence->down[2];
			free(refrence);
		} else if(tree->down[0]->operation < OP_TRI &&
			tree->down[1]->operation ==
				tree->down[0]->operation
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 1.0f;
			tree->color.g = 1.0f;
			tree->color.b = 1.0f;
		} else if(tree->down[0]->operation < OP_TRI &&
			tree->down[1]->operation ==
				tree->down[0]->operation
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 1.0f;
			tree->color.g = 1.0f;
			tree->color.b = 1.0f;
		}
	}else if(tree->operation == OP_MOD){
		if((tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 0.0F) ||
			(tree->down[0]->operation == OP_RAW &&
			tree->down[0]->color.r == 0.0F)
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 0.0f;
			tree->color.g = 0.0f;
			tree->color.b = 0.0f;
		}else if(tree->down[0]->operation == OP_RAW &&
			tree->down[1]->operation == OP_RAW
		){
			fl0 = tree->down[0]->color.r;
			fl0 = fmodf(fl0,tree->down[1]->color.r);
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		} else if(tree->down[0]->operation < OP_TRI &&
			tree->down[1]->operation ==
				tree->down[0]->operation
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 0;
			tree->color.g = 0;
			tree->color.b = 0;
		}
	}else if(tree->operation == OP_DOT){
		if((tree->down[1]->operation == OP_RAW &&
			tree->down[1]->color.r == 0.0F) ||
			(tree->down[0]->operation == OP_RAW &&
			tree->down[0]->color.r == 0.0F)
		){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 0.0f;
			tree->color.g = 0.0f;
			tree->color.b = 0.0f;
		}else if(tree->down[0]->operation == OP_RAW &&
			tree->down[1]->operation == OP_RAW
		){
			fl0 = tree->down[0]->color.r;
			fl0 *= tree->down[1]->color.r;
			fl0 *= 3;
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = fl0;
			tree->color.g = fl0;
			tree->color.b = fl0;
		}
		flags &= 0x7f;
	}else if(tree->operation == OP_CROSS){
		if((f0 & 128) == 0 && (f1 & 128) == 0){
			free(tree->down[0]);
			free(tree->down[1]);
			tree->operation = OP_RAW;
			tree->color.r = 0.0f;
			tree->color.g = 0.0f;
			tree->color.b = 0.0f;
		}
	}
	return flags;
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
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x08");
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
		code_append_put_store(buf);
		cope_append_operation_sqrt(buf);
		//
		jit_append_cStr(buf,"\x0f\x28\x04\x24"); // movaps xmm0,[rsp]
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
		jit_append_cStr(buf,"\x0f\x5c\xc8"); // subps xmm1,xmm0
		jit_append_cStr(buf,"\x0f\x28\xc1"); // movaps xmm0,xmm1
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
		code_append_tst_div0(buf);
		jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x0f\x5e\xc8"); // divps xmm1,xmm0
		// simple solution
		jit_append_cStr(buf,"\x0f\x28\xc1"); // movaps xmm0,xmm1
	}else if(tree->operation == OP_MOD){
		//
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
		compileTreeInsert(buf,tree->down[1]);
		code_append_tst_div0(buf);
		code_append_stack_down(buf);
		jit_append_cStr(buf,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
		//jit_append_cStr(buf,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
		jit_append_cStr(buf,"\x48\xbb"); // mov rbx, &sinf
		float (*refing)(float,float);
		refing = fmodf;
		jit_append_lenStr(buf,(char*)&refing,sizeof(size_t));
		//jit_append_cStr(buf,"\xff\xd3"); // call rax

		jit_append_cStr(buf,"\xf3\x0f\x10\x0c\x24");//movss  xmm1,[rsp]
		jit_append_cStr(buf,"\xf3\x0f\x10\x44\x24\x10");//movss  xmm0,[rsp+0x10]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx (&fmodf)
		jit_append_cStr(buf,"\xf3\x0f\x11\x04\x24");//movss  [rsp],xmm0

		jit_append_cStr(buf,"\xf3\x0f\x10\x4c\x24\x04");//movss  xmm1,[rsp+0x04]
		jit_append_cStr(buf,"\xf3\x0f\x10\x44\x24\x14");//movss  xmm0,[rsp+0x14]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx (&fmodf)
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04");//movss  [rsp+0x04],xmm0

		jit_append_cStr(buf,"\xf3\x0f\x10\x4c\x24\x08");//movss  xmm1,[rsp+0x04]
		jit_append_cStr(buf,"\xf3\x0f\x10\x44\x24\x18");//movss  xmm0,[rsp+0x14]
		jit_append_cStr(buf,"\xff\xd3"); // call rbx (&fmodf)
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x08");//movss  [rsp+0x08],xmm0
		jit_append_cStr(buf,"\x0f\x28\x04\x24"); //movaps xmm0,[rsp]
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
		jit_append_cStr(buf,"\xf3\x0f\x11\x04\x24"); // movss [rsp],xmm0
		// load out!
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x04"); // movss [rsp + 4],xmm0
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x08"); // movss [rsp + 8],xmm0
		jit_append_cStr(buf,"\xf3\x0f\x11\x44\x24\x0c"); // movss [rsp + 8],xmm0
		jit_append_cStr(buf,"\x0f\x28\x04\x24"); // movaps xmm0,[rsp]
	}else if(tree->operation == OP_CROSS){
		compileTreeInsert(buf,tree->down[0]);
		code_append_put_store(buf);
		compileTreeInsert(buf,tree->down[1]);
		jit_append_cStr(buf,"\x0f\x28\x14\x24"); // movaps xmm2,[rsp]
		jit_append_cStr(buf,"\x0f\x28\xc8"); // movaps xmm1,xmm0
		jit_append_cStr(buf,"\x0f\xc6\xc9\xc9"); // shufps xmm1,xmm1,0xc9
		jit_append_cStr(buf,"\x0f\xc6\xd2\xd2"); // shufps xmm2,xmm2,0xd2
		jit_append_cStr(buf,"\x0f\x59\xca"); // mulps xmm1,xmm2
		jit_append_cStr(buf,"\x0f\xc6\xc0\xd2"); // shufps xmm0,xmm0,0xd2
		jit_append_cStr(buf,"\x0f\xc6\xd2\xd2"); // shufps xmm2,xmm2,0xd2
		jit_append_cStr(buf,"\x0f\x59\xc2"); // mulps xmm0,xmm2
		jit_append_cStr(buf,"\x0f\x5c\xc1"); // subps xmm0,xmm1
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
			fseek(fptr,-1,SEEK_CUR);
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
	if(fptr == NULL){
		printf("ERROR! No file found!\n");
		return NULL;
	}
	Node *out = customTreeWithFile(fptr);
	fclose(fptr);
	return out;
}
int32_t testDepthTree(Node *tree){
	int32_t ttd;
	if(tree->operation == OP_TRI){
		ttd  = testDepthTree(tree->down[0]);
		ttd += testDepthTree(tree->down[1]);
		ttd += testDepthTree(tree->down[2]);
		ttd /= 3;
	} else if(tree->operation >= OP_ADD){
		ttd  = testDepthTree(tree->down[0]);
		ttd += testDepthTree(tree->down[1]);
		ttd /= 2;
	} else if(tree->operation >= OP_SIN){
		ttd  = testDepthTree(tree->down[0]);
	} else if(tree->operation >= OP_RAW){
		ttd = 0;
	}
	return ttd + 1;
}
int32_t howInterestingTree(Node *tree,int32_t intrestDepth){
	int32_t flagOut = 0;
	// test tree depth
	if(intrestDepth / 2 >= testDepthTree(tree))
		flagOut |= 4;
	if(intrestDepth < 6)flagOut |= 4;
	if(intrestDepth < 3)flagOut |= 7;
	const int arSize = 8;
	//Color testing[arSize];
	// test pixel colors
	Color c1;
	PixelStr tstPix[arSize];
	float deltas[4] = {0.6f,0.5f,0.4f,0.3f};
	int32_t pixIdx = 0;
	for(int32_t dind = 0;dind < arSize / 2;dind += 2){
		c1 = collapsTree(tree,deltas[dind],deltas[dind + 1]);
		tstPix[pixIdx].r = (uint8_t)(((c1.r + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].g = (uint8_t)(((c1.g + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].b = (uint8_t)(((c1.b + 1.0F) * 0.5F) * 256);
		pixIdx++;
		c1 = collapsTree(tree,-deltas[dind + 1],deltas[dind]);
		tstPix[pixIdx].r = (uint8_t)(((c1.r + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].g = (uint8_t)(((c1.g + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].b = (uint8_t)(((c1.b + 1.0F) * 0.5F) * 256);
		pixIdx++;
		c1 = collapsTree(tree,-deltas[dind],-deltas[dind + 1]);
		tstPix[pixIdx].r = (uint8_t)(((c1.r + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].g = (uint8_t)(((c1.g + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].b = (uint8_t)(((c1.b + 1.0F) * 0.5F) * 256);
		pixIdx++;
		c1 = collapsTree(tree,deltas[dind + 1],-deltas[dind]);
		tstPix[pixIdx].r = (uint8_t)(((c1.r + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].g = (uint8_t)(((c1.g + 1.0F) * 0.5F) * 256);
		tstPix[pixIdx].b = (uint8_t)(((c1.b + 1.0F) * 0.5F) * 256);
		pixIdx++;
	}
	const int32_t epsilon = 10;
	float err;
	int counted = 0;
	int32_t cr,cg,cb;
	for(int ox = 0;ox < arSize - 1;ox++){
		for(int oy = ox + 1;oy < arSize;oy++){
			cr = (int32_t)(tstPix[ox].r - tstPix[oy].r);
			cg = (int32_t)(tstPix[ox].g - tstPix[oy].g);
			cb = (int32_t)(tstPix[ox].b - tstPix[oy].b);
			err =  cr * cr + cg * cg + cb * cb;
			if(err < epsilon)
				// boring!
				continue;
			// count intersting
			counted++;
		}
	}
	// see: I have 6 pos diff, only 3 req => interesting(false)
	if(counted > (arSize * (arSize - 1) / 4))
		flagOut |= 1;
	// test if colored
	if(tstPix[0].r != tstPix[0].g || tstPix[0].r != tstPix[0].b)
		flagOut |= 2;
	return flagOut;
}

#endif
