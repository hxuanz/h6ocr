#include "asr.h"
extern "C"{
	#include <libavutil/opt.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/samplefmt.h>
	#include <libswresample/swresample.h>
}

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <errno.h>

#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include <fstream>

using namespace std;

typedef unsigned char uchar;

/* 上传用户词表 */
int upload_userwords()
{
	char*			userwords = NULL;
	unsigned int	len = 0;
	unsigned int	read_len = 0;
	FILE*			fp = NULL;
	int				ret = -1;

	fp = fopen("userwords.txt", "rb");
	if (NULL == fp)
	{
		printf("\nopen [userwords.txt] failed! \n");
		goto upload_exit;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp); //获取音频文件大小
	fseek(fp, 0, SEEK_SET);

	userwords = (char*)malloc(len + 1);
	if (NULL == userwords)
	{
		printf("\nout of memory! \n");
		goto upload_exit;
	}

	read_len = fread((void*)userwords, 1, len, fp); //读取用户词表内容
	if (read_len != len)
	{
		printf("\nread [userwords.txt] failed!\n");
		goto upload_exit;
	}
	userwords[len] = '\0';

	MSPUploadData("userwords", userwords, len, "sub = uup, dtt = userword", &ret); //上传用户词表
	if (MSP_SUCCESS != ret)
	{
		printf("\nMSPUploadData failed ! errorCode: %d \n", ret);
		goto upload_exit;
	}

upload_exit:
	if (NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}
	if (NULL != userwords)
	{
		free(userwords);
		userwords = NULL;
	}

	return ret;
}
#define	BUFFER_SIZE	4096
#define FRAME_LEN	640 
#define HINTS_SIZE  100

void read_av_file(string audio_file, vector<char>& av_data)
{
	ifstream is(audio_file);
	is.seekg(0, is.end);
	int length = is.tellg();
	is.seekg(0, is.beg);

	char * buffer = new char[length];
	is.read(buffer, length);
	av_data.assign(buffer, buffer + length);
	is.close();
	delete[] buffer;
}

void run_iat(vector<char>& av_data, string session_begin_params, string& result)
{
	const char*		session_id = NULL;
	char			hints[HINTS_SIZE] = { NULL }; //hints为结束本次会话的原因描述，由用户自定义
	unsigned int	total_len = 0;
	int				aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;		//音频状态
	int				ep_stat = MSP_EP_LOOKING_FOR_SPEECH;		//端点检测
	int				rec_stat = MSP_REC_STATUS_SUCCESS;			//识别状态
	int				errcode = MSP_SUCCESS;

	long pcm_count = 0;
	char *p_pcm = av_data.data();
	long pcm_size = av_data.size();
	
	printf("\n开始语音听写 ...\n");
	session_id = QISRSessionBegin(NULL, session_begin_params.c_str(), &errcode); //听写不需要语法，第一个参数为NULL
	if (MSP_SUCCESS != errcode)
	{
		printf("\nQISRSessionBegin failed! error code:%d\n", errcode);
		goto iat_exit;
	}

	while (1)
	{
		unsigned int len = 10 * FRAME_LEN; // 每次写入200ms音频(16k，16bit)：1帧音频20ms，10帧=200ms。16k采样率的16位音频，一帧的大小为640Byte
		int ret = 0;

		if (pcm_size < 2 * len)
			len = pcm_size;
		if (len <= 0)
			break;

		aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
		if (0 == pcm_count)
			aud_stat = MSP_AUDIO_SAMPLE_FIRST;

		printf(">");
		ret = QISRAudioWrite(session_id, (const void *)&p_pcm[pcm_count], len, aud_stat, &ep_stat, &rec_stat);
		if (MSP_SUCCESS != ret)
		{
			printf("\nQISRAudioWrite failed! error code:%d\n", ret);
			goto iat_exit;
		}

		pcm_count += (long)len;
		pcm_size -= (long)len;

		if (MSP_REC_STATUS_SUCCESS == rec_stat) //已经有部分听写结果
		{
			const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
			if (MSP_SUCCESS != errcode)
			{
				printf("\nQISRGetResult failed! error code: %d\n", errcode);
				goto iat_exit;
			}
			if (NULL != rslt)
			{
				unsigned int rslt_len = strlen(rslt);
				total_len += rslt_len;
				if (total_len >= BUFFER_SIZE)
				{
					printf("\nno enough buffer for rec_result !\n");
					goto iat_exit;
				}
				result += string(rslt, rslt_len);
			}
		}

		if (MSP_EP_AFTER_SPEECH == ep_stat)
			break;
		Sleep(200); //模拟人说话时间间隙。200ms对应10帧的音频
	}
	errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
	if (MSP_SUCCESS != errcode)
	{
		printf("\nQISRAudioWrite failed! error code:%d \n", errcode);
		goto iat_exit;
	}

	while (MSP_REC_STATUS_COMPLETE != rec_stat)
	{
		const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
		if (MSP_SUCCESS != errcode)
		{
			printf("\nQISRGetResult failed, error code: %d\n", errcode);
			goto iat_exit;
		}
		if (NULL != rslt)
		{
			unsigned int rslt_len = strlen(rslt);
			total_len += rslt_len;
			if (total_len >= BUFFER_SIZE)
			{
				printf("\nno enough buffer for rec_result !\n");
				goto iat_exit;
			}
			result += string(rslt, rslt_len);
		}
		Sleep(150); //防止频繁占用CPU
	}

iat_exit:

	QISRSessionEnd(session_id, hints);
}


ASR::ASR()
{
	const char* login_params = "appid = 5913cf8c, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动
	/* 用户登录 */
	int ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数	
	if (MSP_SUCCESS != ret)
	{
		printf("MSPLogin failed , Error code %d.\n", ret);
	}
}


ASR::~ASR()
{
	MSPLogout(); 
}


int ASR::convertFromat(unsigned char *src_data, unsigned char *dst_data)
{
	// TODO  通过 http://ffmpeg.org/doxygen/trunk/index.html 讲音频转为 16k采样率的wav文件
}


int ASR::recognise(vector<char>& av_data)
{
	/*
	* sub:				请求业务类型
	* domain:			领域
	* language:			语言
	* accent:			方言
	* sample_rate:		音频采样率
	* result_type:		识别结果格式
	* result_encoding:	结果编码格式
	*
	* 详细参数说明请参阅《讯飞语音云MSC--API文档》
	*/
	string session_begin_params = "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";

	//upload_userwords();  //上传用户词表成功
	run_iat(av_data, session_begin_params, result_); //iflytek02音频内容为“中美数控”；如果上传了用户词表，识别结果为：“中美速控”。

	//_getch();
	return 0;
}
