#include <ElaApplication.h>

#include <QApplication>
#include <QWidget>
#include <memory>
#include <thread>

#include <absl/flags/flag.h>
#include <absl/flags/internal/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>
#include <seeta/CStruct.h>
#include <seeta/Struct.h>
#include <spdlog/spdlog.h>

#include "ela_gui.h"
#include "engine.h"
#include "face_camera.h"
#include "gui.h"
#include "rpc_manager.h"

/**
 ./build/server/server -native_camera_index=1
 -classifier_path=D:/cmake-install-modules/opencv-4.10.0/etc/haarcascades/haarcascade_frontalface_alt2.xml
 *
 */
ABSL_FLAG(std::string, classifier_path,
          "/home/yu/codefield/opencv/data/haarcascades_cuda/"
          "haarcascade_frontalface_alt2.xml",
          "[REQUIRED] Path of classifer model file.");
ABSL_FLAG(int, native_camera_index, -1, "[OPTIONAL] Index of camera device.");
ABSL_FLAG(std::string, network_camera_url, " ",
          "[OPTIONAL] URL of network camera.");
// ABSL_FLAG(int, native_camera_index, -1, "[Optional] set the id of camera
// device.");

constexpr char kServerAddrInfo[] = "localhost:50051";

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(
      "There are the availiable flags for this program:");
  absl::ParseCommandLine(argc, argv);

  seeta::ModelSetting FD_model_setting("fd_2_00.dat",
                                       seeta::ModelSetting::Device::CPU);
  seeta::ModelSetting PD_model_setting("pd_2_00_pts5.dat",
                                       seeta::ModelSetting::Device::CPU);
  seeta::ModelSetting FR_model_setting("fr_2_10.dat",
                                       seeta::ModelSetting::Device::CPU);
  arm_face_id::EngineConfig engine_config;
  engine_config.fd_setting = FD_model_setting;
  engine_config.fr_setting = FR_model_setting;
  engine_config.pd_setting = PD_model_setting;
  engine_config.classifier_path = absl::GetFlag(FLAGS_classifier_path);
  engine_config.native_camera_index = absl::GetFlag(FLAGS_native_camera_index);
  engine_config.network_camera_url = absl::GetFlag(FLAGS_network_camera_url);

  absl::InitializeLog();
  SetStderrThreshold(absl::LogSeverity::kInfo);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
  QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
  // 根据实际屏幕缩放比例更改
  qputenv("QT_SCALE_FACTOR", "1.5");
#endif
#endif
  QApplication app(argc, argv);

#ifdef ELA_GUI
  arm_face_id::FaceCamera::Settings settings;
  settings.cam_index = 1;
  arm_face_id::FaceCamera face_cam(settings);

  std::shared_ptr<arm_face_id::ElaGUI> ela_gui =
      std::make_shared<arm_face_id::ElaGUI>();
  face_cam.AddObserver(ela_gui);

  spdlog::info("正在启动 Ela GUI~ :>");

  eApp->init();
  ela_gui->show();
  ela_gui->adjustSize();
  if (!face_cam.OpenAndStart()) {
    spdlog::warn("无法启动摄像头子进程！:<");
  }
  return app.exec();
#endif

  std::shared_ptr<arm_face_id::Engine> engine_ptr =
      std::make_shared<arm_face_id::Engine>(engine_config);

  arm_face_id::RpcManagerImpl rpc_service(engine_ptr);
  grpc::ServerBuilder server_builder;
  server_builder.AddListeningPort(kServerAddrInfo,
                                  grpc::InsecureServerCredentials());
  server_builder.RegisterService(&rpc_service);
  server_builder.SetSyncServerOption(
      grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 20);
  server_builder.SetSyncServerOption(
      grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, 5);
  const std::unique_ptr rpc_server(server_builder.BuildAndStart());

  // arm_face_id::RpcManagerImpl rpc_service;

  std::shared_ptr<arm_face_id::GUI> gui_ptr =
      std::make_shared<arm_face_id::GUI>(engine_ptr);
  gui_ptr->Get()->show();

  engine_ptr->RegisterICamera(gui_ptr);
  engine_ptr->RegisterIListener(gui_ptr);
  engine_ptr->Start();

  std::thread rpc_thread([&] {
    spdlog::info("Started gRPC server.");
    rpc_server->Wait();
  });
  rpc_thread.detach();

  return app.exec();
}
