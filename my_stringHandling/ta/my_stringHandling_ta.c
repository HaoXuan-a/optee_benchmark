#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <string.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <my_stringHandling_ta.h>

#define N 2048//固定字符串长度为2048

/*字符串拼接操作：将字符串source接到字符串target的后面*/
void strcat_new(char * strD, const char * strS) {
	char * add = strD;
	assert(strD != NULL && strS != NULL);
 
		while(*strD != '\0')
			strD++;
		while((*strS) != '\0') {
			*strD = *strS;
			strD++;
			strS++;
		
		*strD = '\0';
	 
		//while(*strD++ = *strS++);//cat  more efficient
 	}

}

/*字符串逆置操作  */
void strrev_new(char* s) {
    /* h指向s的头部 */  
    char* h = s;      
    char* t = s;  
    char ch; 
  
    /* t指向s的尾部 */  
    while(*t++){};  
    t--;    /* 与t++抵消 */  
    t--;    /* 回跳过结束符'\0' */  
  
    /* 当h和t未重合时，交换它们所指向的字符 */  
    while(h < t)
    {  
        ch = *h;  
        *h++ = *t;    /* h向尾部移动 */  
        *t-- = ch;    /* t向头部移动 */  
    }

} 


/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	IMSG("Hello!\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goodbye!\n");
}



static TEE_Result stringHandling(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	IMSG("Got value: %u from NW", params[0].value.a);

	IMSG("stringHandling:  ");
	
	int i;
	for(i = 0;i < params[0].value.a;i ++) {	
		// 生成随机字符串s1, 长度为N, 定义在程序最顶部, 全局变量
		int flag;
		int j, k = 0;
		char s1[N+1] = {NULL};

		for(j = 0; j < N; j ++) {
			flag = rand() % 2;
			if(flag) {
				s1[k++] = 'A' + rand() % 26;
			} else {
				s1[k++] = 'a' + rand() % 26;
			}
		}
		s1[k] = '\0';
		k = 0;

		// 生成随机字符串s2
		flag = 0;
		char s2[N+1] = {NULL};
		for(j = 0; j < N; j ++) {
			flag = rand() % 2;
			if(flag) {
				s2[k++] = 'A' + rand() % 26;
			} else {
				s2[k++] = 'a' + rand() % 26;
			}
		}
		s2[k] = '\0';
		k = 0;
		//printf("%s\n",s1);
		//printf("----------------------\n");
		//printf("%s\n",s2);
		strrev_new(s1);//字符串逆置
		strrev_new(s2);
		strcmp(s1,s2);//比较两个字符串
		strcat_new(s1,s2);//字符串拼接
		strcpy(s1,s2);//字符串拷贝
	}
	return TEE_SUCCESS;
}


/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch (cmd_id) {
	case TA_MY_STRINGHANDLING_CMD:
		return stringHandling(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
