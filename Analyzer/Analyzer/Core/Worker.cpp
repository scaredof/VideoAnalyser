#include "Worker.h"
#include "Utils/Log.h"
#include "Utils/Common.h"
#include "Scheduler.h"
#include "Analyzer.h"
#include "Control.h"
#include "Algorithm.h"
#include "AvPullStream.h"
#include "AvPushStream.h"
#include "GenerateAlarmVideo.h"
#include "Frame.h"

extern "C" {
#include "libswscale/swscale.h"
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

namespace
{
  float calculateFontScale(int imageWidth, int imageHeight) {
    // 基于1920x1080作为基准分辨率
    float baseWidth = 1920.0f;
    float baseHeight = 1080.0f;
    float baseFontScale = 0.8f;  // 基准字体大小
    // 计算缩放比例（取宽高中较小的比例，避免文字过大）
    float scaleX = imageWidth / baseWidth;
    float scaleY = imageHeight / baseHeight;
    float scale = std::min(scaleX, scaleY);
    // 限制缩放范围，避免过大或过小
    scale = std::max(0.3f, std::min(scale, 2.0f));
    return baseFontScale * scale;  
  }

  void drawTextWithBackground(cv::Mat& image, const std::string& text, cv::Point position,
                             float fontScale, cv::Scalar textColor, cv::Scalar bgColor) {
      // 计算文字大小
      int baseline = 0;
      cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontScale, 1,
  &baseline);
      // 绘制半透明背景
      cv::Rect bgRect(position.x - 5, position.y - textSize.height - 5,
                     textSize.width + 10, textSize.height + baseline + 10);
      // 创建半透明效果
      cv::Mat roi = image(bgRect);
      cv::Mat colorRect(roi.size(), roi.type(), bgColor);
      cv::addWeighted(roi, 0.7, colorRect, 0.3, 0, roi);  // 70%原图 + 30%背景色
      // 绘制文字
      cv::putText(image, text, position, cv::FONT_HERSHEY_SIMPLEX, fontScale, textColor, 1, cv::LINE_AA);
  }

 struct FontConfig {
      float scale;
      int thickness;
      int fontType;
      bool useOutline;
  };
  FontConfig getFontConfig(int imageWidth, int imageHeight) {
      FontConfig config;
      int totalPixels = imageWidth * imageHeight;
      if (totalPixels < 854 * 480) {           // 很低分辨率
          config.scale = 0.6f;
          config.thickness = 1;
          config.fontType = cv::FONT_HERSHEY_SIMPLEX;  // 最清晰的字体
          config.useOutline = true;  // 使用描边增强可读性
      } else if (totalPixels < 1280 * 720) {   // 低分辨率
          config.scale = 0.7f;
          config.thickness = 1;
          config.fontType = cv::FONT_HERSHEY_SIMPLEX;
          config.useOutline = true;
      } else {                                 // 正常分辨率
          float scale = imageWidth / 1920.0f;
          config.scale = std::max(0.6f, 0.8f * scale);
          config.thickness = (imageWidth >= 1920) ? 2 : 1;
          config.fontType = cv::FONT_HERSHEY_SIMPLEX;
          config.useOutline = false;
      }
      return config;
  }   

  void drawTextWithOutline(cv::Mat& image, const std::string& text, cv::Point position,
                          float fontScale, cv::Scalar textColor, cv::Scalar outlineColor, int thickness = 1) {
      // 先绘制描边（黑色，粗一点）
      cv::putText(image, text, position, cv::FONT_HERSHEY_SIMPLEX,
                 fontScale, outlineColor, thickness + 2, cv::LINE_AA);
      // 再绘制文字（白色或彩色，细一点）
      cv::putText(image, text, position, cv::FONT_HERSHEY_SIMPLEX,
                 fontScale, textColor, thickness, cv::LINE_AA);
  }  

  void drawCornerMarkers(cv::Mat& image, cv::Rect rect, cv::Scalar color, int thickness, int length) {
      cv::Point tl = rect.tl();
      cv::Point br = rect.br();
      // 左上角
      cv::line(image, tl, cv::Point(tl.x + length, tl.y), color, thickness);
      cv::line(image, tl, cv::Point(tl.x, tl.y + length), color, thickness);
      // 右上角
      cv::line(image, cv::Point(br.x, tl.y), cv::Point(br.x - length, tl.y), color, thickness);
      cv::line(image, cv::Point(br.x, tl.y), cv::Point(br.x, tl.y + length), color, thickness);
      // 右下角
      cv::line(image, br, cv::Point(br.x - length, br.y), color, thickness);
      cv::line(image, br, cv::Point(br.x, br.y - length), color, thickness);
      // 左下角
      cv::line(image, cv::Point(tl.x, br.y), cv::Point(tl.x + length, br.y), color, thickness);
      cv::line(image, cv::Point(tl.x, br.y), cv::Point(tl.x, br.y - length), color, thickness);
  } 

  cv::Scalar getConfidenceColor(float confidence) {
      // 根据置信度返回不同颜色
      if (confidence >= 0.8f) {
          return cv::Scalar(0, 255, 0);      // 绿色 - 高置信度
      } else if (confidence >= 0.6f) {
          return cv::Scalar(0, 255, 255);    // 黄色 - 中等置信度
      } else {
          return cv::Scalar(0, 165, 255);    // 橙色 - 低置信度
      }
  }  
}

namespace AVSAnalyzer {

    Worker::Worker(Scheduler* scheduler, Control* control) :
        mScheduler(scheduler),
        mControl(new Control(*control)),
        mPullStream(nullptr),
        mPushStream(nullptr),
        mAnalyzer(nullptr),
        mState(false)
    {

        mControl->startTimestamp = getCurTimestamp();

        LOGI("");
    }

    Worker::~Worker()
    {
        LOGI("");

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        mState = false;// 将执行状态设置为false

        for (auto th : mThreads) {
            th->join();
        }

        for (auto th : mThreads) {
            delete th;
            th = nullptr;
        }
        mThreads.clear();
        clearAlarmVideoFrameQ();

        if (mPullStream) {
            delete mPullStream;
            mPullStream = nullptr;
        }
        if (mPushStream) {
            delete mPushStream;
            mPushStream = nullptr;
        }

        if (mAnalyzer) {
            delete mAnalyzer;
            mAnalyzer = nullptr;
        }

        if (mControl) {
            delete mControl;
            mControl = nullptr;
        }
        //最有一步释放mFramePool
        if (mVideoFramePool) {
            delete mVideoFramePool;
            mVideoFramePool = nullptr;
        }

    }
    bool Worker::start(std::string& msg) {

        this->mPullStream = new AvPullStream(this);
        if (this->mPullStream->connect()) {
            if (mControl->pushStream) {
                this->mPushStream = new AvPushStream(this);
                if (this->mPushStream->connect()) {
                    // success
                }
                else {
                    msg = "pull stream connect success, push stream connect error";
                    return false;
                }
            }
            else {
                // success
            }
        }
        else {
            msg = "pull stream connect error";
            return false;
        }

        int videoBgrSize = mControl->videoHeight * mControl->videoWidth * mControl->videoChannel;
        this->mVideoFramePool = new FramePool(videoBgrSize);
        this->mAnalyzer = new Analyzer(mScheduler, mControl);

        mState = true;// 将执行状态设置为true


        std::thread* th = new std::thread(AvPullStream::readThread, this->mPullStream);
        mThreads.push_back(th);

        th = new std::thread(Worker::decodeVideoThread, this);
        mThreads.push_back(th);

        th = new std::thread(Worker::generateAlarmThread, this);
        mThreads.push_back(th);


        if (mControl->pushStream) {
            if (mControl->videoIndex > -1) {
                th = new std::thread(AvPushStream::encodeVideoThread, this->mPushStream);
                mThreads.push_back(th);
            }
        }

        for (auto th : mThreads) {
            th->native_handle();
        }


        return true;
    }


    bool Worker::getState() {
        return mState;
    }
    void Worker::remove() {
        mState = false;
        mScheduler->removeWorker(mControl);
    }
    void Worker::generateAlarmThread(void* arg) {
        Worker* worker = (Worker*)arg;
        worker->handleGenerateAlarm();
    }
    void Worker::handleGenerateAlarm() {

        int width = mControl->videoWidth;
        int height = mControl->videoHeight;
        int channels = mControl->videoChannel;

        Frame* videoFrame = nullptr; // 未编码的视频帧（bgr格式）

        std::queue<Frame* > cacheV;
        int prefix_size = 30; //75 = 25 * 3，事件发生前缓存3秒的数据，1张压缩图片100kb

        bool happening = false;// 当前是否正在发生报警行为
        std::queue<Frame* > happenV;
        int  total_size = 60;//报警总帧数最大长度

        int64_t last_alarm_timestamp = 0;// 上一次报警的时间戳

        int64_t t1, t2 = 0;

        while (getState())
        {
            if (getAlarmVideoFrame(videoFrame)) {

                if (happening && cacheV.empty()) {// 报警事件已经发生，正在进行中
                    
                    happenV.push(videoFrame);

                    if (happenV.size() >= total_size) {
                        last_alarm_timestamp = getCurTimestamp();

                        //产生报警视频
                        Alarm* alarm = new Alarm(
                            height,
                            width,
                            mControl->videoFps,
                            last_alarm_timestamp,prefix_size,
                            mControl->code.data()
                        );
     
                        while (!happenV.empty())
                        {
                            Frame * p = happenV.front();
                            happenV.pop();
                            alarm->frames.push_back(p);
                        }
                    
                        mScheduler->addAlarm(alarm);
                        happening = false;
                    }

                }
                else {// 暂未发生报警事件
          
                 
                    if (!cacheV.empty() && cacheV.size() >= prefix_size) {
                        //缓存帧超过容量上限 prefix_size
                        Frame* head = cacheV.front();
                        cacheV.pop();
                        mVideoFramePool->giveBack(head);
                    }
                    cacheV.push(videoFrame);


                    if (videoFrame->happen &&
                        cacheV.size() >= prefix_size &&
                        (getCurTimestamp() - last_alarm_timestamp) > mControl->minInterval) {
                        //满足报警开始条件
                        happening = true;
                        
                        while (!cacheV.empty())
                        {
                            Frame * p = cacheV.front();
                            cacheV.pop();
                            happenV.push(p);
                        }        
                    }
                }

            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }

        }

        while (!happenV.empty())
        {
            Frame * p = happenV.front();
            happenV.pop();
            delete p;
            p = nullptr;
            
        }
        while (!cacheV.empty())
        {
            Frame* p = cacheV.front();
            cacheV.pop();
            delete p;
            p = nullptr;

        }


    }
    void Worker::decodeVideoThread(void* arg) {
        Worker* worker = (Worker*)arg;
        worker->handleDecodeVideo();
    }
    void Worker::handleDecodeVideo() {
   
        int width = mPullStream->mVideoCodecCtx->width;
        int height = mPullStream->mVideoCodecCtx->height;

        AVPacket pkt; // 未解码的视频帧
        int      pktQSize = 0; // 未解码视频帧队列当前长度

        AVFrame* frame_yuv420p = av_frame_alloc();// pkt->解码->frame
        AVFrame* frame_bgr = av_frame_alloc();

        int frame_bgr_buff_size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, width, height, 1);
        uint8_t* frame_bgr_buff = (uint8_t*)av_malloc(frame_bgr_buff_size);
        av_image_fill_arrays(frame_bgr->data, frame_bgr->linesize, frame_bgr_buff, AV_PIX_FMT_BGR24, width, height, 1);

        SwsContext* sws_ctx_yuv420p2bgr = sws_getContext(width, height,
            mPullStream->mVideoCodecCtx->pix_fmt,
            mPullStream->mVideoCodecCtx->width,
            mPullStream->mVideoCodecCtx->height,
            AV_PIX_FMT_BGR24,
            SWS_BICUBIC, nullptr, nullptr, nullptr);

        int fps = mControl->videoFps;

        //算法检测参数start
        bool cur_is_check = false;// 当前帧是否进行算法检测
        int  continuity_check_count = 0;// 当前连续进行算法检测的帧数
        int  continuity_check_max_time = 6000;//连续进行算法检测，允许最长的时间。单位毫秒
        int64_t continuity_check_start = getCurTime();//单位毫秒
        int64_t continuity_check_end = 0;
        //算法检测参数end

        int ret = -1;
        int64_t frameCount = 0;
        bool happen = false;
        float happenScore = 0.0;
        std::vector<DetectObject> happenDetects;

        // currentFrameCount 用于存放每一帧的实时检测结果
        std::map<std::string, int> currentFrameCount;
        // displayCount 用于稳定地显示快照结果
        std::map<std::string, int> displayCount; 
        int t = 0;
        float fontScale = calculateFontScale(mControl->videoWidth, mControl->videoHeight);
        int fontThickness = (mControl->videoWidth >= 1920) ? 2 : 1; 
        FontConfig fontConfig = getFontConfig(mControl->videoWidth, mControl->videoHeight);

        while (getState())
        {
          t ++;
            if (mPullStream->getVideoPkt(pkt, pktQSize)) {

                if (mControl->videoIndex > -1) {

                    ret = avcodec_send_packet(mPullStream->mVideoCodecCtx, &pkt);
                    if (ret == 0) {
                        ret = avcodec_receive_frame(mPullStream->mVideoCodecCtx, frame_yuv420p);

                        if (ret == 0) {
                            frameCount++;

                            // frame（yuv420p） 转 frame_bgr
                            sws_scale(sws_ctx_yuv420p2bgr,
                                frame_yuv420p->data, frame_yuv420p->linesize, 0, height,
                                frame_bgr->data, frame_bgr->linesize);


                            cv::Mat image(mControl->videoHeight, mControl->videoWidth, CV_8UC3, frame_bgr->data[0]);

                            happen = false;
                            happenScore = 0.0;


                            //只处理偶数帧,不然太卡
                            if (pktQSize <= 5 && frameCount % 2 == 0) {

                                currentFrameCount.clear();

//                                int64_t t1 = getCurTimestamp();
                                cur_is_check = mAnalyzer->handleVideoFrame(t, frameCount, image, happenDetects, happen, happenScore, currentFrameCount);

//                                int64_t t2 = getCurTimestamp();
//                                LOGI("t2 - t1 = %ld",(t2 - t1));

                                if (cur_is_check) {
                                    continuity_check_count += 1;
                                }
                            }
                            else {
                                cur_is_check = false;
                            }

                            continuity_check_end = getCurTime();
                            if (continuity_check_end - continuity_check_start > continuity_check_max_time) {
                                mControl->checkFps = float(continuity_check_count) / (float(continuity_check_end - continuity_check_start) / 1000);
                                continuity_check_count = 0;
                                continuity_check_start = getCurTime();
                            }

                            //绘制start
                            cv::polylines(image, mControl->recognitionRegion_points, mControl->recognitionRegion_points.size(), cv::Scalar(0, 0, 255), 4, 8);//绘制多边形
                            if (happenDetects.size() > 0) {
                                
                                for (size_t i = 0; i < happenDetects.size(); i++)
                                {
                                    if (happenDetects[i].happen) {
                                        int x1, y1, x2, y2;
                                        x1 = happenDetects[i].x1;
                                        y1 = happenDetects[i].y1;
                                        x2 = happenDetects[i].x2;
                                        y2 = happenDetects[i].y2;

                                        std::string class_name = happenDetects[i].class_name;
                                        float       class_score = happenDetects[i].class_score;

                                        std::stringstream class_score_ss;
                                        class_score_ss << std::setprecision(1) << class_score;
                                        std::string title = class_name + ":" + class_score_ss.str();

                                        //cv::rectangle(image, cv::Rect(x1, y1, (x2 - x1), (y2 - y1)), cv::Scalar(0, 0, 255), fontThickness, cv::LINE_8, 0);
                                        // 使用边角标记式（更简洁）
                                        cv::Scalar boxColor = getConfidenceColor(class_score);
                                        cv::Rect rect(x1, y1, x2 - x1, y2 - y1);
                                        drawCornerMarkers(image, rect, boxColor, 3, 25); 
                                        //cv::rectangle(image, cv::Point(x1, y1 + 20), cv::Point(x2, y1), cv::Scalar(0, 255, 255), -1);

                                        int h = y2 - y1;

                                        //cv::putText(image, title, cv::Point(x1, y1 + (h / 3)), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 2);
                                        //cv::putText(image, title, cv::Point(x1, y1 - 10), cv::FONT_HERSHEY_SIMPLEX, fontScale * 0.8f, cv::Scalar(0, 0, 255), fontThickness, cv::LINE_AA);
                                        // 检测框标签（使用描边）
                                        if (fontConfig.useOutline) {
                                            drawTextWithOutline(image, title, cv::Point(x1, y1 - 10),
                                                               fontConfig.scale * 0.8f, cv::Scalar(255, 255, 255), cv::Scalar(0, 0, 0));
                                        } else {
                                            cv::putText(image, title, cv::Point(x1, y1 - 10),
                                                       cv::FONT_HERSHEY_SIMPLEX, fontConfig.scale * 0.8f,
                                                       cv::Scalar(255, 255, 255), fontConfig.thickness, cv::LINE_AA);
                                        }   
                                    }

                                }
                            }
                            //cv::putText(image, mControl->algorithmCode, cv::Point(20, 80), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 1);
                            std::stringstream fps_stream;
                            fps_stream << std::setprecision(4) << mControl->checkFps;
                            std::string fps_title = "checkfps:" + fps_stream.str();
                            //cv::putText(image, fps_title, cv::Point(20, 140), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
                            //cv::putText(image, fps_title, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, fontScale * 0.8f, cv::Scalar(0, 0, 255), fontThickness, cv::LINE_AA);
                            drawTextWithBackground(image, fps_title, cv::Point(20, 40),
                                                            fontConfig.scale, cv::Scalar(255, 255, 255), cv::Scalar(0, 0, 0));
                            
                            if(t % 25 == 0)
                            {
                              displayCount = currentFrameCount;
                            }

                            // 新增：绘制检测统计数量
                            //int draw_y = 200;
                            int draw_y = 80;
                            for (const auto& pair : displayCount) {
                                std::string text = pair.first + ": " + std::to_string(pair.second);
                                //cv::putText(image, text, cv::Point(20, draw_y), cv::FONT_HERSHEY_SIMPLEX, fontScale * 0.8f, cv::Scalar(0, 255, 0), fontThickness, cv::LINE_AA);
                                //draw_y += 30;
                                drawTextWithBackground(image, text, cv::Point(20, draw_y),
                                                                  fontConfig.scale * 0.9f, cv::Scalar(0, 255, 255), cv::Scalar(0, 0, 0));
                                draw_y += (int)(35 * fontConfig.scale); 
                            }
                            //绘制end
     

                            if (mControl->pushStream) {//需要推算法实时流
                                int size = mPushStream->getVideoFrameQSize();
                                if (size > 10) {
                                    mPushStream->clearVideoFrameQueue();
                                }
                                Frame* frame = mVideoFramePool->gain();
                                frame->setBuf(frame_bgr->data[0], frame_bgr_buff_size);
                                frame->happen = happen;
                                frame->happenScore = happenScore;
                                mPushStream->addVideoFrame(frame);
        
                            }
                            
                            //添加合成报警视频帧start
                            int size = getAlarmVideoFrameQSize();
                            if (size > 10) {
                                this->clearAlarmVideoFrameQ();
                            }
                         
                            Frame* frame = mVideoFramePool->gain();
                            frame->setBuf(frame_bgr->data[0], frame_bgr_buff_size);
                            frame->happen = happen;
                            frame->happenScore = happenScore;
                            this->addAlarmVideoFrameQ(frame);
                 
                            //添加合成报警视频帧end
                            
                        
                        }
                        else {
                            LOGE("avcodec_receive_frame error : ret=%d", ret);
                        }
                    }
                    else {
                        LOGE("avcodec_send_packet error : ret=%d", ret);
                    }
                }

                // 队列获取的pkt，必须释放!!!
                //av_free_packet(&pkt);//过时
                av_packet_unref(&pkt);
            }
            else {
//                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }


        av_frame_free(&frame_yuv420p);
        //av_frame_unref(frame_yuv420p);
        frame_yuv420p = NULL;

        av_frame_free(&frame_bgr);
        //av_frame_unref(frame_bgr);
        frame_bgr = NULL;


        av_free(frame_bgr_buff);
        frame_bgr_buff = NULL;


        sws_freeContext(sws_ctx_yuv420p2bgr);
        sws_ctx_yuv420p2bgr = NULL;

    }


    void Worker::addAlarmVideoFrameQ(Frame* frame) {

        mAlarmVideoFrameQ_mtx.lock();
        mAlarmVideoFrameQ.push(frame);
        mAlarmVideoFrameQ_mtx.unlock();

    }
    int Worker::getAlarmVideoFrameQSize() {
        int size = 0;
        mAlarmVideoFrameQ_mtx.lock();
        size = mAlarmVideoFrameQ.size();
        mAlarmVideoFrameQ_mtx.unlock();
        return size;
    }
    bool Worker::getAlarmVideoFrame(Frame*& frame) {
        bool res = false;
        if (mAlarmVideoFrameQ_mtx.try_lock()) {
            if (!mAlarmVideoFrameQ.empty()) {
                frame = mAlarmVideoFrameQ.front();
                mAlarmVideoFrameQ.pop();
                res = true;
            }
            mAlarmVideoFrameQ_mtx.unlock();
        }
        return res;
    }
    void Worker::clearAlarmVideoFrameQ() {

        mAlarmVideoFrameQ_mtx.lock();
        while (!mAlarmVideoFrameQ.empty())
        {
            Frame* frame = mAlarmVideoFrameQ.front();
            mAlarmVideoFrameQ.pop();
            mVideoFramePool->giveBack(frame);
        }
        mAlarmVideoFrameQ_mtx.unlock();

    }

}
