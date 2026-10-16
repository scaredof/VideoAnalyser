#include "AvPullStream.h"
#include "Config.h"
#include "Utils/Log.h"
#include "Utils/Common.h"
#include "Scheduler.h"
#include "Control.h"
#include "Worker.h"
#include <chrono>
#include <libavcodec/packet.h>
#include <mutex>
#include <thread>
namespace{
  static const int MAX_PKT_QUEUE_SIZE = 15;//最多缓存15帧，防止积压
}
namespace AVSAnalyzer {
    AvPullStream::AvPullStream(Worker* worker) :
        mWorker(worker)
    {
        LOGI("");
    }

    AvPullStream::~AvPullStream()
    {
        LOGI("");

        closeConnect();
    }

    bool AvPullStream::connect() {

        std::string streamUrl = mWorker->mControl->streamUrl;
        mFmtCtx = avformat_alloc_context();

        AVDictionary* fmt_options = NULL;
        av_dict_set(&fmt_options, "rtsp_transport", "tcp", 0); //设置rtsp底层网络协议 tcp or udp
        av_dict_set(&fmt_options, "stimeout", "30000000", 0);   //设置rtsp连接超时（单位 us）1秒=1000000
        av_dict_set(&fmt_options, "rw_timeout", "3000000", 0); //设置rtmp/http-flv连接超时（单位 us）
        //av_dict_set(&fmt_options, "timeout", "1000000", 0);//设置udp/http超时（单位 us）

        int ret = avformat_open_input(&mFmtCtx, streamUrl.data(), NULL, &fmt_options);

        if (ret != 0) {
            LOGE("avformat_open_input error: url=%s ", streamUrl.data());
            return false;
        }


        if (avformat_find_stream_info(mFmtCtx, NULL) < 0) {
            LOGE("avformat_find_stream_info error");
            return false;
        }

        // video start
        mWorker->mControl->videoIndex = av_find_best_stream(mFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);


        if (mWorker->mControl->videoIndex > -1) {
            AVCodecParameters* videoCodecPar = mFmtCtx->streams[mWorker->mControl->videoIndex]->codecpar;

            const AVCodec* videoCodec = NULL;
            if (!videoCodec) {
                videoCodec = avcodec_find_decoder(videoCodecPar->codec_id);
                if (!videoCodec) {
                    LOGE("avcodec_find_decoder error");
                    return false;
                }
            }

            mVideoCodecCtx = avcodec_alloc_context3(videoCodec);
            if (avcodec_parameters_to_context(mVideoCodecCtx, videoCodecPar) != 0) {
                LOGE("avcodec_parameters_to_context error");
                return false;
            }
            if (avcodec_open2(mVideoCodecCtx, videoCodec, nullptr) < 0) {
                LOGE("avcodec_open2 error");
                return false;
            }
            //mVideoCodecCtx->thread_count = 1;

            mVideoStream = mFmtCtx->streams[mWorker->mControl->videoIndex];
            if (0 == mVideoStream->avg_frame_rate.den) {

                LOGE("videoIndex=%d,videoStream->avg_frame_rate.den = 0", mWorker->mControl->videoIndex);

                mWorker->mControl->videoFps = 25;
            }
            else {
                mWorker->mControl->videoFps = mVideoStream->avg_frame_rate.num / mVideoStream->avg_frame_rate.den;
            }


            mWorker->mControl->videoWidth = mVideoCodecCtx->width;
            mWorker->mControl->videoHeight = mVideoCodecCtx->height;
            mWorker->mControl->videoChannel = 3;

            if (!mWorker->mControl->parseRecognitionRegion()) {
                LOGE("parseRecognitionRegion() error");
                return false;
            
            }

        }
        else {
            LOGE("av_find_best_stream video error videoIndex=%d", mWorker->mControl->videoIndex);
            return false;
        }
        // video end;


        // audio start

        // audio end


        if (mWorker->mControl->videoIndex <= -1) {
            return false;
        }

        mConnectCount++;

        return true;

    }

    bool AvPullStream::reConnect() {

        if (mConnectCount <= 100) {
            closeConnect();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (connect()) {
                return true;
            }
            else {
                return false;
            }

        }
        return false;
    }
    void AvPullStream::closeConnect() {

        LOGI("");

        clearVideoPktQueue();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (mVideoCodecCtx) {

            avcodec_close(mVideoCodecCtx);
            avcodec_free_context(&mVideoCodecCtx);
            mVideoCodecCtx = NULL;
            mWorker->mControl->videoIndex = -1;
        }

        if (mFmtCtx) {
            // 拉流不需要释放start
            //if (mFmtCtx && !(mFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            //    avio_close(mFmtCtx->pb);
            //}
            // 拉流不需要释放end
            avformat_close_input(&mFmtCtx);
            avformat_free_context(mFmtCtx);
            mFmtCtx = NULL;
        }
    }

    bool AvPullStream::pushVideoPkt(const AVPacket& pkt) {
        std::lock_guard<std::mutex> lock(mVideoPktQ_mtx);
        //队列太长，丢弃最旧的包
        while(mVideoPktQ.size() >= MAX_PKT_QUEUE_SIZE){
          AVPacket oldPacket = mVideoPktQ.front();
          av_packet_unref(&oldPacket);
          mVideoPktQ.pop();
        }
        
        if (av_packet_make_refcounted((AVPacket*)&pkt) < 0) {
            return false;
        }
        mVideoPktQ.push(pkt);
        return true;
    }
    bool AvPullStream::getVideoPkt(AVPacket& pkt, int& pktQSize) {

        std::lock_guard<std::mutex> lock(mVideoPktQ_mtx);
        if (!mVideoPktQ.empty()) {
            pkt = mVideoPktQ.front();
            mVideoPktQ.pop();
            pktQSize = mVideoPktQ.size();
            return true;
        }
        else {
            pktQSize = 0;
            return false;
        }

    }
    void AvPullStream::clearVideoPktQueue() {
        std::lock_guard<std::mutex> lock(mVideoPktQ_mtx);
        while (!mVideoPktQ.empty())
        {
            AVPacket pkt = mVideoPktQ.front();
            mVideoPktQ.pop();
            av_packet_unref(&pkt);
        }
    }
    void AvPullStream::handleRead() {
        int continuity_error_count = 0;

        AVPacket pkt;
        while (mWorker->getState())
        {
            if (av_read_frame(mFmtCtx, &pkt) >= 0) {
                continuity_error_count = 0;

                if (pkt.stream_index == mWorker->mControl->videoIndex) {
                    pushVideoPkt(pkt);
                }
                else {
                    //av_free_packet(&pkt);//过时
                    av_packet_unref(&pkt);
                }
            }
            else {
                //av_free_packet(&pkt);//过时
                av_packet_unref(&pkt);
                continuity_error_count++;
                if (continuity_error_count > 3) {//大于3秒重启拉流连接

                    LOGE("av_read_frame error, continuity_error_count = %d (s)", continuity_error_count);

                    if (reConnect()) {
                        continuity_error_count = 0;
                        LOGI("reConnect success : mConnectCount=%d", mConnectCount);
                    }
                    else {
                        LOGI("reConnect error : mConnectCount=%d", mConnectCount);
                        mWorker->remove();
                        break;
                    }
                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        }

    }
    void AvPullStream::readThread(void* arg) {

        AvPullStream* pullStream = (AvPullStream*)arg;
        pullStream->handleRead();
    }
}
