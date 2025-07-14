#include "power_manager.h"
#include "diagnostic_utils.h"
#include <driver/gpio.h>

PowerManager::PowerManager() {
  lastActivity = millis();
  batteryVoltage = 3.7; // 默认电压
  isCharging = false;
  lowBattery = false;
  criticalBattery = false;
  lowPowerMode = false;
  canSleep = true;
}

bool PowerManager::initialize() {
  DEBUG_INFO("POWER", "初始化电源管理器...");

  // 配置电源管理
  configurePowerManagement();

  // 配置唤醒源 (ESP32-C3使用GPIO唤醒)
  // 按钮按下时为HIGH，所以使用高电平触发唤醒
  DEBUG_DEBUG("POWER", "配置唤醒源...");
  esp_sleep_enable_gpio_wakeup();
  gpio_wakeup_enable(GPIO_NUM_3, GPIO_INTR_HIGH_LEVEL);  // 高电平唤醒
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIMEOUT * 1000); // 定时唤醒

  DEBUG_INFO("POWER", "唤醒配置: 按钮GPIO3高电平触发");

  // 初始电池状态检查
  DEBUG_DEBUG("POWER", "检查初始电池状态...");
  updateBatteryStatus();

  // 验证电源状态
  if (!DiagnosticUtils::checkPowerSupply()) {
    DEBUG_WARN("POWER", "电源状态异常，但继续初始化");
  }

  DEBUG_INFO("POWER", "电源管理器初始化成功!");
  DEBUG_INFO("POWER", "初始电压: %.3fV, 电量: %d%%",
             batteryVoltage, getBatteryPercentage());

  return true;
}

void PowerManager::reset() {
  lastActivity = millis();
  lowPowerMode = false;
  canSleep = true;
}

void PowerManager::configurePowerManagement() {
  // 根据芯片类型配置电源管理
  #ifdef CONFIG_IDF_TARGET_ESP32C3
    // ESP32-C3配置
    pmConfig.max_freq_mhz = 160;        // 最大频率160MHz
    pmConfig.min_freq_mhz = 10;         // 最小频率10MHz
    pmConfig.light_sleep_enable = true;  // 启用轻度睡眠
    DEBUG_INFO("POWER", "配置ESP32-C3电源管理");
  #elif defined(CONFIG_IDF_TARGET_ESP32)
    // ESP32配置
    pmConfig.max_freq_mhz = 240;        // ESP32最大频率240MHz
    pmConfig.min_freq_mhz = 10;         // 最小频率10MHz
    pmConfig.light_sleep_enable = true;  // 启用轻度睡眠
    DEBUG_INFO("POWER", "配置ESP32电源管理");
  #else
    // 默认配置
    pmConfig.max_freq_mhz = 160;
    pmConfig.min_freq_mhz = 10;
    pmConfig.light_sleep_enable = true;
    DEBUG_INFO("POWER", "配置默认电源管理");
  #endif

  esp_err_t ret = esp_pm_configure(&pmConfig);
  if (ret != ESP_OK) {
    DEBUG_ERROR("POWER", "电源管理配置失败: %s", esp_err_to_name(ret));
  } else {
    DEBUG_INFO("POWER", "电源管理配置成功");
  }
}

void PowerManager::updateBatteryStatus() {
  unsigned long currentTime = millis();
  if (currentTime - lastBatteryCheck < batteryCheckInterval) {
    return;
  }
  
  readBatteryVoltage();
  checkBatteryStatus();
  
  lastBatteryCheck = currentTime;
}

void PowerManager::readBatteryVoltage() {
  DEBUG_VERBOSE("POWER", "读取电池电压...");

  // ESP32-C3 SuperMini通常没有专门的电池监测引脚
  // 这里使用简化的电压读取方法
  // 实际项目中可能需要外部分压电路

  float voltageSum = 0;
  int validSamples = 0;

  // 多次采样求平均值
  for (int i = 0; i < VOLTAGE_SAMPLES; i++) {
    // 简化处理，使用固定值模拟
    // 实际应用中需要根据硬件设计实现ADC读取
    float sample = 3.3f + (random(-100, 100) / 1000.0f); // 模拟电压波动

    // 简单的有效性检查
    if (sample > 0 && sample < 5.0f) {
      voltageSum += sample;
      validSamples++;
    }

    delay(VOLTAGE_SAMPLE_INTERVAL / VOLTAGE_SAMPLES);
  }

  if (validSamples > 0) {
    float newVoltage = voltageSum / validSamples;

    // 简单的滤波
    if (batteryVoltage == 0) {
      batteryVoltage = newVoltage;
    } else {
      batteryVoltage = batteryVoltage * 0.8f + newVoltage * 0.2f; // 低通滤波
    }

    DEBUG_VERBOSE("POWER", "电压采样: %.3fV (有效样本: %d/%d)",
                  batteryVoltage, validSamples, VOLTAGE_SAMPLES);
  } else {
    DEBUG_WARN("POWER", "电压采样失败，无有效样本");
  }

  // 检测充电状态 (需要硬件支持)
  // 这里简化处理
  isCharging = false;
}

void PowerManager::checkBatteryStatus() {
  // 检查低电量
  if (batteryVoltage <= BATTERY_LOW_VOLTAGE && !lowBattery) {
    lowBattery = true;
    DEBUG_PRINTLN("电池电量低!");
  } else if (batteryVoltage > BATTERY_LOW_VOLTAGE + 0.1) {
    lowBattery = false;
  }
  
  // 检查严重低电量
  if (batteryVoltage <= BATTERY_CRITICAL_VOLTAGE && !criticalBattery) {
    criticalBattery = true;
    DEBUG_PRINTLN("电池电量严重不足!");
  } else if (batteryVoltage > BATTERY_CRITICAL_VOLTAGE + 0.1) {
    criticalBattery = false;
  }
}

float PowerManager::getBatteryVoltage() const {
  return batteryVoltage;
}

int PowerManager::getBatteryPercentage() const {
  // 将电压转换为百分比 (3.0V-4.2V范围)
  float percentage = (batteryVoltage - 3.0) / 1.2 * 100.0;
  return constrain((int)percentage, 0, 100);
}

bool PowerManager::isLowBattery() const {
  return lowBattery;
}

bool PowerManager::isCriticalBattery() const {
  return criticalBattery;
}

bool PowerManager::isBatteryCharging() const {
  return isCharging;
}

void PowerManager::updateActivity() {
  lastActivity = millis();
}

unsigned long PowerManager::getTimeSinceLastActivity() const {
  return millis() - lastActivity;
}

bool PowerManager::shouldSleep() const {
  if (!autoSleepEnabled || !canSleep) {
    return false;
  }
  
  return getTimeSinceLastActivity() >= sleepTimeout;
}

void PowerManager::enableAutoSleep(bool enable) {
  autoSleepEnabled = enable;
  DEBUG_PRINTF("自动休眠%s\n", enable ? "开启" : "关闭");
}

void PowerManager::setSleepTimeout(unsigned long timeout) {
  sleepTimeout = timeout;
  DEBUG_PRINTF("休眠超时设置为: %lu ms\n", timeout);
}

void PowerManager::preventSleep() {
  canSleep = false;
  DEBUG_PRINTLN("休眠已禁用");
}

void PowerManager::allowSleep() {
  canSleep = true;
  DEBUG_PRINTLN("休眠已启用");
}

void PowerManager::forceSleep() {
  DEBUG_PRINTLN("强制进入休眠模式");
  enterSleepMode();
}

void PowerManager::enableLowPowerMode(bool enable) {
  lowPowerMode = enable;
  
  if (enable) {
    // 降低CPU频率
    setCpuFrequencyMhz(80);
    DEBUG_PRINTLN("进入低功耗模式");
  } else {
    // 恢复正常频率
    setCpuFrequencyMhz(160);
    DEBUG_PRINTLN("退出低功耗模式");
  }
}

bool PowerManager::isLowPowerMode() const {
  return lowPowerMode;
}

void PowerManager::optimizePowerConsumption() {
  // 优化功耗设置
  if (lowPowerMode) {
    // 已经在低功耗模式
    return;
  }
  
  // 根据电池状态调整功耗
  if (lowBattery) {
    enableLowPowerMode(true);
  }
  
  // 其他功耗优化措施
  // 例如：降低显示亮度、减少传感器采样频率等
}

void PowerManager::enterSleepMode() {
  DEBUG_PRINTLN("准备进入休眠模式...");
  
  // 保存当前状态
  // 这里可以保存重要数据
  
  // 配置唤醒源 (ESP32-C3)
  // 按钮按下时为HIGH，使用高电平触发唤醒
  esp_sleep_enable_gpio_wakeup();
  gpio_wakeup_enable(GPIO_NUM_3, GPIO_INTR_HIGH_LEVEL); // 按钮唤醒 (高电平)
  
  // 根据电池状态选择休眠模式
  if (criticalBattery) {
    DEBUG_PRINTLN("进入深度休眠模式");
    enterDeepSleep();
  } else {
    DEBUG_PRINTLN("进入轻度休眠模式");
    enterLightSleep();
  }
}

void PowerManager::enterLightSleep() {
  // 轻度休眠，保持RAM数据
  esp_light_sleep_start();
  
  // 唤醒后继续执行
  DEBUG_PRINTLN("从轻度休眠中唤醒");
  
  // 调用唤醒处理函数
  wakeUp();
}

void PowerManager::enterDeepSleep() {
  // 深度休眠，RAM数据丢失
  esp_deep_sleep_start();
  
  // 这行代码不会执行，因为深度休眠会重启系统
}

void PowerManager::wakeUp() {
  updateActivity();
  DEBUG_PRINTLN("系统唤醒");
  
  // 设置唤醒标志，让主程序知道需要恢复显示状态
  wakeupFlag = true;
}

bool PowerManager::isWakeupFromSleep() const {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  return wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED;
}

esp_sleep_wakeup_cause_t PowerManager::getWakeupCause() const {
  return esp_sleep_get_wakeup_cause();
}

bool PowerManager::hasPowerEvent() const {
  return lowBattery || criticalBattery;
}

String PowerManager::getPowerEventMessage() const {
  if (criticalBattery) {
    return "电池电量严重不足，请立即充电!";
  } else if (lowBattery) {
    return "电池电量低，请及时充电";
  }
  return "";
}

void PowerManager::clearPowerEvent() {
  // 清除电源事件标志
  // 实际实现中可能需要更复杂的逻辑
}

void PowerManager::shutdown() {
  DEBUG_PRINTLN("系统关闭中...");
  
  // 保存重要数据
  // 关闭外设
  // 进入深度休眠
  esp_deep_sleep_start();
}

void PowerManager::restart() {
  DEBUG_PRINTLN("系统重启中...");
  ESP.restart();
}

unsigned long PowerManager::getUptime() const {
  return millis();
}

void PowerManager::printPowerInfo() const {
  DEBUG_PRINTLN("=== 电源信息 ===");
  DEBUG_PRINTF("电池电压: %.2f V\n", batteryVoltage);
  DEBUG_PRINTF("电池百分比: %d%%\n", getBatteryPercentage());
  DEBUG_PRINTF("充电状态: %s\n", isCharging ? "充电中" : "未充电");
  DEBUG_PRINTF("低电量: %s\n", lowBattery ? "是" : "否");
  DEBUG_PRINTF("严重低电量: %s\n", criticalBattery ? "是" : "否");
  DEBUG_PRINTF("低功耗模式: %s\n", lowPowerMode ? "是" : "否");
  DEBUG_PRINTF("运行时间: %lu ms\n", getUptime());
}

void PowerManager::printSleepInfo() const {
  DEBUG_PRINTLN("=== 休眠信息 ===");
  DEBUG_PRINTF("自动休眠: %s\n", autoSleepEnabled ? "开启" : "关闭");
  DEBUG_PRINTF("可以休眠: %s\n", canSleep ? "是" : "否");
  DEBUG_PRINTF("休眠超时: %lu ms\n", sleepTimeout);
  DEBUG_PRINTF("距离上次活动: %lu ms\n", getTimeSinceLastActivity());
  DEBUG_PRINTF("应该休眠: %s\n", shouldSleep() ? "是" : "否");
  
  if (isWakeupFromSleep()) {
    esp_sleep_wakeup_cause_t wakeup_reason = getWakeupCause();
    DEBUG_PRINTF("唤醒原因: ");
    switch (wakeup_reason) {
      case ESP_SLEEP_WAKEUP_EXT0:
        DEBUG_PRINTLN("外部中断");
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        DEBUG_PRINTLN("定时器");
        break;
      default:
        DEBUG_PRINTLN("其他");
        break;
    }
  }
}

bool PowerManager::hasWakeupEvent() const {
  return wakeupFlag;
}

void PowerManager::clearWakeupEvent() {
  wakeupFlag = false;
  DEBUG_DEBUG("POWER", "唤醒事件已清除");
}
