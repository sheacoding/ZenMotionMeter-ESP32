#include "sensor_manager.h"
#include <EEPROM.h>
#include <math.h>

SensorManager::SensorManager() {
  // 初始化滤波器
  for (int i = 0; i < 3; i++) {
    accelFilter[i] = 0.0;
    gyroFilter[i] = 0.0;
  }
  
  // 初始化稳定性历史数据
  for (int i = 0; i < STABILITY_WINDOW_SIZE; i++) {
    stabilityHistory[i] = 0.0;
  }
  
  // 初始化稳定性数据
  stabilityData.score = 0.0;
  stabilityData.avgScore = 0.0;
  stabilityData.variance = 0.0;
  stabilityData.isStable = false;
  stabilityData.lastBreakTime = 0;
  stabilityData.breakCount = 0;
}

bool SensorManager::initialize() {
  // 初始化I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000); // 400kHz
  
  // 初始化MPU6050
  mpu.initialize();
  
  // 检查连接
  if (!mpu.testConnection()) {
    DEBUG_PRINTLN("MPU6050连接失败!");
    return false;
  }
  
  // 配置MPU6050
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);  // ±2g
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);  // ±250°/s
  mpu.setDLPFMode(MPU6050_DLPF_BW_20);             // 20Hz低通滤波
  mpu.setRate(MPU6050_SAMPLE_RATE - 1);            // 设置采样率
  
  // 加载校准数据
  loadCalibration();
  
  DEBUG_PRINTLN("MPU6050初始化成功!");
  return true;
}

bool SensorManager::isConnected() const {
  return const_cast<MPU6050&>(mpu).testConnection();
}

void SensorManager::reset() {
  // 重置滤波器
  for (int i = 0; i < 3; i++) {
    accelFilter[i] = 0.0;
    gyroFilter[i] = 0.0;
  }
  
  // 重置稳定性历史
  historyIndex = 0;
  historyFull = false;
  for (int i = 0; i < STABILITY_WINDOW_SIZE; i++) {
    stabilityHistory[i] = 0.0;
  }
  
  // 重置稳定性数据
  stabilityData.score = 0.0;
  stabilityData.avgScore = 0.0;
  stabilityData.variance = 0.0;
  stabilityData.isStable = false;
  stabilityData.breakCount = 0;
}

bool SensorManager::startCalibration() {
  if (isCalibrating) {
    return false;
  }
  
  isCalibrating = true;
  calibrationSamples = 0;
  
  // 清零累计值
  for (int i = 0; i < 6; i++) {
    calibrationSum[i] = 0.0;
  }
  
  DEBUG_PRINTLN("开始校准...");
  return true;
}

bool SensorManager::updateCalibration() {
  if (!isCalibrating) {
    return false;
  }
  
  // 读取原始数据
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // 累计数据
  calibrationSum[0] += ax / ACCEL_SCALE_FACTOR;
  calibrationSum[1] += ay / ACCEL_SCALE_FACTOR;
  calibrationSum[2] += az / ACCEL_SCALE_FACTOR;
  calibrationSum[3] += gx / GYRO_SCALE_FACTOR;
  calibrationSum[4] += gy / GYRO_SCALE_FACTOR;
  calibrationSum[5] += gz / GYRO_SCALE_FACTOR;
  
  calibrationSamples++;
  
  return calibrationSamples < CALIBRATION_SAMPLES;
}

bool SensorManager::finishCalibration() {
  if (!isCalibrating || calibrationSamples < CALIBRATION_SAMPLES) {
    return false;
  }
  
  // 计算偏移值
  calibration.accelOffsetX = calibrationSum[0] / calibrationSamples;
  calibration.accelOffsetY = calibrationSum[1] / calibrationSamples;
  calibration.accelOffsetZ = (calibrationSum[2] / calibrationSamples) - 1.0; // 重力补偿
  calibration.gyroOffsetX = calibrationSum[3] / calibrationSamples;
  calibration.gyroOffsetY = calibrationSum[4] / calibrationSamples;
  calibration.gyroOffsetZ = calibrationSum[5] / calibrationSamples;
  
  calibration.isCalibrated = true;
  calibration.calibrationTime = millis();
  
  // 保存校准数据
  saveCalibration();
  
  isCalibrating = false;
  DEBUG_PRINTLN("校准完成!");
  
  return true;
}

bool SensorManager::isCalibrationComplete() {
  return calibration.isCalibrated;
}

bool SensorManager::isCalibrationInProgress() const {
  return isCalibrating;
}

void SensorManager::loadCalibration() {
  // 从EEPROM加载校准数据
  EEPROM.begin(EEPROM_SIZE);
  
  // 检查校准数据有效性标志
  uint8_t validFlag = EEPROM.read(EEPROM_SETTINGS_ADDR + 50);
  if (validFlag == 0xAA) {
    EEPROM.get(EEPROM_SETTINGS_ADDR + 51, calibration);
    DEBUG_PRINTLN("校准数据已加载");
  } else {
    // 使用默认校准数据
    calibration.accelOffsetX = 0.0;
    calibration.accelOffsetY = 0.0;
    calibration.accelOffsetZ = 0.0;
    calibration.gyroOffsetX = 0.0;
    calibration.gyroOffsetY = 0.0;
    calibration.gyroOffsetZ = 0.0;
    calibration.isCalibrated = false;
    DEBUG_PRINTLN("使用默认校准数据");
  }
}

void SensorManager::saveCalibration() {
  // 保存校准数据到EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(EEPROM_SETTINGS_ADDR + 50, 0xAA); // 有效性标志
  EEPROM.put(EEPROM_SETTINGS_ADDR + 51, calibration);
  EEPROM.commit();
  DEBUG_PRINTLN("校准数据已保存");
}

bool SensorManager::readSensorData() {
  if (isCalibrating) {
    return updateCalibration();
  }
  
  // 读取原始数据
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // 转换为物理单位
  rawData.accelX = ax / ACCEL_SCALE_FACTOR;
  rawData.accelY = ay / ACCEL_SCALE_FACTOR;
  rawData.accelZ = az / ACCEL_SCALE_FACTOR;
  rawData.gyroX = gx / GYRO_SCALE_FACTOR;
  rawData.gyroY = gy / GYRO_SCALE_FACTOR;
  rawData.gyroZ = gz / GYRO_SCALE_FACTOR;
  rawData.timestamp = millis();
  
  // 应用校准
  applyCalibration(rawData);
  
  // 应用滤波
  applyLowPassFilter(rawData);
  
  // 计算稳定性评分
  float score = calculateStabilityScore(rawData);
  updateStabilityHistory(score);
  
  // 更新稳定性数据
  stabilityData.score = score;
  stabilityData.avgScore = getAverageScore();
  stabilityData.variance = calculateVariance();
  stabilityData.acceleration_magnitude = sqrt(rawData.accelX * rawData.accelX + 
                                             rawData.accelY * rawData.accelY + 
                                             rawData.accelZ * rawData.accelZ);
  stabilityData.gyro_magnitude = sqrt(rawData.gyroX * rawData.gyroX + 
                                     rawData.gyroY * rawData.gyroY + 
                                     rawData.gyroZ * rawData.gyroZ);
  
  // 检查是否稳定
  stabilityData.isStable = (score >= STABILITY_THRESHOLD);
  
  // 检查是否破定
  if (!stabilityData.isStable && (millis() - stabilityData.lastBreakTime) > 1000) {
    stabilityData.breakCount++;
    stabilityData.lastBreakTime = millis();
  }
  
  return true;
}

void SensorManager::applyCalibration(SensorData& data) {
  if (calibration.isCalibrated) {
    data.accelX -= calibration.accelOffsetX;
    data.accelY -= calibration.accelOffsetY;
    data.accelZ -= calibration.accelOffsetZ;
    data.gyroX -= calibration.gyroOffsetX;
    data.gyroY -= calibration.gyroOffsetY;
    data.gyroZ -= calibration.gyroOffsetZ;
  }
}

void SensorManager::applyLowPassFilter(SensorData& data) {
  // 低通滤波器：y[n] = α * x[n] + (1-α) * y[n-1]
  accelFilter[0] = alpha * data.accelX + (1.0 - alpha) * accelFilter[0];
  accelFilter[1] = alpha * data.accelY + (1.0 - alpha) * accelFilter[1];
  accelFilter[2] = alpha * data.accelZ + (1.0 - alpha) * accelFilter[2];

  gyroFilter[0] = alpha * data.gyroX + (1.0 - alpha) * gyroFilter[0];
  gyroFilter[1] = alpha * data.gyroY + (1.0 - alpha) * gyroFilter[1];
  gyroFilter[2] = alpha * data.gyroZ + (1.0 - alpha) * gyroFilter[2];

  // 更新滤波后的数据
  data.accelX = accelFilter[0];
  data.accelY = accelFilter[1];
  data.accelZ = accelFilter[2];
  data.gyroX = gyroFilter[0];
  data.gyroY = gyroFilter[1];
  data.gyroZ = gyroFilter[2];
}

float SensorManager::calculateStabilityScore(const SensorData& data) {
  // 计算加速度和角速度的总幅值
  float accelMagnitude = sqrt(data.accelX * data.accelX +
                             data.accelY * data.accelY +
                             data.accelZ * data.accelZ);

  float gyroMagnitude = sqrt(data.gyroX * data.gyroX +
                            data.gyroY * data.gyroY +
                            data.gyroZ * data.gyroZ);

  // 计算与重力的偏差（理想情况下应该接近1g）
  float gravityDeviation = abs(accelMagnitude - 1.0);

  // 稳定性评分算法
  // 基于加速度偏差和角速度的综合评分
  float accelScore = max(0.0f, 100.0f - gravityDeviation * 200.0f);
  float gyroScore = max(0.0f, 100.0f - gyroMagnitude * 10.0f);

  // 综合评分（加权平均）
  float score = (accelScore * 0.6f + gyroScore * 0.4f);

  // 限制评分范围
  return constrain(score, 0.0f, 100.0f);
}

void SensorManager::updateStabilityHistory(float score) {
  stabilityHistory[historyIndex] = score;
  historyIndex = (historyIndex + 1) % STABILITY_WINDOW_SIZE;

  if (!historyFull && historyIndex == 0) {
    historyFull = true;
  }
}

float SensorManager::calculateVariance() {
  if (!historyFull && historyIndex < 2) {
    return 0.0;
  }

  int count = historyFull ? STABILITY_WINDOW_SIZE : historyIndex;
  float mean = getAverageScore();
  float variance = 0.0;

  for (int i = 0; i < count; i++) {
    float diff = stabilityHistory[i] - mean;
    variance += diff * diff;
  }

  return variance / count;
}

SensorData SensorManager::getRawData() const {
  return rawData;
}

SensorData SensorManager::getFilteredData() const {
  SensorData filtered = rawData;
  filtered.accelX = accelFilter[0];
  filtered.accelY = accelFilter[1];
  filtered.accelZ = accelFilter[2];
  filtered.gyroX = gyroFilter[0];
  filtered.gyroY = gyroFilter[1];
  filtered.gyroZ = gyroFilter[2];
  return filtered;
}

StabilityData SensorManager::getStabilityData() const {
  return stabilityData;
}

float SensorManager::getCurrentScore() const {
  return stabilityData.score;
}

float SensorManager::getAverageScore() const {
  if (!historyFull && historyIndex == 0) {
    return 0.0;
  }

  int count = historyFull ? STABILITY_WINDOW_SIZE : historyIndex;
  float sum = 0.0;

  for (int i = 0; i < count; i++) {
    sum += stabilityHistory[i];
  }

  return sum / count;
}

bool SensorManager::isStable() const {
  return stabilityData.isStable;
}

bool SensorManager::isBreakDetected() const {
  return !stabilityData.isStable &&
         (millis() - stabilityData.lastBreakTime) < 2000;
}

bool SensorManager::hasError() const {
  return !isConnected();
}

String SensorManager::getErrorMessage() const {
  if (!isConnected()) {
    return "传感器连接错误";
  }
  return "";
}

void SensorManager::printSensorData() const {
  DEBUG_PRINTF("加速度: X=%.3f Y=%.3f Z=%.3f | ",
               rawData.accelX, rawData.accelY, rawData.accelZ);
  DEBUG_PRINTF("陀螺仪: X=%.3f Y=%.3f Z=%.3f\n",
               rawData.gyroX, rawData.gyroY, rawData.gyroZ);
}

void SensorManager::printStabilityData() const {
  DEBUG_PRINTF("稳定性评分: %.1f | 平均: %.1f | 方差: %.2f | %s\n",
               stabilityData.score, stabilityData.avgScore,
               stabilityData.variance,
               stabilityData.isStable ? "稳定" : "不稳定");
}
