/*
 * 气定神闲仪基本演示程序
 * 
 * 这个示例展示了设备的基本功能：
 * 1. 传感器数据读取
 * 2. 稳定性评分计算
 * 3. OLED显示
 * 4. 按钮交互
 * 5. 蜂鸣器反馈
 * 
 * 使用方法：
 * - 将此文件复制到src/main.cpp来运行演示
 * - 或者在现有main.cpp中参考相关代码
 */

#include <Arduino.h>
#include "config.h"
#include "data_types.h"
#include "sensor_manager.h"
#include "display_manager.h"
#include "input_manager.h"

// 全局对象
SensorManager sensor;
DisplayManager display;
InputManager input;

// 演示数据
ZenMotionData demoData;
unsigned long lastUpdate = 0;
unsigned long demoStartTime = 0;
bool demoRunning = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=================================");
  Serial.println("气定神闲仪基本演示程序");
  Serial.println("=================================");
  
  // 初始化各个模块
  Serial.println("初始化传感器...");
  if (!sensor.initialize()) {
    Serial.println("传感器初始化失败!");
    return;
  }
  
  Serial.println("初始化显示屏...");
  if (!display.initialize()) {
    Serial.println("显示屏初始化失败!");
    return;
  }
  
  Serial.println("初始化输入管理器...");
  if (!input.initialize()) {
    Serial.println("输入管理器初始化失败!");
    return;
  }
  
  // 初始化演示数据
  demoData.settings.stabilityThreshold = STABILITY_THRESHOLD;
  demoData.settings.soundEnabled = true;
  demoData.status.batteryVoltage = 3.7;
  demoData.status.isCharging = false;
  demoData.status.lowBattery = false;
  
  // 显示欢迎信息
  display.showMessage("欢迎使用气定神闲仪", 2000);
  display.showMessage("按按钮开始演示", 2000);
  
  Serial.println("初始化完成!");
  Serial.println("按钮操作说明:");
  Serial.println("- 单击: 开始/停止演示");
  Serial.println("- 双击: 校准传感器");
  Serial.println("- 长按: 显示统计信息");
}

void loop() {
  unsigned long currentTime = millis();
  
  // 更新输入
  input.update();
  handleInput();
  
  // 读取传感器数据
  if (sensor.readSensorData()) {
    demoData.sensor = sensor.getRawData();
    demoData.stability = sensor.getStabilityData();
  }
  
  // 更新演示数据
  updateDemoData();
  
  // 更新显示 (每100ms)
  if (currentTime - lastUpdate >= 100) {
    display.update(demoData);
    lastUpdate = currentTime;
  }
  
  // 检查破定提醒
  if (demoRunning && !demoData.stability.isStable) {
    if (demoData.settings.soundEnabled) {
      input.playBreakWarning();
    }
  }
  
  // 打印调试信息 (每秒)
  static unsigned long lastDebugPrint = 0;
  if (currentTime - lastDebugPrint >= 1000) {
    printDebugInfo();
    lastDebugPrint = currentTime;
  }
  
  delay(10);
}

void handleInput() {
  if (input.hasButtonEvent()) {
    ButtonEvent event = input.getButtonEvent();
    
    switch (event.state) {
      case BUTTON_PRESSED:
        handleSingleClick();
        break;
      case BUTTON_LONG_PRESSED:
        handleLongPress();
        break;
      case BUTTON_DOUBLE_PRESSED:
        handleDoubleClick();
        break;
      default:
        break;
    }
    
    input.clearButtonEvent();
  }
}

void handleSingleClick() {
  Serial.println("单击按钮");
  
  if (!demoRunning) {
    // 开始演示
    demoRunning = true;
    demoStartTime = millis();
    demoData.currentSession.startTime = demoStartTime;
    demoData.currentSession.duration = 0;
    demoData.currentSession.breakCount = 0;
    
    input.playStartSound();
    display.showMessage("开始练习演示", 1000);
    Serial.println("演示开始");
  } else {
    // 停止演示
    demoRunning = false;
    demoData.currentSession.duration = millis() - demoStartTime;
    
    input.playStopSound();
    display.showMessage("演示结束", 1000);
    Serial.println("演示结束");
    
    // 显示统计信息
    showSessionStats();
  }
}

void handleLongPress() {
  Serial.println("长按按钮");
  
  // 显示详细统计信息
  showDetailedStats();
}

void handleDoubleClick() {
  Serial.println("双击按钮");
  
  // 开始校准
  if (sensor.startCalibration()) {
    display.showMessage("开始校准...", 1000);
    Serial.println("开始传感器校准");
    
    // 校准过程
    for (int i = 0; i < 100; i++) {
      sensor.updateCalibration();
      display.showCalibrationProgress(i);
      delay(50);
    }
    
    if (sensor.finishCalibration()) {
      input.playSuccessSound();
      display.showMessage("校准完成!", 2000);
      Serial.println("校准完成");
    } else {
      input.playErrorSound();
      display.showMessage("校准失败", 2000);
      Serial.println("校准失败");
    }
  }
}

void updateDemoData() {
  if (demoRunning) {
    // 更新会话时长
    demoData.currentSession.duration = millis() - demoStartTime;
    
    // 更新破定计数
    if (!demoData.stability.isStable) {
      static unsigned long lastBreakTime = 0;
      if (millis() - lastBreakTime > 2000) {
        demoData.currentSession.breakCount++;
        lastBreakTime = millis();
      }
    }
    
    // 更新平均稳定性
    static float totalScore = 0;
    static int sampleCount = 0;
    totalScore += demoData.stability.score;
    sampleCount++;
    demoData.currentSession.avgStability = totalScore / sampleCount;
    
    // 更新最高稳定性
    if (demoData.stability.score > demoData.currentSession.maxStability) {
      demoData.currentSession.maxStability = demoData.stability.score;
    }
  }
  
  // 更新今日统计 (演示用)
  demoData.todayStats.totalTime = demoData.currentSession.duration;
  demoData.todayStats.sessionCount = demoRunning ? 1 : 0;
  demoData.todayStats.avgStability = demoData.currentSession.avgStability;
  demoData.todayStats.bestStability = demoData.currentSession.maxStability;
  demoData.todayStats.totalBreaks = demoData.currentSession.breakCount;
}

void showSessionStats() {
  Serial.println("\n=== 会话统计 ===");
  Serial.printf("持续时间: %lu 秒\n", demoData.currentSession.duration / 1000);
  Serial.printf("平均稳定性: %.1f\n", demoData.currentSession.avgStability);
  Serial.printf("最高稳定性: %.1f\n", demoData.currentSession.maxStability);
  Serial.printf("破定次数: %d\n", demoData.currentSession.breakCount);
  Serial.println("================\n");
}

void showDetailedStats() {
  display.showMessage("详细统计信息", 1000);
  
  String stats = "时长:" + String(demoData.currentSession.duration / 1000) + "s";
  display.showMessage(stats, 2000);
  
  stats = "评分:" + String((int)demoData.currentSession.avgStability);
  display.showMessage(stats, 2000);
  
  stats = "破定:" + String(demoData.currentSession.breakCount) + "次";
  display.showMessage(stats, 2000);
}

void printDebugInfo() {
  if (!demoRunning) return;
  
  Serial.printf("稳定性: %.1f | 平均: %.1f | 破定: %d | 时长: %lu秒\n",
                demoData.stability.score,
                demoData.currentSession.avgStability,
                demoData.currentSession.breakCount,
                demoData.currentSession.duration / 1000);
}
