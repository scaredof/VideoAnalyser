#include "Core/Config.h"
#include "Core/Scheduler.h"
#include "Core/Server.h"
#include "Core/Version.h" 
#include "Core/Algorithm.h"
using namespace AVSAnalyzer;

int main(int argc, char** argv)
{
#ifdef WIN32
	srand(time(NULL));//时间初始化
#endif // WIN32

	const char* file = NULL;

	for (int i = 1; i < argc; i += 2)
	{
		if (argv[i][0] != '-')
		{
			printf("parameter error:%s\n", argv[i]);
			return -1;
		}
		switch (argv[i][1])
		{
			case 'h': {
				//打印help信息
				printf("-h 打印参数配置信息并退出\n");
				printf("-f 配置文件    如：-f config.json \n");
				system("pause\n"); 
				exit(0); 
				return -1;
			}
			case 'f': {
				file = argv[i + 1];
				break;
			}
			default: {
				printf("set parameter error:%s\n", argv[i]);
				return -1;

			}
		}
	}
	
	if (file == NULL) {
		printf("failed to read config file\n");
		return -1;
	}

	Config config(file);
	if (!config.mState) {
		printf("failed to read config file: %s\n", file);
		return -1;
	}
	printf("Analyzer %s \n", PROJECT_VERSION);

	//config.show();
	printf("v3.0 发布于2023.10.23，参考视频：https://www.bilibili.com/video/BV1Xy4y1P7M2 \n");
	printf("v3.1 发布于2023.12.11，参考视频：https://www.bilibili.com/video/BV1F64y1L7dq \n");
	printf("v3.2 发布于2023.12.31，参考视频：https://www.bilibili.com/video/BV12g4y167u2 \n");
	printf("v3.3 发布于2024.04.02，参考视频：https://www.bilibili.com/video/BV1pK421h74U \n");
	printf("v3.40 发布于2024.05.09，参考视频：https://www.bilibili.com/video/BV1tH4y1G775 \n");
	printf("v3.41 发布于2024.05.20，参考视频：https://www.bilibili.com/video/BV1hJ4m1w7tP \n");
	printf("v3.42 发布于2024.07.30，参考视频：https://www.bilibili.com/video/BV1hJ4m1w7tP \n");
	printf("v3.43 发布于2024.10.02，参考视频：https://www.bilibili.com/video/BV14a2NYJE6i \n");
	printf("v3.44 发布于2024.11.25，参考视频：https://www.bilibili.com/video/BV1n9zuYKEt5 \n");
	printf("v3.45 发布于2024.12.25，参考视频：https://www.bilibili.com/video/BV1wfCBYFEVD \n");
	printf("v3.46 发布于2025.02.05，参考视频：https://www.bilibili.com/video/BV1Y7NneSEim \n");
	printf("v3.47 发布于2025.03.04，参考视频：https://www.bilibili.com/video/BV1Ed9RYaEha \n");
	printf("v3.48 发布于2025.03.26，参考视频：https://www.bilibili.com/video/BV1SFZKYnEEq \n");
	printf("v3.49（当前版本） 发布于2025.05.04 \n");

	printf("\n");
	printf("请注意! config.json有涉及路径的字段，一定要在启动前修改成自己电脑的路径，否则程序一定会报错的，如果不知道config.json各个参数代表什么意思，请参考对应视频\n");
	printf("\n");

	Scheduler scheduler(&config);
	if (!scheduler.initAlgorithm()) {
		return -1;
	}
	Server server;
	server.start(&scheduler);
	scheduler.loop();

	return 0;
}