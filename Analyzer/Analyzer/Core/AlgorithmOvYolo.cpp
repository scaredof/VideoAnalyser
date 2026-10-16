#include "AlgorithmOvYolo.h"
#include "Config.h"
#include "Utils/Log.h"
#include "Utils/Common.h"

namespace AVSAnalyzer {

    AlgorithmOvYolo::AlgorithmOvYolo(Config* config, std::string& modelPath, std::vector<std::string>& classNames):
        Algorithm(config),mClassNames(classNames){
        LOGI("modelPath=%s", modelPath.data());
        core = new ov::Core;
        compiled_model = core->compile_model(modelPath, "CPU");
        infer_request = compiled_model.create_infer_request();
    }

    AlgorithmOvYolo::~AlgorithmOvYolo()
    {
        LOGI("");
        delete core;
        core = nullptr;

    }

    bool AlgorithmOvYolo::objectDetect(cv::Mat& image, std::vector<DetectObject>& detects, const std::string& targetObject){

        mMtx.lock();

        // Preprocess the image
        cv::Mat letterbox_img = letterbox(image);
        float scale = letterbox_img.size[0] / 640.0;
        cv::Mat blob = cv::dnn::blobFromImage(letterbox_img, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true);

        // -------- Step 5. Feed the blob into the input node of the Model -------
        // Get input port for model with one input
        auto input_port = compiled_model.input();


        // Create tensor from external memory
        ov::Tensor input_tensor(input_port.get_element_type(), input_port.get_shape(), blob.ptr(0));
        // Set input tensor for model with one input
        infer_request.set_input_tensor(input_tensor);

        // -------- Step 6. Start inference --------
        infer_request.infer();

        // -------- Step 7. Get the inference result --------
        auto output = infer_request.get_output_tensor(0);
        auto output_shape = output.get_shape();

        int rows = output_shape[2];        //8400
        int dimensions = output_shape[1];  //84: box[cx, cy, w, h]+80 classes scores

        // -------- Step 8. Postprocess the result --------
        float* data = output.data<float>();
        cv::Mat output_buffer(output_shape[1], output_shape[2], CV_32F, data);
        cv::transpose(output_buffer, output_buffer); //[8400,84]
        float score_threshold = 0.25;
        float nms_threshold = 0.5;
        std::vector<int> class_ids;
        std::vector<float> class_scores;
        std::vector<cv::Rect> boxes;

        // Figure out the bbox, class_id and class_score


        for (int i = 0; i < output_buffer.rows; i++) {
            cv::Mat classes_scores = output_buffer.row(i).colRange(4, 4+mClassNames.size());
            cv::Point class_id;
            double maxClassScore;
            cv::minMaxLoc(classes_scores, 0, &maxClassScore, 0, &class_id);

            if (maxClassScore > score_threshold) {
                class_scores.push_back(maxClassScore);
                class_ids.push_back(class_id.x);
                float cx = output_buffer.at<float>(i, 0);
                float cy = output_buffer.at<float>(i, 1);
                float w = output_buffer.at<float>(i, 2);
                float h = output_buffer.at<float>(i, 3);

                int left = int((cx - 0.5 * w) * scale);
                int top = int((cy - 0.5 * h) * scale);
                int width = int(w * scale);
                int height = int(h * scale);

                boxes.push_back(cv::Rect(left, top, width, height));
            }
        }
        //NMS
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, class_scores, score_threshold, nms_threshold, indices);

        // -------- Visualize the detection results -----------
        //if (indices.size() > 0) {
        //    detects.clear();
        //}

        for (size_t i = 0; i < indices.size(); i++) {
            int index = indices[i];
            int class_id = class_ids[index];

            std::string class_name = mClassNames[class_id];
            if(!targetObject.empty() && class_name != targetObject){
              continue;
            }

            DetectObject detect;
            detect.class_score = class_scores[index];
            detect.class_name = mClassNames[class_id];
            detect.x1 = boxes[index].tl().x;
            detect.y1 = boxes[index].tl().y;

            detect.x2 = detect.x1 + boxes[index].width;
            detect.y2 = detect.y1 + boxes[index].height;

            detects.push_back(detect);

        }

        //int64_t t2 = getCurTime();

        mMtx.unlock();

        return true;
    }

}
