#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <cpu_prime_ta.h>

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

int main(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_CPU_PRIME_UUID;
	uint32_t err_origin;

    	struct timeval start, end;	//测量调用TA程序运行时间
	int * time1;  //time1，time2记录自系统的启动开始累计到当前时刻CPU总使用时间，为了测量CPU利用率
	int * time2;
	time1 = getCPUusage();	

	/*初始化连接到TEE的上下文 */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
	printf("TEEC_InitializeContext ok~~~\n");

	/*
	打开一个会话到“ cpu_prime” TA，会话创建成功会输出TEEC_OpenSession return ok~~~ 
	 */
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	
	printf("TEEC_OpenSession ok~~~~\n");
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);
	printf("TEEC_OpenSession return ok~~~\n");
	/*
	 *通过调用TA，在TA中执行计算素数功能。
	命令ID部分的值以及如何解释参数是TA提供的接口的一部分。
	 */

	/* 清除TEEC_Operation结构*/
	memset(&op, 0, sizeof(op));
	printf("100000 prime numbers:  \n");

	/*
	 *准备参数。 在第一个参数中传递值，其余三个参数未使用。
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = 100000;   //传入参数设置范围，计算1~100000之间的素数
	
	/*
	 * TA_HELLO_WORLD_CMD_VALUE 是TA中的实际调用函数。
	 */

	gettimeofday( &start, NULL );  //记录REE切换至TEE时的时间
	res = TEEC_InvokeCommand(&sess, TA_CPU_PRIME_CMD_VALUE, &op,
				 &err_origin);

	printf("TEEC_InvokeCommand ok~~~\n");
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	gettimeofday( &end, NULL );//TA执行结束后，记录TEE切换至REE时的时间
	int time_us = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; //计算时间精确到微秒
	printf("===========================time: %d us\n",time_us);
	float timeuse = time_us * 0.000001;

	printf("-----------------------------time: %.4f s\n",timeuse) ;

	/*
	 *完成了TA调用，关闭会话并结束上下文。
	 */

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);
	
	getRAM();
	getDisk();

	time2 = getCPUusage();
	calUsage(time1, time2);  

	return 0;
}
