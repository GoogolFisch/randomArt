
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

   //#include "arch/x86_64.c"
#endif

#endif
