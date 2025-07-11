#ifndef DIAGNOSTIC_UTILS_H
#define DIAGNOSTIC_UTILS_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class DiagnosticUtils {
private:
  static bool i2cInitialized;
  static uint32_t lastSystemReport;
  static uint32_t systemReportInterval;
  
public:
  // 系统诊断
  static void initialize();
  static void printSystemInfo();
  static void printMemoryInfo();
  static void printChipInfo();
  static void periodicSystemReport();
  
  // I2C诊断
  static bool scanI2CBus();
  static bool testI2CDevice(uint8_t address);
  static bool testI2CWithDifferentSpeeds(uint8_t address);
  static void printI2CStatus();
  static bool initializeI2CWithSpeed(uint32_t clockSpeed);
  
  // GPIO诊断
  static bool testGPIOPin(uint8_t pin, const char* pinName);
  static void testAllConfiguredPins();
  static bool checkPullupResistor(uint8_t pin);
  
  // 电源诊断
  static float measureVoltage();
  static bool checkPowerSupply();
  static void printPowerStatus();
  
  // OLED专项诊断
  static bool diagnoseOLED();
  static bool testOLEDCommunication();
  static bool testOLEDInitialization();
  static void printOLEDDiagnostics();
  
  // 错误处理
  static void playErrorSequence();
  static void reportError(const char* module, const char* error);
  static void reportWarning(const char* module, const char* warning);
  
  // 硬件自检
  static bool performHardwareSelfTest();
  static bool testSensorConnectivity();
  static bool testDisplayConnectivity();
  static bool testInputOutput();
  
  // 调试辅助
  static void hexDump(const uint8_t* data, size_t length);
  static void printBuffer(const char* name, const uint8_t* buffer, size_t size);
  static String formatBytes(size_t bytes);
  static String formatUptime(unsigned long ms);
  
  // 性能监控
  static void startPerformanceTimer(const char* name);
  static void endPerformanceTimer(const char* name);
  static void printPerformanceStats();
};

// 性能计时器结构
struct PerformanceTimer {
  const char* name;
  unsigned long startTime;
  unsigned long totalTime;
  uint32_t callCount;
};

// 全局诊断状态
struct DiagnosticStatus {
  bool systemInitialized;
  bool i2cWorking;
  bool oledWorking;
  bool sensorWorking;
  bool powerOK;
  uint32_t errorCount;
  uint32_t warningCount;
  unsigned long lastDiagnosticTime;
};

extern DiagnosticStatus g_diagnosticStatus;

// 便捷宏定义
#define DIAGNOSTIC_ERROR(module, format, ...) \
  do { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
    DiagnosticUtils::reportError(module, buffer); \
  } while(0)

#define DIAGNOSTIC_WARNING(module, format, ...) \
  do { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
    DiagnosticUtils::reportWarning(module, buffer); \
  } while(0)

#define PERFORMANCE_START(name) DiagnosticUtils::startPerformanceTimer(name)
#define PERFORMANCE_END(name) DiagnosticUtils::endPerformanceTimer(name)

// 硬件测试按钮组合
#define HARDWARE_TEST_BUTTON_COMBO_TIME 3000  // 长按3秒触发硬件测试

#endif // DIAGNOSTIC_UTILS_H
