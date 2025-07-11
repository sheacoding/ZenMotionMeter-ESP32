#include <Arduino.h>
#include "config.h"
#include "data_types.h"
#include "sensor_manager.h"
#include "display_manager.h"
#include "input_manager.h"
#include "data_manager.h"
#include "power_manager.h"
#include "diagnostic_utils.h"

// ==================== 全局对象 ====================
SensorManager sensorManager;
DisplayManager displayManager;
InputManager inputManager;
DataManager dataManager;
PowerManager powerManager;

// ==================== 全局数据 ====================
ZenMotionData zenData;

// ==================== 系统状态 ====================
SystemState currentState = STATE_BOOT_ANIMATION;
unsigned long lastUpdate = 0;
unsigned long lastSensorRead = 0;
bool systemInitialized = false;

// ==================== 函数声明 ====================
void initializeSystem();
void updateSensors();
void updateDisplay();
void handleInput();
void updatePower();
void saveData();
void handleSystemState();
void handleBootAnimationState();
void handleMainMenuState();
void handleIdleState();
void handleCalibratingState();
void handlePracticingState();
void handlePausedState();
void handleMenuState();
void handleSettingsState();
void handleHistoryState();
void handleSleepState();
void handleSingleClick();
void handleLongPress();
void handleDoubleClick();
void printSystemInfo();

// 状态管理函数
bool isValidStateTransition(SystemState from, SystemState to);
void changeSystemState(SystemState newState, const char* reason);
void updateDisplayForState();
void validateSystemConsistency();

void setup() {
  // 初始化调试串口
  DEBUG_INIT();

  DEBUG_INFO("MAIN", "=================================");
  DEBUG_INFO("MAIN", "气定神闲仪 (Zen-Motion Meter)");
  DEBUG_INFO("MAIN", "版本: %s", PROJECT_VERSION);
  DEBUG_INFO("MAIN", "=================================");

  // 输出引脚配置信息
  PRINT_PIN_CONFIG();

  // 初始化诊断系统
  DiagnosticUtils::initialize();

  // 检查是否需要硬件自检 (根据开发板配置检测按钮状态)
  pinMode(BUTTON_PIN, BUTTON_PIN_MODE);
  delay(50); // 等待引脚稳定
  if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_STATE) {
    DEBUG_INFO("MAIN", "检测到按钮按下 (%s)，启动硬件自检...",
               (BUTTON_PRESSED_STATE == HIGH) ? "HIGH" : "LOW");
    delay(HARDWARE_TEST_BUTTON_COMBO_TIME);
    if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_STATE) {
      DEBUG_INFO("MAIN", "执行完整硬件自检");
      DiagnosticUtils::performHardwareSelfTest();
      delay(2000);
    } else {
      DEBUG_INFO("MAIN", "按钮已释放，取消硬件自检");
    }
  } else {
    DEBUG_DEBUG("MAIN", "按钮未按下 (%s)，正常启动",
                (BUTTON_RELEASED_STATE == HIGH) ? "HIGH" : "LOW");
  }

  // 初始化系统
  initializeSystem();

  DEBUG_INFO("MAIN", "系统初始化完成，开始运行...");
  DEBUG_MEMORY();
}

void loop() {
  if (!systemInitialized) {
    delay(100);
    return;
  }

  unsigned long currentTime = millis();

  // 更新传感器数据
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    updateSensors();
    lastSensorRead = currentTime;
  }

  // 处理输入
  handleInput();

  // 更新电源管理
  updatePower();

  // 处理系统状态
  handleSystemState();

  // 更新显示
  if (currentTime - lastUpdate >= DISPLAY_UPDATE_INTERVAL) {
    updateDisplay();
    lastUpdate = currentTime;
  }

  // 保存数据
  saveData();

  // 定期系统报告
  DiagnosticUtils::periodicSystemReport();

  // 短暂延迟
  delay(10);
}

void initializeSystem() {
  DEBUG_INFO("INIT", "开始初始化系统...");
  PERFORMANCE_START("SYSTEM_INIT");

  // 打印系统信息
  DEBUG_SYSTEM_INFO();

  // 初始化电源管理器
  DEBUG_INFO("INIT", "初始化电源管理器...");
  PERFORMANCE_START("POWER_INIT");
  if (!powerManager.initialize()) {
    DEBUG_ERROR("INIT", "电源管理器初始化失败!");
    DiagnosticUtils::reportError("INIT", "电源管理器初始化失败");
    return;
  }
  PERFORMANCE_END("POWER_INIT");
  DEBUG_INFO("INIT", "✓ 电源管理器初始化成功");

  // 检查唤醒原因
  if (powerManager.isWakeupFromSleep()) {
    DEBUG_INFO("INIT", "从休眠中唤醒");
    esp_sleep_wakeup_cause_t wakeup_reason = powerManager.getWakeupCause();
    switch (wakeup_reason) {
      case ESP_SLEEP_WAKEUP_GPIO:
        DEBUG_INFO("INIT", "GPIO唤醒 (按钮)");
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        DEBUG_INFO("INIT", "定时器唤醒");
        break;
      default:
        DEBUG_INFO("INIT", "其他原因唤醒: %d", wakeup_reason);
        break;
    }
  }

  // 初始化数据管理器
  DEBUG_INFO("INIT", "初始化数据管理器...");
  PERFORMANCE_START("DATA_INIT");
  if (!dataManager.initialize()) {
    DEBUG_ERROR("INIT", "数据管理器初始化失败!");
    DiagnosticUtils::reportError("INIT", "数据管理器初始化失败");
    return;
  }
  PERFORMANCE_END("DATA_INIT");
  DEBUG_INFO("INIT", "✓ 数据管理器初始化成功");

  // 初始化输入管理器
  DEBUG_INFO("INIT", "初始化输入管理器...");
  PERFORMANCE_START("INPUT_INIT");
  if (!inputManager.initialize()) {
    DEBUG_ERROR("INIT", "输入管理器初始化失败!");
    DiagnosticUtils::reportError("INIT", "输入管理器初始化失败");
    return;
  }
  PERFORMANCE_END("INPUT_INIT");
  DEBUG_INFO("INIT", "✓ 输入管理器初始化成功");

  // 初始化显示管理器 (包含详细的OLED诊断)
  DEBUG_INFO("INIT", "初始化显示管理器...");
  PERFORMANCE_START("DISPLAY_INIT");
  if (!displayManager.initialize()) {
    DEBUG_ERROR("INIT", "显示管理器初始化失败!");
    DiagnosticUtils::reportError("INIT", "OLED显示屏初始化失败");
    DiagnosticUtils::printOLEDDiagnostics();
    return;
  }
  PERFORMANCE_END("DISPLAY_INIT");
  DEBUG_INFO("INIT", "✓ 显示管理器初始化成功");

  // 初始化传感器管理器
  DEBUG_INFO("INIT", "初始化传感器管理器...");
  PERFORMANCE_START("SENSOR_INIT");
  if (!sensorManager.initialize()) {
    DEBUG_ERROR("INIT", "传感器管理器初始化失败!");
    DiagnosticUtils::reportError("INIT", "MPU6050传感器初始化失败");
    if (displayManager.isReady()) {
      displayManager.showError("传感器初始化失败");
    }
    return;
  }
  PERFORMANCE_END("SENSOR_INIT");
  DEBUG_INFO("INIT", "✓ 传感器管理器初始化成功");

  // 加载设置
  zenData.settings = dataManager.getSettings();

  // 设置音频状态
  inputManager.setAudioEnabled(zenData.settings.soundEnabled);

  // 设置显示亮度
  displayManager.setBrightness(zenData.settings.displayBrightness);

  // 初始化系统状态
  zenData.status.currentState = STATE_IDLE;
  zenData.status.currentPage = PAGE_MAIN;
  zenData.status.lastActivity = millis();
  zenData.status.uptime = millis();
  zenData.status.sensorError = false;
  zenData.status.displayError = false;

  // 播放启动音
  inputManager.playStartSound();

  systemInitialized = true;
  currentState = STATE_IDLE;

  PERFORMANCE_END("SYSTEM_INIT");
  DEBUG_INFO("INIT", "✓ 系统初始化完成!");

  // 打印性能统计
  DiagnosticUtils::printPerformanceStats();

  // 最终内存检查
  DEBUG_MEMORY();
}

void updateSensors() {
  // 读取传感器数据
  if (sensorManager.readSensorData()) {
    zenData.sensor = sensorManager.getRawData();
    zenData.stability = sensorManager.getStabilityData();

    // 更新会话稳定性数据
    if (dataManager.isSessionActive()) {
      dataManager.updateSessionStability(zenData.stability.score);
    }

    // 检查破定事件
    if (sensorManager.isBreakDetected()) {
      dataManager.addBreakEvent();

      // 播放破定提醒音
      if (zenData.settings.soundEnabled && currentState == STATE_PRACTICING) {
        inputManager.playBreakWarning();
      }
    }

    zenData.status.sensorError = false;
  } else {
    zenData.status.sensorError = true;
    DEBUG_PRINTLN("传感器读取错误");
  }
}

void updateDisplay() {
  // 更新显示数据
  zenData.currentSession = dataManager.getCurrentSession();
  zenData.todayStats = dataManager.getTodayStats();
  zenData.settings = dataManager.getSettings();
  zenData.status.batteryVoltage = powerManager.getBatteryVoltage();
  zenData.status.isCharging = powerManager.isBatteryCharging();
  zenData.status.lowBattery = powerManager.isLowBattery();

  // 更新系统状态相关数据
  zenData.status.currentState = currentState;
  zenData.status.uptime = millis();

  // 根据当前状态更新特定数据
  switch (currentState) {
    case STATE_PRACTICING:
    case STATE_PAUSED:
      // 确保会话数据是最新的
      zenData.currentSession = dataManager.getCurrentSession();
      break;

    case STATE_CALIBRATING:
      // 校准状态下的特殊处理
      DEBUG_DEBUG("DISPLAY", "校准模式显示更新");
      break;

    default:
      break;
  }

  // 更新显示
  displayManager.update(zenData);

  DEBUG_DEBUG("DISPLAY", "显示已更新，状态: %d", currentState);
}

void handleInput() {
  inputManager.update();

  if (inputManager.hasButtonEvent()) {
    ButtonEvent event = inputManager.getButtonEvent();

    // 更新活动时间
    powerManager.updateActivity();
    zenData.status.lastActivity = millis();

    // 添加调试信息
    DEBUG_INFO("INPUT", "检测到按钮事件: %d", event.state);

    switch (event.state) {
      case BUTTON_PRESSED:
        DEBUG_INFO("INPUT", "处理单击事件");
        handleSingleClick();
        break;
      case BUTTON_LONG_PRESSED:
        DEBUG_INFO("INPUT", "处理长按事件");
        handleLongPress();
        break;
      case BUTTON_DOUBLE_PRESSED:
        DEBUG_INFO("INPUT", "处理双击事件");
        handleDoubleClick();
        break;
      default:
        DEBUG_WARN("INPUT", "未知按钮事件: %d", event.state);
        break;
    }

    inputManager.clearButtonEvent();
  }
}

void handleSingleClick() {
  DEBUG_INFO("INPUT", "单击按钮，当前状态: %d", currentState);

  switch (currentState) {
    case STATE_BOOT_ANIMATION:
      // 开机动画期间忽略按钮
      DEBUG_INFO("STATE", "开机动画中，忽略单击");
      break;

    case STATE_MAIN_MENU:
      // 在主菜单中切换选项
      DEBUG_INFO("STATE", "主菜单中切换选项");
      displayManager.nextMenuOption();
      DEBUG_INFO("DISPLAY", "切换到菜单选项: %d", displayManager.getCurrentMenuOption());
      break;

    case STATE_IDLE:
      // 返回主菜单
      DEBUG_INFO("STATE", "从空闲状态返回主菜单");
      changeSystemState(STATE_MAIN_MENU, "单击返回主菜单");
      displayManager.showMessage("返回主菜单", 500);
      break;

    case STATE_PRACTICING:
      // 暂停练习
      DEBUG_INFO("STATE", "暂停练习");
      dataManager.pauseSession();
      changeSystemState(STATE_PAUSED, "单击暂停练习");
      displayManager.showMessage("练习暂停", 1000);
      break;

    case STATE_PAUSED:
      // 恢复练习
      DEBUG_INFO("STATE", "恢复练习");
      dataManager.resumeSession();
      changeSystemState(STATE_PRACTICING, "单击恢复练习");
      displayManager.showMessage("恢复练习", 1000);
      break;

    case STATE_MENU:
      // 切换页面
      DEBUG_INFO("STATE", "在菜单中切换页面");
      displayManager.nextPage();
      DEBUG_INFO("DISPLAY", "切换到页面: %d", displayManager.getCurrentPage());
      break;

    case STATE_CALIBRATING:
      // 校准过程中不响应单击
      DEBUG_INFO("STATE", "校准中，忽略单击");
      displayManager.showMessage("校准中...", 500);
      break;

    case STATE_SETTINGS:
    case STATE_HISTORY:
      // 在设置或历史页面中的导航
      DEBUG_INFO("STATE", "页面内导航");
      displayManager.showMessage("导航功能", 500);
      break;

    default:
      DEBUG_WARN("STATE", "未知状态: %d", currentState);
      break;
  }
}

void handleLongPress() {
  DEBUG_INFO("INPUT", "长按按钮，当前状态: %d", currentState);

  switch (currentState) {
    case STATE_BOOT_ANIMATION:
      // 开机动画期间忽略按钮
      DEBUG_INFO("STATE", "开机动画中，忽略长按");
      break;

    case STATE_MAIN_MENU:
      {
        // 确认选择菜单项
        DEBUG_INFO("STATE", "确认菜单选择");
        MainMenuOption selectedOption = displayManager.getSelectedMenuOption();

        switch (selectedOption) {
        case MENU_START_PRACTICE:
          DEBUG_INFO("STATE", "选择开始练习");
          dataManager.startSession();
          changeSystemState(STATE_PRACTICING, "菜单选择开始练习");
          inputManager.playStartSound();
          displayManager.showMessage("开始练习", 1000);
          break;

        case MENU_HISTORY_DATA:
          DEBUG_INFO("STATE", "选择历史数据");
          changeSystemState(STATE_HISTORY, "菜单选择历史数据");
          displayManager.showMessage("历史数据", 1000);
          break;

        case MENU_SYSTEM_SETTINGS:
          DEBUG_INFO("STATE", "选择系统设置");
          changeSystemState(STATE_SETTINGS, "菜单选择系统设置");
          displayManager.showMessage("系统设置", 1000);
          break;

        case MENU_SENSOR_CALIBRATION:
          DEBUG_INFO("STATE", "选择传感器校准");
          if (sensorManager.startCalibration()) {
            changeSystemState(STATE_CALIBRATING, "菜单选择传感器校准");
            displayManager.showMessage("开始校准", 1000);
          } else {
            DEBUG_ERROR("STATE", "校准启动失败");
            displayManager.showMessage("校准失败", 1000);
          }
          break;

        default:
          DEBUG_WARN("STATE", "未知菜单选项: %d", selectedOption);
          break;
      }
      }
      break;

    case STATE_IDLE:
      // 返回主菜单
      DEBUG_INFO("STATE", "从空闲状态返回主菜单");
      changeSystemState(STATE_MAIN_MENU, "长按返回主菜单");
      displayManager.showMessage("主菜单", 1000);
      break;

    case STATE_PRACTICING:
    case STATE_PAUSED:
      // 停止练习并返回主菜单
      DEBUG_INFO("STATE", "停止练习");
      dataManager.stopSession();
      changeSystemState(STATE_MAIN_MENU, "长按停止练习");
      inputManager.playStopSound();
      displayManager.showMessage("练习结束", 1000);
      break;

    case STATE_MENU:
      // 退出旧菜单，返回主菜单
      DEBUG_INFO("STATE", "退出旧菜单");
      changeSystemState(STATE_MAIN_MENU, "长按退出旧菜单");
      displayManager.showMessage("返回主菜单", 1000);
      break;

    case STATE_CALIBRATING:
      // 取消校准并返回主菜单
      DEBUG_INFO("STATE", "取消校准");
      sensorManager.finishCalibration();  // 确保校准状态被清理
      changeSystemState(STATE_MAIN_MENU, "长按取消校准");
      displayManager.showMessage("校准取消", 1000);
      break;

    case STATE_SETTINGS:
    case STATE_HISTORY:
      // 从设置或历史页面返回主菜单
      DEBUG_INFO("STATE", "返回主菜单");
      changeSystemState(STATE_MAIN_MENU, "长按返回主菜单");
      displayManager.showMessage("返回主菜单", 1000);
      break;

    default:
      DEBUG_WARN("STATE", "未知状态: %d", currentState);
      break;
  }
}

void handleDoubleClick() {
  DEBUG_INFO("INPUT", "双击按钮，当前状态: %d", currentState);

  switch (currentState) {
    case STATE_BOOT_ANIMATION:
      // 开机动画期间忽略按钮
      DEBUG_INFO("STATE", "开机动画中，忽略双击");
      break;

    case STATE_MAIN_MENU:
      // 在主菜单中双击快速进入练习模式
      DEBUG_INFO("STATE", "主菜单双击快速开始练习");
      dataManager.startSession();
      changeSystemState(STATE_PRACTICING, "双击快速开始练习");
      inputManager.playStartSound();
      displayManager.showMessage("快速开始", 1000);
      break;

    case STATE_IDLE:
      // 快速开始校准
      DEBUG_INFO("STATE", "快速开始传感器校准");
      if (sensorManager.startCalibration()) {
        changeSystemState(STATE_CALIBRATING, "双击开始校准");
        displayManager.showMessage("开始校准", 1000);
      } else {
        DEBUG_ERROR("STATE", "校准启动失败");
        displayManager.showMessage("校准失败", 1000);
      }
      break;

    case STATE_PRACTICING:
    case STATE_PAUSED:
      // 在练习状态下双击显示详细信息
      DEBUG_INFO("STATE", "显示练习详细信息");
      displayManager.showMessage("练习详情", 1000);
      break;

    case STATE_MENU:
      // 在旧菜单中双击返回主菜单
      DEBUG_INFO("STATE", "旧菜单双击返回主菜单");
      changeSystemState(STATE_MAIN_MENU, "双击返回主菜单");
      displayManager.showMessage("返回主菜单", 500);
      break;

    default:
      DEBUG_INFO("STATE", "当前状态不支持双击: %d", currentState);
      displayManager.showMessage("操作无效", 500);
      break;
  }
}

void updatePower() {
  powerManager.updateBatteryStatus();

  // 检查电源事件
  if (powerManager.hasPowerEvent()) {
    String message = powerManager.getPowerEventMessage();
    displayManager.showWarning(message);

    // 如果电池严重不足，强制保存数据并休眠
    if (powerManager.isCriticalBattery()) {
      dataManager.forceSave();
      powerManager.forceSleep();
    }
  }

  // 检查是否应该休眠
  if (powerManager.shouldSleep() && currentState == STATE_IDLE) {
    DEBUG_PRINTLN("准备进入休眠模式");
    dataManager.forceSave();
    displayManager.showShutdownScreen();
    powerManager.enterSleepMode();
  }

  // 优化功耗
  powerManager.optimizePowerConsumption();
}

void saveData() {
  if (dataManager.needsSave()) {
    dataManager.saveData();
  }
}

void handleSystemState() {
  zenData.status.currentState = currentState;

  switch (currentState) {
    case STATE_BOOT_ANIMATION:
      handleBootAnimationState();
      break;
    case STATE_MAIN_MENU:
      handleMainMenuState();
      break;
    case STATE_IDLE:
      handleIdleState();
      break;
    case STATE_CALIBRATING:
      handleCalibratingState();
      break;
    case STATE_PRACTICING:
      handlePracticingState();
      break;
    case STATE_PAUSED:
      handlePausedState();
      break;
    case STATE_MENU:
      handleMenuState();
      break;
    case STATE_SETTINGS:
      handleSettingsState();
      break;
    case STATE_HISTORY:
      handleHistoryState();
      break;
    case STATE_SLEEP:
      handleSleepState();
      break;
  }
}

void handleIdleState() {
  // 空闲状态处理
  // 检查是否需要自动校准
  if (zenData.settings.calibrationEnabled && !sensorManager.isCalibrationComplete()) {
    // 可以提示用户进行校准
  }
}

void handleCalibratingState() {
  // 校准状态处理
  if (sensorManager.isCalibrationInProgress()) {
    // 更新校准进度
    // 这里可以显示校准进度
    displayManager.showCalibrationProgress(50); // 简化处理
  } else if (sensorManager.isCalibrationComplete()) {
    // 校准完成
    currentState = STATE_IDLE;
    displayManager.setPage(PAGE_MAIN);
    inputManager.playSuccessSound();
    displayManager.showMessage("校准完成!", 2000);
    DEBUG_PRINTLN("校准完成");
  }
}

void handlePracticingState() {
  // 练习状态处理
  zenData.currentSession.duration = dataManager.getSessionDuration();

  // 检查是否达到预设时长
  if (zenData.currentSession.duration >= zenData.settings.practiceTime) {
    // 练习时间到
    dataManager.stopSession();
    currentState = STATE_IDLE;
    displayManager.setPage(PAGE_MAIN);
    inputManager.playSuccessSound();
    displayManager.showMessage("练习完成!", 3000);
    DEBUG_PRINTLN("练习时间到，自动停止");
  }
}

void handlePausedState() {
  // 暂停状态处理
  // 可以显示暂停提示
}

void handleMenuState() {
  // 菜单状态处理
  // 可以处理菜单导航逻辑
}

void handleSettingsState() {
  // 设置状态处理
  // 可以处理设置修改逻辑
}

void handleSleepState() {
  // 休眠状态处理
  // 通常不会执行到这里，因为系统已经休眠
}

void printSystemInfo() {
  DEBUG_PRINTLN("\n=== 系统状态信息 ===");
  DEBUG_PRINTF("当前状态: %d\n", currentState);
  DEBUG_PRINTF("运行时间: %lu ms\n", millis());
  DEBUG_PRINTF("系统初始化: %s\n", systemInitialized ? "是" : "否");

  // 打印各模块信息
  sensorManager.printStabilityData();
  dataManager.printSessionInfo();
  powerManager.printPowerInfo();

  DEBUG_PRINTLN("==================\n");
}

// ==================== 状态管理函数 ====================

bool isValidStateTransition(SystemState from, SystemState to) {
  // 定义有效的状态转换
  switch (from) {
    case STATE_BOOT_ANIMATION:
      return (to == STATE_MAIN_MENU);

    case STATE_MAIN_MENU:
      return (to == STATE_PRACTICING || to == STATE_HISTORY || to == STATE_SETTINGS ||
              to == STATE_CALIBRATING || to == STATE_SLEEP);

    case STATE_IDLE:
      return (to == STATE_MAIN_MENU || to == STATE_PRACTICING || to == STATE_MENU ||
              to == STATE_CALIBRATING || to == STATE_SLEEP);

    case STATE_PRACTICING:
      return (to == STATE_PAUSED || to == STATE_MAIN_MENU || to == STATE_IDLE);

    case STATE_PAUSED:
      return (to == STATE_PRACTICING || to == STATE_MAIN_MENU || to == STATE_IDLE);

    case STATE_MENU:
      return (to == STATE_MAIN_MENU || to == STATE_IDLE || to == STATE_SETTINGS);

    case STATE_CALIBRATING:
      return (to == STATE_MAIN_MENU || to == STATE_IDLE);

    case STATE_SETTINGS:
      return (to == STATE_MAIN_MENU || to == STATE_MENU || to == STATE_IDLE);

    case STATE_HISTORY:
      return (to == STATE_MAIN_MENU || to == STATE_IDLE);

    case STATE_SLEEP:
      return (to == STATE_MAIN_MENU || to == STATE_IDLE);

    default:
      return false;
  }
}

void changeSystemState(SystemState newState, const char* reason) {
  SystemState oldState = currentState;

  // 验证状态转换是否有效
  if (!isValidStateTransition(oldState, newState)) {
    DEBUG_ERROR("STATE", "无效的状态转换: %d -> %d (%s)", oldState, newState, reason);
    return;
  }

  // 执行状态转换
  currentState = newState;

  // 记录状态转换
  DEBUG_INFO("STATE", "状态转换: %d -> %d (%s)", oldState, newState, reason);

  // 更新显示
  updateDisplayForState();

  // 验证系统一致性
  validateSystemConsistency();

  // 更新活动时间
  zenData.status.lastActivity = millis();
}

void updateDisplayForState() {
  // 根据当前状态更新显示页面
  switch (currentState) {
    case STATE_BOOT_ANIMATION:
      displayManager.setPage(PAGE_BOOT_ANIMATION);
      break;

    case STATE_MAIN_MENU:
      displayManager.setPage(PAGE_MAIN_MENU);
      break;

    case STATE_IDLE:
    case STATE_PRACTICING:
    case STATE_PAUSED:
      displayManager.setPage(PAGE_MAIN);
      break;

    case STATE_MENU:
      // 保持当前页面，允许用户浏览
      break;

    case STATE_CALIBRATING:
      displayManager.setPage(PAGE_CALIBRATION);
      break;

    case STATE_SETTINGS:
      displayManager.setPage(PAGE_SETTINGS);
      break;

    case STATE_HISTORY:
      displayManager.setPage(PAGE_HISTORY);
      break;

    case STATE_SLEEP:
      // 显示将被关闭
      break;

    default:
      DEBUG_WARN("STATE", "未知状态，使用默认页面: %d", currentState);
      displayManager.setPage(PAGE_MAIN_MENU);
      break;
  }

  DEBUG_DEBUG("STATE", "显示页面已更新为状态: %d", currentState);
}

void validateSystemConsistency() {
  // 验证系统状态一致性
  bool consistent = true;

  // 检查练习状态与数据管理器状态的一致性
  if (currentState == STATE_PRACTICING && !dataManager.isSessionActive()) {
    DEBUG_WARN("STATE", "状态不一致: 系统处于练习状态但会话未激活");
    consistent = false;
  }

  if (currentState != STATE_PRACTICING && dataManager.isSessionActive()) {
    DEBUG_WARN("STATE", "状态不一致: 会话激活但系统不在练习状态");
    consistent = false;
  }

  // 检查校准状态与传感器管理器状态的一致性
  if (currentState == STATE_CALIBRATING && !sensorManager.isCalibrationInProgress()) {
    DEBUG_WARN("STATE", "状态不一致: 系统处于校准状态但传感器未在校准");
    consistent = false;
  }

  if (currentState != STATE_CALIBRATING && sensorManager.isCalibrationInProgress()) {
    DEBUG_WARN("STATE", "状态不一致: 传感器在校准但系统不在校准状态");
    consistent = false;
  }

  if (consistent) {
    DEBUG_DEBUG("STATE", "系统状态一致性验证通过");
  } else {
    DEBUG_ERROR("STATE", "系统状态一致性验证失败");
  }
}

// ==================== 新增状态处理函数 ====================

void handleBootAnimationState() {
  // 检查开机动画是否完成
  if (displayManager.isBootAnimationComplete()) {
    DEBUG_INFO("STATE", "开机动画完成，切换到主菜单");
    changeSystemState(STATE_MAIN_MENU, "开机动画完成");
  }

  // 开机动画期间不处理其他逻辑
  DEBUG_DEBUG("STATE", "开机动画进行中...");
}

void handleMainMenuState() {
  // 主菜单状态处理
  // 检查是否有自动切换需求
  static unsigned long lastMenuActivity = 0;
  unsigned long currentTime = millis();

  // 更新最后活动时间（如果有按钮操作）
  if (inputManager.hasButtonEvent()) {
    lastMenuActivity = currentTime;
  }

  // 主菜单状态下的定期检查
  if (currentTime - lastMenuActivity > 300000) { // 5分钟无操作
    DEBUG_INFO("STATE", "主菜单长时间无操作，考虑进入休眠");
    // 可以在这里添加自动休眠逻辑
  }

  DEBUG_DEBUG("STATE", "主菜单状态正常运行");
}

void handleHistoryState() {
  // 历史数据状态处理
  // 更新历史数据显示
  zenData.todayStats = dataManager.getTodayStats();

  // 检查是否需要刷新历史数据
  static unsigned long lastHistoryUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastHistoryUpdate > 5000) { // 每5秒更新一次
    // 刷新历史统计数据
    lastHistoryUpdate = currentTime;
    DEBUG_DEBUG("STATE", "更新历史数据显示");
  }

  DEBUG_DEBUG("STATE", "历史数据状态正常运行");
}