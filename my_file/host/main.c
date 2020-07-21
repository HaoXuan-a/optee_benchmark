#include <err.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* TA API: UUID and command IDs */
#include <my_file_ta.h>

/* TEE resources */
struct test_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};
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

/*与TA会话准备部分操作，包括完成上下文初始化以及打开会话*/
void prepare_tee_session(struct test_ctx *ctx){
	TEEC_UUID uuid = TA_MY_FILE_UUID;
	uint32_t origin;
	TEEC_Result res;

	/* 初始化连接到TEE的上下文 */
	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* 打开与TA的会话 */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, origin);
}
/*会话结束后操作，包括关闭会话并结束上下文。*/
void terminate_tee_session(struct test_ctx *ctx){
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->ctx);
}

/*读取指定id的文件*/
TEEC_Result read_secure_object(struct test_ctx *ctx, char *id,char *data, size_t data_len){
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = data;
	op.params[1].tmpref.size = data_len;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_MY_FILE_CMD_READ_RAW,
				 &op, &origin);
	switch (res) {
	case TEEC_SUCCESS:
	case TEEC_ERROR_SHORT_BUFFER:
	case TEEC_ERROR_ITEM_NOT_FOUND:
		break;
	default:
		printf("Command READ_RAW failed: 0x%x / %u\n", res, origin);
	}

	return res;
}

/*向指定id文件写入数据data*/
TEEC_Result write_secure_object(struct test_ctx *ctx, char *id,char *data, size_t data_len){
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	op.params[1].tmpref.buffer = data;
	op.params[1].tmpref.size = data_len;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_MY_FILE_CMD_WRITE_RAW,
				 &op, &origin);
	if (res != TEEC_SUCCESS)
		printf("Command WRITE_RAW failed: 0x%x / %u\n", res, origin);

	switch (res) {
	case TEEC_SUCCESS:
		break;
	default:
		printf("Command WRITE_RAW failed: 0x%x / %u\n", res, origin);
	}

	return res;
}

/*删除参数id对应的文件*/
TEEC_Result delete_secure_object(struct test_ctx *ctx, char *id){
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;
	size_t id_len = strlen(id);

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = id;
	op.params[0].tmpref.size = id_len;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_MY_FILE_CMD_DELETE,
				 &op, &origin);

	switch (res) {
	case TEEC_SUCCESS:
	case TEEC_ERROR_ITEM_NOT_FOUND:
		break;
	default:
		printf("Command DELETE failed: 0x%x / %u\n", res, origin);
	}

	return res;
}

#define TEST_OBJECT_SIZE	32768   //设置文件大小为32768字节

int main(void){
	struct test_ctx ctx;
	char obj1_id[] = "object#first";		//设置文件标识id
	char obj2_id[] = "object#second";		
	char obj1_data[TEST_OBJECT_SIZE];	//设置写入文件时缓冲区大小
	char read_data[TEST_OBJECT_SIZE];	//设置读文件时缓冲区大小
	TEEC_Result res;
	/*
	*start_c,start_r,start_d,end_c,end_r,end_d定义创建、读取、删除文件object#first三种操作的时间
	*start,end为对object#first文件创建或删除操作的时间
	*/
	
	struct timeval start_c,start_r,start_d,end_c,end_r,end_d,start,end;  
	int * time1;
	int * time2;
	time1 = getCPUusage();	

	printf("Prepare session with the TA\n");
	prepare_tee_session(&ctx);

	/*
	 * 对object#first文件的三种操作：创建、读取、删除
	 */
	printf("\nTest on object \"%s\"\n", obj1_id);

	gettimeofday( &start_c, NULL );
	printf("- Create and load object in the TA secure storage\n");

	memset(obj1_data, 0xA1, sizeof(obj1_data));//obj1_data数组初始化

	res = write_secure_object(&ctx, obj1_id,
				  obj1_data, sizeof(obj1_data));//向object#first写入obj1_data的内容
	if (res != TEEC_SUCCESS)
		errx(1, "Failed to create an object in the secure storage");

	gettimeofday( &end_c, NULL );
	int time_us_c = 1000000 * ( end_c.tv_sec - start_c.tv_sec ) + end_c.tv_usec - start_c.tv_usec; 
	printf("===========================create time: %d us\n",time_us_c);
	float timeuse_c = time_us_c * 0.000001;
	printf("-----------------------------time_create: %.4f s\n",timeuse_c);

	printf("- Read back the object\n");

	gettimeofday( &start_r, NULL );

	res = read_secure_object(&ctx, obj1_id,
				 read_data, sizeof(read_data));//读取object#first
	if (res != TEEC_SUCCESS)
		errx(1, "Failed to read an object from the secure storage");
	if (memcmp(obj1_data, read_data, sizeof(obj1_data))) //比较内存区域obj1_data和read_data内容
		errx(1, "Unexpected content found in secure storage");

	gettimeofday( &end_r, NULL );
	int time_us_r = 1000000 * ( end_r.tv_sec - start_r.tv_sec ) + end_r.tv_usec - start_r.tv_usec; 
	printf("===========================read time: %d us\n",time_us_r);
	float timeuse_r = time_us_r * 0.000001;
	printf("-----------------------------time_read: %.4f s\n",timeuse_r);

	printf("- Delete the object\n");

	gettimeofday( &start_d, NULL );
	res = delete_secure_object(&ctx, obj1_id);  //删除object#first
	if (res != TEEC_SUCCESS)
		errx(1, "Failed to delete the object: 0x%x", res);
	gettimeofday( &end_d, NULL );
	int time_us_d = 1000000 * ( end_d.tv_sec - start_d.tv_sec ) + end_d.tv_usec - start_d.tv_usec; 
	printf("===========================delete time: %d us\n",time_us_d);
	float timeuse_d = time_us_d * 0.000001;
	printf("-----------------------------time_delete: %.4f s\n",timeuse_d);
	/*
	 * 非易失性存储：如果找不到，则创建object#second，如果找到，则将其删除
	 */
	printf("\nTest on object \"%s\"\n", obj2_id);
	gettimeofday( &start, NULL );
	//读取object#second文件，若文件不存在则创建该文件；否则删除该文件
	res = read_secure_object(&ctx, obj2_id,
				  read_data, sizeof(read_data));
	if (res != TEEC_SUCCESS && res != TEEC_ERROR_ITEM_NOT_FOUND)
		errx(1, "Unexpected status when reading an object : 0x%x", res);

	if (res == TEEC_ERROR_ITEM_NOT_FOUND) {
		char data[] = "This is data stored in the secure storage.\n";

		printf("- Object not found in TA secure storage, create it.\n");

		res = write_secure_object(&ctx, obj2_id,
					  data, sizeof(data));
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to create/load an object");

	} else if (res == TEEC_SUCCESS) {
		printf("- Object found in TA secure storage, delete it.\n");

		res = delete_secure_object(&ctx, obj2_id);
		if (res != TEEC_SUCCESS)
			errx(1, "Failed to delete an object");
	}

	printf("\nWe're done, close and release TEE resources\n");
	gettimeofday( &end, NULL );
	int time_us = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec; 
	printf("===========================create time: %d us\n",time_us);
	float timeuse = time_us * 0.000001;
	printf("-----------------------------time_object2: %.4f s\n",timeuse);

	terminate_tee_session(&ctx);
	
	getRAM();
	getDisk();
	time2 = getCPUusage();
	calUsage(time1, time2); 
	return 0;
}
