#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <my_pipe_ta.h>
/*获取自系统的启动开始累计到调用该函数时刻CPU总使用时间*/
int * getCPUusage(){
	FILE *fp = NULL;
	
	int time1_0 = 0, time1_2 = 0, time1_3 = 0;
	char read1[100];
	
	int *time;
	time = (int *)malloc(3*sizeof(int));

	fp = fopen("/proc/stat", "r");
	fgets(read1,sizeof(read1),fp);
	
	int i = 5;
	while(read1[i] != ' '){
		time1_0 *= 10;
		time1_0 += read1[i] -'0';
		i ++;	
	}

	*(time + 0) = time1_0;
	i ++;
	while(read1[i] != ' '){
		i ++;	
	}
	i ++;
		
	while(read1[i] != ' '){
		time1_2 *= 10;
		time1_2 += read1[i] -'0';
		i ++;	
	}

	*(time + 1) = time1_2;
	i ++;
	while(read1[i] != ' '){
		time1_3 *= 10;
		time1_3 += read1[i] -'0';
		i ++;	
	}	

	*(time + 2) = time1_3;	
	fclose(fp);

	return time;
}
/*根据获取cpu使用时间计算cpu利用率*/
float calUsage(int * time1,int * time2){	
	int deltaUsed = *(time2 + 0) - *(time1 + 0) + *(time2 + 1) - *(time1 + 1);
        int deltaTotal = deltaUsed + *(time2 + 2) - *(time1 + 2);
        float usage = ((float)deltaUsed / deltaTotal) * 100;
	printf("cpu usage is : %.2f%\n",usage);
	return usage;

}

/*获取RAM使用情况*/
void getRAM(){
	char buff[80];
	FILE *fp=popen("free", "r");
	fgets(buff,sizeof(buff),(FILE*)fp);
	fgets(buff,sizeof(buff),(FILE*)fp);
	printf("buff: %s\n",buff);
	pclose(fp);
	int i = 4;
	int total = 0;
	int used = 0;
	int free = 0;
	while(buff[i] == ' '){
		i ++;	
	}
	while(buff[i] != ' '){
		total *= 10;
		total += buff[i] -'0';
		i ++;	
	}
	
	printf("RAM Total is %.2f MB\n",total / 1024.0);
	while(buff[i] == ' '){
		i ++;	
	}
	while(buff[i] != ' '){
		used *= 10;
		used += buff[i] -'0';
		i ++;	
	}
	printf("RAM used is %.2f MB\n",used / 1024.0);
	while(buff[i] == ' '){
		i ++;	
	}
	while(buff[i] != ' '){
		free *= 10;
		free += buff[i] -'0';
		i ++;	
	}
	printf("RAM free is %.2f MB\n",free / 1024.0d);
}

/*获取磁盘使用率*/
void getDisk(){
	char buff[80];
	FILE *fp=popen("df -h", "r");
	fgets(buff,sizeof(buff),(FILE*)fp);
	fgets(buff,sizeof(buff),(FILE*)fp);
	fgets(buff,sizeof(buff),(FILE*)fp);
	pclose(fp);
	int i = 0;
		
	while(buff[i] == ' '){
		i ++;	
	}
	printf("DISK Total Space is: ");
	while(buff[i] != ' '){
		printf("%c",buff[i]);
		i ++;
	}
	printf("\n");
	while(buff[i] == ' '){
		i ++;	
	}
	printf("DISK Used Space is: ");
	while(buff[i] != ' '){
		printf("%c",buff[i]);
		i ++;
	}
	printf("\n");
	while(buff[i] == ' '){
		i ++;	
	}
	while(buff[i] != ' '){
		i ++;
	}
	while(buff[i] == ' '){
		i ++;	
	}
	printf("DISK Used Percentage = ");
	while(buff[i] != ' '){
		printf("%c",buff[i]);
		i ++;
	}
	printf("\n");
	
}


int main(void){
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MY_PIPE_UUID;
	uint32_t err_origin;

    	struct timeval start, end;//测量调用TA程序运行时间

	int * time1; //time1，time2记录自系统的启动开始累计到当前时刻CPU总使用时间，为了测量CPU利用率
	int * time2;
	time1 = getCPUusage();	

	/*初始化连接到TEE的上下文 */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
	printf("TEEC_InitializeContext ok~~~\n");

	/*
	打开一个会话到my_pipe_ta，会话创建成功会输出TEEC_OpenSession return ok~~~ 
	 */
	res = TEEC_OpenSession(&ctx, &sess, &uuid,TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	
	printf("TEEC_OpenSession ok~~~~\n");
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);
	printf("TEEC_OpenSession return ok~~~\n");
	

	/* 清除TEEC_Operation结构*/
	memset(&op, 0, sizeof(op));
	printf("pipe (100):  \n");

	/*
	 * 准备参数。 在第一个参数中传递值，其余三个参数未使用。
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = 100; //设置TA程序循环执行次数

	
	/*
	 * TA_MY_PIPE_CMD_RUN是TA中的所实际调用函数的标志。
	 */
	printf("Invoking TA to increment %d\n", op.params[0].value.a);
	gettimeofday( &start, NULL );
	res = TEEC_InvokeCommand(&sess, TA_MY_PIPE_CMD_RUN, &op,&err_origin);//通过op将指定的参数传递到TA中

	printf("TEEC_InvokeCommand ok~~~\n");
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",res, err_origin);

	gettimeofday( &end, NULL );
	int time_us = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 
	printf("===========================time: %d us\n",time_us);
	float timeuse = time_us * 0.000001;

	printf("-----------------------------time: %.4f s\n",timeuse) ;

	/*
	 *完成了TA调用，关闭会话并结束上下文。
	 *
	 *当会话关闭后，TA会在日志中打印"Goodbye!" 
	 */

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	getRAM();
	getDisk();
	time2 = getCPUusage();
	calUsage(time1, time2);  

	return 0;
}
