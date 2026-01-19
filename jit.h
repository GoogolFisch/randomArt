
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

void jit_buffer_mark(JitBuffer *ins,int32_t flag);
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
	jit_append_cStr(ins,"\xc7\x04\x24"); // mov DWORD [rsp     ], ??
	jit_append_lenStr(ins,(char*)&raw,sizeof(float));
	jit_append_cStr(ins,"\xc7\x44\x24\x04"); // mov DWORD [rsp +  4], ??
	jit_append_lenStr(ins,(char*)&raw,sizeof(float));
	jit_append_cStr(ins,"\xc7\x44\x24\x08"); // mov DWORD [rsp +  8], ??
	jit_append_lenStr(ins,(char*)&raw,sizeof(float));
	jit_append_cStr(ins,"\xc7\x44\x24\x0c"); // mov DWORD [rsp + 12], ??
	jit_append_lenStr(ins,(char*)&raw,sizeof(float));
	jit_append_cStr(ins,"\x0f\x28\x04\x24"); // movaps xmm0,[rsp]
	// \xc7\x04\x24\x??\x??\x??\x??
	// \xc7\x44\x24\x04\x??\x??\x??\x??
	// \xc7\x44\x24\x08\x??\x??\x??\x??
	// \xc7\x44\x24\x0c\x??\x??\x??\x??
}

void code_append_put_store(JitBuffer *ins){
	jit_append_cStr(ins,"\x0f\x29\x04\x24"); // movaps [rsp],xmm0
}
   //#include "arch/x86_64.c"
#endif

#endif
