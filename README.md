# VideoAnalyser 视频分析平台

VideoAnalyser 是一个面向单机部署的视频流管理与 AI 行为分析平台。项目由 Web 管理后台、C++ 视频分析器和流媒体服务三部分组成，可用于接入 RTSP 等视频流、配置检测算法与布控区域，并查看分析告警。

当前仓库版本为 `3.49`。

## 1. 系统组成

| 组件 | 目录/程序 | 默认端口 | 作用 |
| --- | --- | ---: | --- |
| 管理后台 | `Admin/` | 9001 | Django Web 页面、视频流/算法/布控/告警管理 |
| 分析器 | `Analyzer/` | 9003 | 拉取视频帧、运行 ONNX/OpenVINO 模型、生成告警 |
| 流媒体服务 | ZLMediaKit `MediaServer` | HTTP 80、RTSP 554 | 拉流、转协议、在线播放与分析结果推流 |
| 外部算法 API | 用户自行提供，可选 | 8070 | 使用 `api` 算法时接收图片并返回识别结果 |

> [!IMPORTANT]
> 仓库包含 Django 后台源码、C++ Analyzer 源码和推理模型，但不包含 ZLMediaKit 的 `MediaServer` 二进制，也不包含 Analyzer 编译所需的所有 C/C++ 第三方开发库。首次使用建议先启动 Web 后台，再按需搭建完整分析链路。

## 2. 仓库目录

```text
VideoAnalyser/
├── Admin/                  # Django 管理后台
│   ├── app/                # 业务代码
│   ├── framework/          # Django 配置
│   ├── static/             # 前端静态资源
│   └── templates/          # 页面模板
├── Analyzer/               # C++ 视频分析器
│   ├── Analyzer/           # C++ 源码
│   └── models/             # ONNX/OpenVINO 模型
├── data/                   # 运维脚本
├── config.example.json     # 配置模板
└── README.md               # 本文档
```

运行产生的视频、截图、数据库、日志、缓存和编译产物均被 `.gitignore` 排除，不会提交到 GitHub。

## 3. 个人用户快速启动 Web 后台

以下步骤可以先启动管理页面。没有启动 ZLMediaKit 和 Analyzer 时，页面可以访问，但视频转发、在线播放和 AI 分析功能不可用。

### 3.1 环境要求

- Python 3.8 或更高版本
- Git
- Windows 10/11 或常见 Linux 发行版

### 3.2 克隆项目

```bash
git clone git@github.com:scaredof/VideoAnalyser.git
cd VideoAnalyser
```

也可以使用 HTTPS：

```bash
git clone https://github.com/scaredof/VideoAnalyser.git
cd VideoAnalyser
```

### 3.3 创建配置文件

复制配置模板：

Windows PowerShell：

```powershell
Copy-Item config.example.json config.json
```

Linux：

```bash
cp config.example.json config.json
```

编辑 `config.json`，至少设置以下字段：

```json
{
  "host": "127.0.0.1",
  "adminPort": 9001,
  "mediaHttpPort": 80,
  "analyzerPort": 9003,
  "mediaRtspPort": 554,
  "mediaSecret": "替换为ZLMediaKit的API密钥",
  "uploadDir": "项目绝对路径/Admin/static/upload",
  "modelDir": "项目绝对路径/Analyzer/models",
  "algorithmApiUrl": "http://127.0.0.1:8070/receiveImageLLMVL"
}
```

路径必须是本机真实的绝对路径。Windows JSON 路径中的反斜杠需要写成双反斜杠，例如：

```json
{
  "uploadDir": "D:\\VideoAnalyser\\Admin\\static\\upload",
  "modelDir": "D:\\VideoAnalyser\\Analyzer\\models"
}
```

`config.json` 包含本机路径和服务密钥，已被 Git 忽略，请勿强制提交。

### 3.4 创建 Python 虚拟环境并安装依赖

Windows PowerShell：

```powershell
cd Admin
python -m venv venv
.\venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r requirements-windows.txt
```

如果 PowerShell 禁止执行激活脚本，可以在当前窗口运行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\venv\Scripts\Activate.ps1
```

Linux：

```bash
cd Admin
python3 -m venv venv
source venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements-linux.txt
```

### 3.5 初始化数据库

仓库不提交运行中的 SQLite 数据库。首次运行时执行：

```bash
python manage.py migrate --run-syncdb
python manage.py createsuperuser
```

按提示创建自己的管理员账号和密码。旧版文档中的示例账号不是新部署自动生成的账号。

当前源码会直接访问 `av_algorithm` 表，但该表没有对应的 Django 模型迁移。首次部署还需执行：

```bash
python manage.py shell
```

在 Django Shell 中粘贴：

```python
from django.db import connection

with connection.cursor() as cursor:
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS av_algorithm (
            id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
            sort INTEGER NOT NULL DEFAULT 0,
            code VARCHAR(50) NOT NULL,
            name VARCHAR(50) NOT NULL,
            object_count INTEGER NOT NULL DEFAULT 0,
            objects TEXT NOT NULL,
            remark TEXT NOT NULL DEFAULT '',
            state INTEGER NOT NULL DEFAULT 0
        )
    """)
```

输入 `exit()` 退出 Shell。

### 3.6 启动后台

建议为当前终端设置一个独立的 Django 密钥：

Windows PowerShell：

```powershell
$env:DJANGO_SECRET_KEY = "替换为随机且足够长的字符串"
python manage.py runserver 0.0.0.0:9001
```

Linux：

```bash
export DJANGO_SECRET_KEY='替换为随机且足够长的字符串'
python manage.py runserver 0.0.0.0:9001
```

浏览器访问：

```text
http://127.0.0.1:9001/login
```

使用刚才通过 `createsuperuser` 创建的账号登录。

## 4. 搭建完整视频分析链路

完整使用顺序如下：

```text
摄像头/RTSP 视频源
        ↓
ZLMediaKit MediaServer（拉流、转协议）
        ↓                    ↓
Django 管理后台         C++ Analyzer（AI 推理）
        ↑                    ↓
        └──── 布控与告警 API ────┘
```

### 4.1 准备 ZLMediaKit

自行安装或编译 ZLMediaKit，并确认：

1. HTTP API 和 HTTP 播放端口与 `mediaHttpPort` 一致。
2. RTSP 端口与 `mediaRtspPort` 一致。
3. ZLMediaKit 配置中的 API `secret` 与 `config.json` 的 `mediaSecret` 一致。
4. 防火墙已放行所用端口。

仓库中的后台会调用以下 ZLMediaKit API：

- `/index/api/addStreamProxy`
- `/index/api/delStreamProxy`
- `/index/api/getMediaList`
- `/index/api/getMediaInfo`

可以先在浏览器或命令行访问 ZLMediaKit HTTP 端口，确认服务已启动，再使用后台的“视频流”功能。

### 4.2 编译 Analyzer（Linux）

Analyzer 使用 C++17。`Analyzer/CMakeLists.txt` 当前期望以下依赖安装在 `/usr/local` 对应目录：

- CMake 3.16+
- OpenCV
- FFmpeg
- ONNX Runtime
- OpenVINO 与 TBB
- libevent
- libcurl
- jsoncpp
- pthread

依赖准备完成后：

```bash
cd Analyzer
mkdir -p build
cd build
cmake ..
cmake --build . -j
```

生成的程序通常位于 `Analyzer/build/Analyzer`。

> [!NOTE]
> `CMakeLists.txt` 中的第三方库路径是固定的 `/usr/local/...`。如果你的库安装在其他位置，需要调整 CMake 配置或通过适当的 CMake 参数提供路径。

### 4.3 编译 Analyzer（Windows）

可使用 `Analyzer/Analyzer.sln` 打开 Visual Studio 工程，但工程引用了仓库外的 `Analyzer/3rdparty` 依赖目录。需要先自行准备 FFmpeg、ONNX Runtime、OpenCV、OpenVINO、TBB、curl、libevent 和 jsoncpp，并根据本机目录修改项目的包含目录、库目录与链接配置。

如果只是体验后台页面，暂时不需要编译 Analyzer。

### 4.4 启动 Analyzer

从项目根目录启动，以便统一使用根目录的 `config.json`。

Linux：

```bash
export LD_LIBRARY_PATH=/usr/local/openvino/3rdparty/tbb/lib:${LD_LIBRARY_PATH:-}
./Analyzer/build/Analyzer -f ./config.json
```

Windows（假设已生成可执行文件）：

```powershell
.\Analyzer\Analyzer\x64\Release\Analyzer.exe -f .\config.json
```

健康检查：

```text
http://127.0.0.1:9003/api/health
```

Linux 环境也可以参考 `data/auto_restart.sh` 守护 Analyzer。使用前必须把脚本中的 `BASE_DIR` 改为实际项目路径。

### 4.5 推荐启动顺序

1. 启动 ZLMediaKit `MediaServer`。
2. 启动 Django：在 `Admin` 目录执行 `python manage.py runserver 0.0.0.0:9001`。
3. 启动 Analyzer：在项目根目录执行 Analyzer，并传入 `config.json`。
4. 打开 `http://127.0.0.1:9001/login`。

## 5. 日常使用流程

### 5.1 添加视频流

1. 登录后台。
2. 进入“视频流”页面并添加视频源。
3. 填写唯一编号、应用名、流名称和拉流地址，例如摄像头的 RTSP URL。
4. 执行转发操作。
5. 在“在线流”或播放页面确认画面正常。

如果转发失败，优先检查 ZLMediaKit 是否在线、`mediaSecret` 是否一致，以及视频源能否从服务器访问。

### 5.2 添加算法

在“算法管理”中创建算法时，算法编号必须与 Analyzer 源码支持的编号一致：

| 算法编号 | 模型/用途 | 可用目标示例 |
| --- | --- | --- |
| `on_yolov8n_80` | YOLOv8n ONNX Runtime 通用检测 | `person,car,...` |
| `ov_yolov8n_80` | YOLOv8n OpenVINO 通用检测 | `person,car,...` |
| `ov_yolov8n_fight_nofight` | 打架识别 | `fight,nofight` |
| `ov_yolov8n_fire_smoke` | 火焰/烟雾识别 | `fire,smoke` |
| `ov_yolov8n_smoke` | 吸烟/烟雾模型 | `smoke` |
| `ov_yolov11n_safehat` | 安全帽识别 | `head,safehat` |
| `api` | 调用外部算法 API | 由外部 API 定义 |

目标名称区分大小写，应使用英文逗号分隔，并与模型标签保持一致。

### 5.3 创建布控

1. 确认视频流已经在线。
2. 进入“布控”并选择视频流。
3. 选择算法和目标类别。
4. 在画面上绘制识别区域。
5. 设置检测间隔、分类阈值和重叠阈值。
6. 按需选择是否推送分析后的视频流。
7. 保存并启动布控。

Analyzer 收到布控任务后会拉取视频、执行推理，并把告警信息回调到后台。告警图片和视频写入 `Admin/static/upload/alarm`，该目录不会被 Git 跟踪。

## 6. 配置字段说明

| 字段 | 示例 | 说明 |
| --- | --- | --- |
| `version` | `3.49` | 项目版本标识 |
| `host` | `127.0.0.1` | 各服务所在主机；跨机器部署时填写服务器可访问地址 |
| `videoAnalyzerPort` | `9000` | 打包版总控程序预留端口 |
| `adminPort` | `9001` | Django 后台端口 |
| `mediaHttpPort` | `80` | ZLMediaKit HTTP/API/播放端口 |
| `analyzerPort` | `9003` | Analyzer HTTP API 端口 |
| `mediaRtspPort` | `554` | ZLMediaKit RTSP 端口 |
| `mediaSecret` | `...` | ZLMediaKit API 密钥，请勿公开 |
| `uploadDir` | 绝对路径 | 上传文件和告警文件根目录 |
| `modelDir` | 绝对路径 | Analyzer 模型目录 |
| `recordLogInterval` | `30` | 打包版启动工具记录进程状态的间隔，单位秒 |
| `workerConcurrency` | `20` | 工作并发配置；当前源码中主要供整体配置保留 |
| `algorithmApiUrl` | `http://127.0.0.1:8070/receiveImageLLMVL` | `api` 算法使用的外部服务地址 |

## 7. 常用检查

### 后台是否在线

```text
http://127.0.0.1:9001/login
```

### Analyzer 是否在线

```text
http://127.0.0.1:9003/api/health
```

### 查看端口占用（Windows）

```powershell
Get-NetTCPConnection -State Listen | Where-Object LocalPort -in 80,554,9001,9003
```

### 查看端口占用（Linux）

```bash
ss -lntp | grep -E ':(80|554|9001|9003)\b'
```

## 8. 常见问题

### 启动后台时报 `config.json` 不存在

后台会从项目根目录读取 `config.json`。请从 `config.example.json` 复制生成，并确认文件位于 `Admin` 的上一级目录。

### 后台能打开，但视频流功能不可用

这是 ZLMediaKit 未启动、端口不一致或 `mediaSecret` 不匹配时的典型现象。先验证 MediaServer 的 HTTP API，再检查 `config.json`。

### 后台能打开，但布控启动失败

检查 Analyzer 是否运行、`analyzerPort` 是否一致，以及 `http://127.0.0.1:9003/api/health` 是否可访问。还要确认所选算法编号与 Analyzer 支持的编号完全一致。

### Analyzer 提示模型加载失败

检查 `modelDir` 是否为绝对路径、目录是否存在，以及 `.xml` 与 `.bin` 模型文件是否完整。Linux 还需确认 ONNX Runtime/OpenVINO 的动态库搜索路径。

### 数据库提示 `no such table`

先执行：

```bash
python manage.py migrate --run-syncdb
```

如果缺少 `av_algorithm`，再按照“3.5 初始化数据库”中的 SQL 创建该表。

### Windows 路径导致 JSON 解析失败

JSON 中的 `\` 是转义符，Windows 路径必须写为 `D:\\VideoAnalyser\\...`，也可以使用正斜杠 `D:/VideoAnalyser/...`。

## 9. 安全与生产部署提示

- 不要提交 `config.json`、数据库、视频、告警文件或真实密钥。
- 生产环境必须设置随机的 `DJANGO_SECRET_KEY`。
- 当前 Django 配置中 `DEBUG = True` 且 `ALLOWED_HOSTS = ["*"]`，只适合开发或可信内网；公网部署前应修改。
- Django 的 `runserver` 仅用于开发。生产环境应使用 Gunicorn/uWSGI 等 WSGI 服务，并通过 Nginx 或其他反向代理对外提供服务。
- 首次登录后应使用强密码，不要沿用示例或默认密码。
- 摄像头 RTSP 地址通常包含凭据，不要写进仓库或公开日志。

## 10. 当前已知限制

- 仓库未内置 ZLMediaKit，需单独安装。
- Analyzer 第三方 C/C++ 库未随仓库分发，编译前需自行准备。
- Windows Visual Studio 工程引用仓库外的 `3rdparty` 目录，需要调整本地依赖路径。
- `av_algorithm` 使用直接 SQL 管理，当前没有 Django migration，首次部署需手工建表。
- `api` 算法依赖外部 HTTP 算法服务；不使用该算法时可忽略 `algorithmApiUrl` 对应服务。

更细的后台与 Analyzer 原始说明分别位于 `Admin/README.md` 和 `Analyzer/README.md`。
