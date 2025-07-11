#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_pm.h>
#include "config.h"
#include "data_types.h"

class PowerManager {
private:
  // 电源状态
  float batteryVoltage = 0.0;
  bool isCharging = false;
  bool lowBattery = false;
  bool criticalBattery = false;
  
  // 休眠管理
  unsigned long lastActivity = 0;
  unsigned long sleepTimeout = SLEEP_TIMEOUT;
  bool autoSleepEnabled = true;
  bool canSleep = true;
  
  // 电源监测
  unsigned long lastBatteryCheck = 0;
  const unsigned long batteryCheckInterval = 30000; // 30秒检查一次
  
  // 低功耗模式
  bool lowPowerMode = false;

  // 根据芯片类型选择合适的电源管理配置
  #ifdef CONFIG_IDF_TARGET_ESP32C3
    esp_pm_config_esp32c3_t pmConfig;
  #elif defined(CONFIG_IDF_TARGET_ESP32)
    esp_pm_config_esp32_t pmConfig;
  #else
    esp_pm_config_esp32_t pmConfig;  // 默认使用ESP32配置
  #endif
  
  // 内部方法
  void readBatteryVoltage();
  void checkBatteryStatus();
  void configurePowerManagement();
  void enterLightSleep();
  void enterDeepSleep();
  void wakeupCallback();
  
public:
  PowerManager();
  
  // 初始化
  bool initialize();
  void reset();
  
  // 电池监测
  void updateBatteryStatus();
  float getBatteryVoltage() const;
  int getBatteryPercentage() const;
  bool isLowBattery() const;
  bool isCriticalBattery() const;
  bool isBatteryCharging() const;
  
  // 活动管理
  void updateActivity();
  unsigned long getTimeSinceLastActivity() const;
  bool shouldSleep() const;
  
  // 休眠控制
  void enableAutoSleep(bool enable);
  void setSleepTimeout(unsigned long timeout);
  void preventSleep();
  void allowSleep();
  void forceSleep();
  
  // 低功耗模式
  void enableLowPowerMode(bool enable);
  bool isLowPowerMode() const;
  void optimizePowerConsumption();
  
  // 休眠模式
  void enterSleepMode();
  void wakeUp();
  bool isWakeupFromSleep() const;
  esp_sleep_wakeup_cause_t getWakeupCause() const;
  
  // 电源事件
  bool hasPowerEvent() const;
  String getPowerEventMessage() const;
  void clearPowerEvent();
  
  // 系统控制
  void shutdown();
  void restart();
  
  // 调试功能
  void printPowerInfo() const;
  void printSleepInfo() const;
  
  // 电源统计
  unsigned long getUptime() const;
  unsigned long getSleepTime() const;
  float getAveragePowerConsumption() const;
};

#endif // POWER_MANAGER_H
