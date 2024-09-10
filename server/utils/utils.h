#ifndef UTILS_H
#define UTILS_H

#include <QImage>
#include <vector>

#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

#define ASSERET_WITH_LOG(msg, expr) \
  if (!expr) {                      \
    spdlog::error(msg);             \
    assert(expr);                   \
  }

namespace utils {

// 将cv::Mat转换为QImage
QImage MatToQImage(const cv::Mat& mat);

cv::Mat QImageToMat(const QImage& qimage);

void EncodeMat(cv::Mat& mat, std::string ext, std::vector<uchar>& bytes);

void DecodeMat(std::string bytes_str, cv::Mat& res);
}  // namespace utils

#endif  // UTILS_H
