#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include "config.h"
#include "data_types.h"

class SensorManager {
private:
  MPU6050 mpu;
  CalibrationData calibration;
  SensorData rawData;
  StabilityData stabilityData;
  
  // 滤波相关
  float accelFilter[3];           // 加速度滤波器
  float gyroFilter[3];            // 陀螺仪滤波器
  float alpha = 0.8;              // 低通滤波系数
  
  // 稳定性计算相关
  float stabilityHistory[STABILITY_WINDOW_SIZE];  // 稳定性历史数据
  int historyIndex = 0;           // 历史数据索引
  bool historyFull = false;       // 历史数据是否已满
  
  // 校准相关
  bool isCalibrating = false;
  int calibrationSamples = 0;
  float calibrationSum[6] = {0};  // 累计值用于计算偏移
  
  // 内部方法
  void applyCalibration(SensorData& data);
  void applyLowPassFilter(SensorData& data);
  float calculateStabilityScore(const SensorData& data);
  void updateStabilityHistory(float score);
  float calculateVariance();
  
public:
  SensorManager();
  
  // 初始化和配置
  bool initialize();
  bool isConnected() const;
  void reset();
  
  // 校准功能
  bool startCalibration();
  bool updateCalibration();
  bool finishCalibration();
  bool isCalibrationComplete();
  bool isCalibrationInProgress() const;
  void loadCalibration();
  void saveCalibration();
  
  // 数据读取
  bool readSensorData();
  SensorData getRawData() const;
  SensorData getFilteredData() const;
  
  // 稳定性评分
  StabilityData getStabilityData() const;
  float getCurrentScore() const;
  float getAverageScore() const;
  bool isStable() const;
  bool isBreakDetected() const;
  
  // 状态检查
  bool hasError() const;
  String getErrorMessage() const;
  
  // 调试功能
  void printSensorData() const;
  void printStabilityData() const;
};

#endif // SENSOR_MANAGER_H
