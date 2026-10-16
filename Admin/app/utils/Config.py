import json
import os
from framework.settings import BASE_DIR
class Config:
    def __init__(self):

        BASE_DIR_PARENT_DIR = os.path.dirname(BASE_DIR) # 根目录的父级目录
        filepath = os.path.join(BASE_DIR_PARENT_DIR, "config.json")
        print("Config.__init__",os.path.abspath(__file__))
        print("Config.__init__,filepath=%s"%filepath)

        f = open(filepath, 'r', encoding='gbk')
        content = f.read()
        config_data = json.loads(content)
        f.close()

        print("Config.__init__",config_data)

        host = config_data.get("host")
        videoAnalyzerPort =config_data.get("videoAnalyzerPort")
        adminPort =config_data.get("adminPort")
        analyzerPort =config_data.get("analyzerPort")
        mediaHttpPort =config_data.get("mediaHttpPort")
        mediaRtspPort =config_data.get("mediaRtspPort")
        mediaSecret =config_data.get("mediaSecret")
        uploadDir =config_data.get("uploadDir")
        modelDir =config_data.get("modelDir")

        self.host = host
        self.videoAnalyzerHost = "http://"+host +":"+ str(videoAnalyzerPort)  # http://127.0.0.1:9000
        self.adminHost = "http://"+host +":"+ str(adminPort)         # http://127.0.0.1:9001
        self.analyzerHost = "http://"+host +":"+ str(analyzerPort)   # http://127.0.0.1:9002
        self.mediaHttpHost = "http://"+host +":"+ str(mediaHttpPort) # http://127.0.0.1:80
        self.mediaWsHost = "ws://"+host +":"+ str(mediaHttpPort)     # http://127.0.0.1:80
        self.mediaRtspHost = "rtsp://"+host +":"+ str(mediaRtspPort) # http://127.0.0.1:554
        self.mediaSecret = mediaSecret
        self.uploadDir = uploadDir
        self.modelDir = modelDir

        self.uploadStorageDir = os.path.join(self.uploadDir,"storage")
        self.uploadAlarmDir = os.path.join(self.uploadDir,"alarm")


        self.uploadDir_www = "/static/upload/"


    def __del__(self):
        pass

    def show(self):
        pass


