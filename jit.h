
#ifndef JIT_H_
#define JIT_H_
#include<stdint.h>
#include<stdlib.h>
#include<sys/mman.h>

typedef struct {
	Color (*run)(void *memory,float curX,float curY);
	size_t len;
} JitCode;

typedef struct JitBuffer{
	size_t filled;
	size_t capacity;
	char *memory;
	size_t pos;
	size_t flags;
}JitBuffer;

void ins(JitBuffer *ins,int32_t flag);
void jit_append_buffer(JitBuffer *ins,JitBuffer *buffer);
void jit_append_lenStr(JitBuffer *ins,char *buffer,size_t adding);
void jit_append_cStr(JitBuffer *ins,char *buffer);

JitCode *jit_create_code(JitBuffer *ins);
void jit_unmap_code(JitCode *code);

void code_append_stack_init(JitBuffer *ins);
void code_append_stack_xy(JitBuffer *ins);
void code_append_stack_end(JitBuffer *ins);

void code_append_stack_down(JitBuffer *ins);
void code_append_stack_up(JitBuffer *ins);

void code_append_fetch_x(JitBuffer *ins);
void code_append_fetch_y(JitBuffer *ins);
void code_append_fetch_raw(JitBuffer *ins,float raw);

void code_append_put_store(JitBuffer *ins);
void code_append_tst_div0(JitBuffer *ins);

//

void code_append_operation_tri_sto0(JitBuffer *ins);
void code_append_operation_tri_sto1(JitBuffer *ins);
void code_append_operation_tri_load(JitBuffer *ins);

void code_append_operation_sqrt(JitBuffer *ins);
void code_append_operation_sin(JitBuffer *ins);

void code_append_operation_add(JitBuffer *ins);
void code_append_operation_sub(JitBuffer *ins);
void code_append_operation_mul(JitBuffer *ins);
void code_append_operation_div(JitBuffer *ins);
void code_append_operation_mod(JitBuffer *ins);

void code_append_operation_dot(JitBuffer *ins);
void code_append_operation_cross(JitBuffer *ins);

/*
void free_code(Code code)
{
    munmap(code.run, code.len);
}
code->len = sb.count;
code->run = mmap(NULL, sb.count, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
*/
// TODO


#endif
//
#ifdef JIT_IMPLEMENTATION
void jit_append_lenStr(JitBuffer *ins,char *buffer,size_t adding){
	if(ins->filled + adding > ins->capacity){
		if(ins->capacity < 16)
			ins->capacity = 16;
		while(ins->filled + adding > ins->capacity)
			ins->capacity *= 2;
		ins->memory = realloc(ins->memory,sizeof(char) * ins->capacity);
	}
	for(int32_t idx = 0;idx < adding;idx++){
		ins->memory[ins->filled] = buffer[idx];
		ins->filled++;
	}
}
void jit_append_buffer(JitBuffer *ins,JitBuffer *buffer){
	jit_append_lenStr(ins,buffer->memory,buffer->filled);
}
void jit_append_cStr(JitBuffer *ins,char *buffer){
	int32_t count = 0;
	while(buffer[count] != 0)count++;
	jit_append_lenStr(ins,buffer,count);
}
void jit_buffer_mark(JitBuffer *ins,int32_t flag){
	ins->flags = flag;
	ins->pos = ins->filled;
}

#if defined(_WIN32)
   //#include "platform/windows.c"
#else
   //#include "platform/posix.c"
JitCode *jit_create_code(JitBuffer *ins){
	JitCode *jc = malloc(sizeof(JitCode));
	//jc->len = ins->filled;
	jc->len = ins->capacity;
	jc->run = mmap(	NULL, jc->len,
			PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1, 0);
	if(jc->run == MAP_FAILED){
		free(jc);
		printf("Could not create running code!\n");
		return NULL;
	}
	memcpy(jc->run, ins->memory, jc->len);
	return jc;
}
int jit_free_code(JitCode *code){
	if(munmap(code->run, code->len) == -1)
		return 1;
	return 0;
}
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
   //#include "arch/aarch64.c"
#elif defined(__x86_64__) || defined(_M_AMD64)

void code_append_stack_init(JitBuffer *ins){
	jit_append_cStr(ins,"\x55"); // push rbp
	jit_append_cStr(ins,"\x53"); // push rbx
	jit_append_cStr(ins,"\x41\x54"); // push r12
	jit_append_cStr(ins,"\x41\x55"); // push r13
	jit_append_cStr(ins,"\x41\x56"); // push r14
	jit_append_cStr(ins,"\x41\x57"); // push r15
	//
	jit_append_cStr(ins,"\x48\x89\xe5"); // mov rbp,rsp
	jit_append_cStr(ins,"\x48\xff\xcc"); // dec rsp
	jit_append_cStr(ins,"\x48\x83\xe4\xf0"); // and rsp,-0x10
	jit_append_cStr(ins,"\x48\x89\x2c\x24"); // mov [rsp],rbp
	jit_append_cStr(ins,"\x48\x89\xe5"); // mov rbp,rsp
}
void code_append_stack_xy(JitBuffer *ins){
	// put x on the stack
	jit_append_cStr(ins,"\x48\x83\xec\x10"); // sub rsp,0x10
	jit_append_cStr(ins,"\xf3\x0f\x11\x04\x24"    ); // movps DW [rsp     ], xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x04"); // movps DW [rsp +  4], xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x08"); // movps DW [rsp +  8], xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x0c"); // movps DW [rsp + 12], xmm0
	// put y on the stack
	jit_append_cStr(ins,"\x48\x83\xec\x10"); // sub rsp,0x10
	jit_append_cStr(ins,"\xf3\x0f\x11\x0c\x24"    ); // movps DW [rsp     ], xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x4c\x24\x04"); // movps DW [rsp +  4], xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x4c\x24\x08"); // movps DW [rsp +  8], xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x4c\x24\x0c"); // movps DW [rsp + 12], xmm0
}
void code_append_stack_end(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\xc8"); // movaps xmm1,xmm0
	jit_append_cStr(ins,"\x0f\xc6\xc9\x06"); // shufps xmm1,xmm1, 0x06
	jit_append_cStr(ins,"\x0f\xc6\xc0\x04"); // shufps xmm0,xmm0, 0x04
	//
	jit_append_cStr(ins,"\x48\x83\xc4\x20"); // add rsp,0x20
	jit_append_cStr(ins,"\x5d"); // pop rbp
	jit_append_cStr(ins,"\x48\x89\xec"); // mov rsp,rbp
	jit_append_cStr(ins,"\x41\x5f"); // push r15
	jit_append_cStr(ins,"\x41\x5e"); // push r14
	jit_append_cStr(ins,"\x41\x5d"); // push r13
	jit_append_cStr(ins,"\x41\x5c"); // push r12
	jit_append_cStr(ins,"\x5b"); // pop rbx
	jit_append_cStr(ins,"\x5d"); // pop rbp
	jit_append_cStr(ins,"\xc3"); // ret
}
// TODO make more complicated
void code_append_stack_down(JitBuffer *ins){
	if(ins->filled - ins->pos == 4){
		if(ins->memory[ins->filled - 4] != '\x48')
			goto stack_down_new_down;
		if(ins->memory[ins->filled - 3] != '\x83')
			goto stack_down_new_down;
		if(ins->memory[ins->filled - 2] != '\xec')
			goto stack_down_tst_rem;
		if((uint32_t)ins->memory[ins->filled - 1] >= (uint32_t)'\x70')
			goto stack_down_new_down;
		ins->memory[ins->filled - 1] += '\x10';
		return;
stack_down_tst_rem:
		if(ins->memory[ins->filled-2] != '\xc4')
			goto stack_down_new_down;
		ins->memory[ins->filled - 1] -= '\x10';
		if((uint32_t)ins->memory[ins->filled - 1] == (uint32_t)'\x00'){
			ins->filled -= 4;
			ins->pos = -999;
		}
		return;
	}
stack_down_new_down:
	jit_buffer_mark(ins,128 | (ins->flags & ~256));
	jit_append_cStr(ins,"\x48\x83\xec\x10"); // sub rsp,0x10
	// \x48\x81\xec\x??\x??\x??\x?? // sub rsp,0x????????
}
void code_append_stack_up(JitBuffer *ins){
	jit_buffer_mark(ins,256 | (ins->flags & ~128));
	jit_append_cStr(ins,"\x48\x83\xc4\x10"); // add rsp,0x10
	// \x48\x81\xc4\x??\x??\x??\x?? // add rsp,0x????????
}
void code_append_fetch_x(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x45\xf0"); // movaps xmm0,[rsp - 16]
}
void code_append_fetch_y(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x45\xe0"); // movaps xmm0,[rsp - 32]
}
void code_append_fetch_raw(JitBuffer *ins,float raw){
	jit_append_cStr(ins,"\xb8"); // mov eax, ???
	jit_append_lenStr(ins,(char*)&raw,sizeof(float));
	jit_append_cStr(ins,"\x66\x0f\x6e\xc0"); // movd xmm0,eax
	jit_append_cStr(ins,"\x0f\xc6\xc0\x80"); // shufps xmm0,xmm0,0
}

void code_append_put_store(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
}
void code_append_tst_div0(JitBuffer *ins){
	float flt_1F = 1.0F;
	float flt_0F = 0.0F;
	code_append_stack_down(ins);
	code_append_put_store(ins);

	jit_append_cStr(ins,"\x66\x0f\xef\xc0"); // pxor xmm0,xmm0
	jit_append_cStr(ins,"\xf3\x0f\x10\x0c\x24"); // movss xmm1,[rsp]
	jit_append_cStr(ins,"\x0f\x2e\xc8"); // ucomiss xmm1,xmm0
	jit_append_cStr(ins,"\x75\x0f"); // jnz after ; !!!
	jit_append_cStr(ins,"\xc7\x04\x24"); // movss [rsp],(float)1.0
	jit_append_lenStr(ins,(char*)&flt_1F,sizeof(float));
	jit_append_cStr(ins,"\xc7\x44\x24\x10"); // movss [rsp+10],(float)0.0
	jit_append_lenStr(ins,(char*)&flt_0F,sizeof(float));
	// after
	jit_append_cStr(ins,"\xf3\x0f\x10\x4c\x24\x04"); // movss xmm1,[rsp+04]
	jit_append_cStr(ins,"\x0f\x2e\xc8"); // ucomiss xmm1,xmm0
	jit_append_cStr(ins,"\x75\x10"); // jnz after ; !!!
	jit_append_cStr(ins,"\xc7\x44\x24\x04"); // movss [rsp+04],(float)1.0
	jit_append_lenStr(ins,(char*)&flt_1F,sizeof(float));
	jit_append_cStr(ins,"\xc7\x44\x24\x14"); // movss [rsp+14],(float)0.0
	jit_append_lenStr(ins,(char*)&flt_0F,sizeof(float));
	// after
	jit_append_cStr(ins,"\xf3\x0f\x10\x4c\x24\x08"); // movss xmm1,[rsp+08]
	jit_append_cStr(ins,"\x0f\x2e\xc8"); // ucomiss xmm1,xmm0
	jit_append_cStr(ins,"\x75\x10"); // jnz after ; !!!
	jit_append_cStr(ins,"\xc7\x44\x24\x08"); // movss [rsp+08],(float)1.0
	jit_append_lenStr(ins,(char*)&flt_1F,sizeof(float));
	jit_append_cStr(ins,"\xc7\x44\x24\x18"); // movss [rsp+18],(float)0.0
	jit_append_lenStr(ins,(char*)&flt_0F,sizeof(float));
	//
	jit_append_cStr(ins,"\x0f\x28\x04\x24"); // movaps xmm0,[rsp]
	code_append_stack_up(ins);
}

void code_append_operation_tri_sto0(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
	//jit_append_cStr(ins,"\xf3\x0f\x11\x04\x24");
	//movss DWORD [rsp+0x4],xmm0
}
void code_append_operation_tri_sto1(JitBuffer *ins){
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x04");
	//movss DWORD [rsp+0x8],xmm0
}
void code_append_operation_tri_load(JitBuffer *ins){
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x08");
	//movaps xmm0,[rsp]
	jit_append_cStr(ins,"\x0f\x28\x04\x24");
}

void code_append_operation_sqrt(JitBuffer *ins){
	jit_append_cStr(ins,"\x66\x0f\xef\xc9"); // pxor xmm1,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x10\x04\x24"); // movss xmm0,[rsp]
	jit_append_cStr(ins,"\x0f\x2e\xc1"); // ucomiss xmm0,xmm1
	jit_append_cStr(ins,"\x77\x12"); // ja do positiv1
	jit_append_cStr(ins,"\xf3\x0f\x5c\xc8"); // subss xmm1,xmm0
	jit_append_cStr(ins,"\xf3\x0f\x51\xc9"); // sqrtss xmm1,xmm1
	jit_append_cStr(ins,"\x66\x0f\xef\xc0"); // pxor xmm1,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x5c\xc1"); // subss xmm0,xmm1
	jit_append_cStr(ins,"\xeb\x04"); // jmp after it1
	jit_append_cStr(ins,"\xf3\x0f\x51\xc0"); // sqrtps xmm0,xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x04\x24"); // movss [rsp],xmm0
	//
	jit_append_cStr(ins,"\x66\x0f\xef\xc9"); // pxor xmm1,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x10\x44\x24\x04"); // movss xmm0,[rsp+4]
	jit_append_cStr(ins,"\x0f\x2e\xc1"); // ucomiss xmm0,xmm1
	jit_append_cStr(ins,"\x77\x12"); // ja do positiv1
	jit_append_cStr(ins,"\xf3\x0f\x5c\xc8"); // subss xmm1,xmm0
	jit_append_cStr(ins,"\xf3\x0f\x51\xc9"); // sqrtss xmm1,xmm1
	jit_append_cStr(ins,"\x66\x0f\xef\xc0"); // pxor xmm1,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x5c\xc1"); // subss xmm0,xmm1
	jit_append_cStr(ins,"\xeb\x04"); // jmp after it1
	jit_append_cStr(ins,"\xf3\x0f\x51\xc0"); // sqrtps xmm0,xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x04"); // movss [rsp+4],xmm0
	//
	jit_append_cStr(ins,"\x66\x0f\xef\xc9"); // pxor xmm1,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x10\x44\x24\x08"); // movss xmm0,[rsp+8]
	jit_append_cStr(ins,"\x0f\x2e\xc1"); // ucomiss xmm0,xmm1
	jit_append_cStr(ins,"\x77\x12"); // ja do positiv1
	jit_append_cStr(ins,"\xf3\x0f\x5c\xc8"); // subss xmm1,xmm0
	jit_append_cStr(ins,"\xf3\x0f\x51\xc9"); // sqrtss xmm1,xmm1
	jit_append_cStr(ins,"\x66\x0f\xef\xc0"); // pxor xmm1,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x5c\xc1"); // subss xmm0,xmm1
	jit_append_cStr(ins,"\xeb\x04"); // jmp after it1
	jit_append_cStr(ins,"\xf3\x0f\x51\xc0"); // sqrtps xmm0,xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x08"); // movss [rsp+8],xmm0
	//
	jit_append_cStr(ins,"\x0f\x28\x04\x24"); //movaps xmm0,[rsp]
}
void code_append_operation_sin(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
	jit_append_cStr(ins,"\x48\xbb"); // mov rbx, &sinf
	float (*refing)(float);
	refing = sinf;
	jit_append_lenStr(ins,(char*)&refing,sizeof(size_t));
	jit_append_cStr(ins,"\xf3\x0f\x10\x04\x24"); // movss xmm0,[rsp]
	jit_append_cStr(ins,"\xff\xd3"); // call rbx
	jit_append_cStr(ins,"\xf3\x0f\x11\x04\x24"); // movss [rsp],xmm0
	jit_append_cStr(ins,"\xf3\x0f\x10\x44\x24\x04"); // movss xmm0,[rsp + 4]
	jit_append_cStr(ins,"\xff\xd3"); // call rbx
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x04"); // movss [rsp + 4],xmm0
	jit_append_cStr(ins,"\xf3\x0f\x10\x44\x24\x08"); // movss xmm0,[rsp + 8]
	jit_append_cStr(ins,"\xff\xd3"); // call rbx
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x08"); // movss [rsp + 8],xmm0
	//
	jit_append_cStr(ins,"\x0f\x28\x04\x24"); //movaps xmm0,[rsp]
}
void code_append_operation_add(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
	jit_append_cStr(ins,"\x0f\x58\xc1"); // addss xmm1,xmm0
}
void code_append_operation_sub(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
	jit_append_cStr(ins,"\x0f\x5c\xc8"); // subps xmm1,xmm0
	jit_append_cStr(ins,"\x0f\x28\xc1"); // movaps xmm0,xmm1
}
void code_append_operation_mul(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
	jit_append_cStr(ins,"\x0f\x59\xc1"); // mulps xmm0,xmm1
}
void code_append_operation_div(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
	jit_append_cStr(ins,"\x0f\x5e\xc8"); // divps xmm1,xmm0
	// simple solution
	jit_append_cStr(ins,"\x0f\x28\xc1"); // movaps xmm0,xmm1
}
void code_append_operation_mod(JitBuffer *ins){
	code_append_stack_down(ins);
	jit_append_cStr(ins,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
	//jit_append_cStr(ins,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
	jit_append_cStr(ins,"\x48\xbb"); // mov rbx, &sinf
	float (*refing)(float,float);
	refing = fmodf;
	jit_append_lenStr(ins,(char*)&refing,sizeof(size_t));
	//jit_append_cStr(ins,"\xff\xd3"); // call rax

	jit_append_cStr(ins,"\xf3\x0f\x10\x0c\x24");//movss  xmm1,[rsp]
	jit_append_cStr(ins,"\xf3\x0f\x10\x44\x24\x10");//movss  xmm0,[rsp+0x10]
	jit_append_cStr(ins,"\xff\xd3"); // call rbx (&fmodf)
	jit_append_cStr(ins,"\xf3\x0f\x11\x04\x24");//movss  [rsp],xmm0

	jit_append_cStr(ins,"\xf3\x0f\x10\x4c\x24\x04");//movss  xmm1,[rsp+0x04]
	jit_append_cStr(ins,"\xf3\x0f\x10\x44\x24\x14");//movss  xmm0,[rsp+0x14]
	jit_append_cStr(ins,"\xff\xd3"); // call rbx (&fmodf)
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x04");//movss  [rsp+0x04],xmm0

	jit_append_cStr(ins,"\xf3\x0f\x10\x4c\x24\x08");//movss  xmm1,[rsp+0x04]
	jit_append_cStr(ins,"\xf3\x0f\x10\x44\x24\x18");//movss  xmm0,[rsp+0x14]
	jit_append_cStr(ins,"\xff\xd3"); // call rbx (&fmodf)
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x08");//movss  [rsp+0x08],xmm0
	jit_append_cStr(ins,"\x0f\x28\x04\x24"); //movaps xmm0,[rsp]
	code_append_stack_up(ins);
}
void code_append_operation_dot(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x0c\x24"); // movaps xmm1,[rsp]
	jit_append_cStr(ins,"\x0f\x59\xc8"); // mulps xmm1,xmm0
	// boring way!
	jit_append_cStr(ins,"\x0f\x28\xc1"); // movaps xmm0,xmm1
	jit_append_cStr(ins,"\x0f\xc6\xc1\x55"); // shufps xmm0,xmm1,0x55
	jit_append_cStr(ins,"\xf3\x0f\x58\xc1"); // addss  xmm0,xmm1
	jit_append_cStr(ins,"\x0f\x15\xc9"); // unpckhps xmm1,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x58\xc1"); // addss  xmm0,xmm1
	jit_append_cStr(ins,"\xf3\x0f\x11\x04\x24"); // movss [rsp],xmm0
	// load out!
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x04"); // movss [rsp + 4],xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x08"); // movss [rsp + 8],xmm0
	jit_append_cStr(ins,"\xf3\x0f\x11\x44\x24\x0c"); // movss [rsp + 8],xmm0
	jit_append_cStr(ins,"\x0f\x28\x04\x24"); // movaps xmm0,[rsp]
}
void code_append_operation_cross(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x28\x14\x24"); // movaps xmm2,[rsp]
	jit_append_cStr(ins,"\x0f\x28\xc8"); // movaps xmm1,xmm0
	jit_append_cStr(ins,"\x0f\xc6\xc9\xc9"); // shufps xmm1,xmm1,0xc9
	jit_append_cStr(ins,"\x0f\xc6\xd2\xd2"); // shufps xmm2,xmm2,0xd2
	jit_append_cStr(ins,"\x0f\x59\xca"); // mulps xmm1,xmm2
	jit_append_cStr(ins,"\x0f\xc6\xc0\xd2"); // shufps xmm0,xmm0,0xd2
	jit_append_cStr(ins,"\x0f\xc6\xd2\xd2"); // shufps xmm2,xmm2,0xd2
	jit_append_cStr(ins,"\x0f\x59\xc2"); // mulps xmm0,xmm2
	jit_append_cStr(ins,"\x0f\x5c\xc1"); // subps xmm0,xmm1
}
   //#include "arch/x86_64.c"
#endif

#endif
