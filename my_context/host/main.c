#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>


/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <my_context_ta.h>

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
/*根据获取的cpu使用时间计算cpu利用率*/
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

int main(){
	struct timeval start, end;
	int * time1;
	int * time2;
	time1 = getCPUusage();	

	float timeuse;
	pid_t pid;

	unsigned long check;
	int p1[2], p2[2];
	unsigned long iter = 10;  //设置进程执行次数
	/*创建子进程*/
	if (pipe(p1) || pipe(p2)) {
		perror("pipe create failed");
		exit(1);
	}
	gettimeofday( &start, NULL );
	pid = fork();//获取子进程id
	if (pid == -1){
		printf("Fork error \n");
	}else if (pid == 0) {

		/* child process */
	
		printf("child pid: %d\n", getpid());
		unsigned long iter1;

		iter1 = 10;
		/* slave, read p1 & write p2 */
		close(p1[1]); close(p2[0]);
		while (iter1 > 0) {
			if (read(p1[0], (char *)&check, sizeof(check)) != sizeof(check)) {
				if ((errno != 0) && (errno != EINTR))
					perror("slave read failed");
				exit(1);
			}
			if (check != iter1) {
				fprintf(stderr, "Slave sync error: expect %lu, got %lu\n",
					iter, check);
				exit(2);
			}
			if (write(p2[1], (char *)&iter1, sizeof(iter1)) != sizeof(check)) {
				if ((errno != 0) && (errno != EINTR))
					perror("slave write failed");
				exit(1);
			}
			iter1 = my_fork(iter1);//调用执行TA，实现iter1--。
		printf("iter_child: %d\n",iter1);

		}
	}else { 
		/* parent process */	
        	printf("pid: %d\n", pid);//父进程中返回子进程的pid
        	printf("father pid: %d\n", getpid());
		/* master, write p1 & read p2 */
		close(p1[0]); close(p2[1]);
		while (iter > 0) {
			if (write(p1[1], (char *)&iter, sizeof(iter)) != sizeof(iter)) {
				if ((errno != 0) && (errno != EINTR))
					perror("master write failed");
				exit(1);
			}
			if (read(p2[0], (char *)&check, sizeof(check)) != sizeof(check)) {
				if ((errno != 0) && (errno != EINTR))
					perror("master read failed");
				exit(1);
			}
			if (check != iter) {
				fprintf(stderr, "Master sync error: expect %lu, got %lu\n",
					iter, check);
				exit(2);
			}
		iter =my_fork(iter); //调用执行TA，实现iter--。

		printf("iter_father: %d\n",iter);
		}
	}
	gettimeofday( &end, NULL );
	int time_us = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 
	printf("===========================time: %d us\n",time_us);
	timeuse = time_us * 0.000001;

	printf("-----------------------------time: %.4f s\n",timeuse) ;

	getRAM();
	getDisk();
	time2 = getCPUusage();
	calUsage(time1, time2);  
	return 0;

}

int my_fork(int iter){
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MY_CONTEXT_UUID;
	uint32_t err_origin;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
	printf("TEEC_InitializeContext ok~~~\n");

	/*
	 打开一个会话到“ my_context” TA，会话创建成功会输出TEEC_OpenSession return ok~~~ 
	 */
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	
	printf("TEEC_OpenSession ok~~~~\n");
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);
	printf("TEEC_OpenSession return ok~~~\n");
	/*
	*在这种情况下，通过调用TA,执行TA中函数，实现参数减小一个数字
	*
	*命令ID部分的值以及参数如何*解释是TA提供的接口的一部分。
	 */

	/* 清除TEEC_Operation结构*/
	memset(&op, 0, sizeof(op));
	//printf("my context for 500:  \n");

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * the remaining three parameters are unused.
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = iter;//op获取参数iter的值
	
	/*
	 * TA_MY_CONTEXT_CMD 是TA中的实际调用函数标志。
	 */
	res = TEEC_InvokeCommand(&sess, TA_MY_CONTEXT_CMD, &op, &err_origin);//将参数值传递给my_context_ta.c中TA_MY_CONTEXT_CMD对应的函数中
  
	printf("TEEC_InvokeCommand ok~~~\n");
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
	printf("Invoking TA to descrement %d\n", op.params[0].value.a);
	/*
	 *完成与TA交互后，关闭会话并结束上下文。
	 *
	 *当会话关闭后，TA会在日志中打印"Goodbye!" 
	 */

	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	return op.params[0].value.a;
}
