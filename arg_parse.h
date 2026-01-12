
#ifndef ARG_PARSE_H_
#define ARG_PARSE_H_

#include<stdio.h>
#include<stdint.h>

int32_t getArgumentInt32 (int32_t argc,char **argv,char *baseString, int32_t *output);
int32_t getArgumentUInt32(int32_t argc,char **argv,char *baseString,uint32_t *output);
int32_t getArgumentString(int32_t argc,char **argv,char *baseString, char   **output);
int32_t getArgumentExists(int32_t argc,char **argv,char *baseString                 );
#endif

#ifdef ARG_PARSE_IMPLEMENTATION

int32_t getArgumentInt32 (int32_t argc,char **argv,char *baseString,int32_t *output){
	char *real = NULL;
	int32_t argg = getArgumentString(argc,argv,baseString,&real);
	if(real == NULL)
		return 0;
	int32_t idx = 0;
	// adding negative
	int32_t qq = 0;
	if(real[idx] == '-')
		qq = 1;
	// counting
	int32_t sd = 0;
	while(real[idx] != 0){
		if(real[idx] < '0' || real[idx] > '9'){
			idx++;continue;
		}
		sd *= 10;
		sd += real[idx] - '0';
		idx++;
	}
	// sign swap
	if(qq > 0)
		sd = -sd;
	*output = sd;
	return argg;
}
int32_t getArgumentUInt32(int32_t argc,char **argv,char *baseString,uint32_t *output){
	char *real = NULL;
	int32_t argg = getArgumentString(argc,argv,baseString,&real);
	if(real == NULL)
		return 0;
	int32_t idx = 0;
	// counting
	int32_t sd = 0;
	while(real[idx] != 0){
		if(real[idx] < '0' || real[idx] > '9'){
			idx++;continue;
		}
		sd *= 10;
		sd += real[idx] - '0';
		idx++;
	}
	*output = sd;
	return argg;
}
int32_t getArgumentString(int32_t argc,char **argv,char *baseString,char **output){
	int32_t argg,idx;
	for(argg = 0;argg < argc;argg++){
		for(idx = 0;baseString[idx] != 0;idx++){
			if(argv[argg][idx] != baseString[idx])
				break;
		}
		if(baseString[idx] != 0)
			continue;
		break;
	}
	if(argc == argg)
		return 0;

	char *real;
	real = argv[argg] + idx;
	*output = real;
	return argg;
}
int32_t getArgumentExists(int32_t argc,char **argv,char *baseString){
	int32_t argg,idx;
	for(argg = 0;argg < argc;argg++){
		for(idx = 0;baseString[idx] != 0;idx++){
			if(argv[argg][idx] != baseString[idx])
				break;
		}
		if(baseString[idx] != 0)
			continue;
		break;
	}
	if(argc == argg)
		return 0;
	return argg;
}

#endif
