/*
 * 按钮配置测试程序
 * 
 * 专门用于验证新的按钮配置：
 * - 按下时HIGH电平
 * - 松开时LOW电平
 * - INPUT引脚模式 (无内部电阻)
 * 
 * 使用方法：
 * 1. 将此文件复制到src/main.cpp
 * 2. 编译并上传到ESP32-C3
 * 3. 使用 pio device monitor 查看测试结果
 * 4. 测试各种按钮操作
 */

#include <Arduino.h>
#include "config.h"
#include "input_manager.h"
#include "diagnostic_utils.h"

// 测试对象
InputManager testInput;

// 测试状态
unsigned long lastStatusReport = 0;
const unsigned long statusReportInterval = 2000; // 2秒
unsigned long testStartTime = 0;
int buttonPressCount = 0;
int singleClickCount = 0;
int longPressCount = 0;
int doubleClickCount = 0;

void setup() {
  // 初始化调试系统
  DEBUG_INIT();
  
  DEBUG_INFO("BUTTON_TEST", "=================================");
  DEBUG_INFO("BUTTON_TEST", "按钮配置测试程序");
  DEBUG_INFO("BUTTON_TEST", "=================================");
  
  // 初始化诊断系统
  DiagnosticUtils::initialize();
  
  // 显示按钮配置信息
  DEBUG_INFO("BUTTON_TEST", "按钮配置验证:");
  DEBUG_INFO("BUTTON_TEST", "- 引脚: GPIO%d", BUTTON_PIN);
  DEBUG_INFO("BUTTON_TEST", "- 按下状态: %s", BUTTON_PRESSED_STATE ? "HIGH" : "LOW");
  DEBUG_INFO("BUTTON_TEST", "- 松开状态: %s", BUTTON_RELEASED_STATE ? "HIGH" : "LOW");
  DEBUG_INFO("BUTTON_TEST", "- 引脚模式: %s", "INPUT");
  
  // 初始化输入管理器
  DEBUG_INFO("BUTTON_TEST", "初始化输入管理器...");
  if (!testInput.initialize()) {
    DEBUG_ERROR("BUTTON_TEST", "输入管理器初始化失败!");
    return;
  }
  
  // 执行GPIO引脚测试
  DEBUG_INFO("BUTTON_TEST", "执行GPIO引脚测试...");
  DiagnosticUtils::testGPIOPin(BUTTON_PIN, "BUTTON");
  
  // 显示初始按钮状态
  DEBUG_INFO("BUTTON_TEST", "初始按钮状态检查:");
  bool currentState = digitalRead(BUTTON_PIN);
  DEBUG_INFO("BUTTON_TEST", "- GPIO读取值: %s", currentState ? "HIGH" : "LOW");
  DEBUG_INFO("BUTTON_TEST", "- 解释为: %s", 
             currentState == BUTTON_PRESSED_STATE ? "按下" : "松开");
  DEBUG_INFO("BUTTON_TEST", "- isButtonPressed(): %s", 
             testInput.isButtonPressed() ? "true" : "false");
  DEBUG_INFO("BUTTON_TEST", "- isButtonReleased(): %s", 
             testInput.isButtonReleased() ? "true" : "false");
  
  // 播放启动音
  testInput.playStartSound();
  
  testStartTime = millis();
  
  DEBUG_INFO("BUTTON_TEST", "=================================");
  DEBUG_INFO("BUTTON_TEST", "开始按钮测试...");
  DEBUG_INFO("BUTTON_TEST", "请进行以下测试:");
  DEBUG_INFO("BUTTON_TEST", "1. 单击按钮 (快速按下并释放)");
  DEBUG_INFO("BUTTON_TEST", "2. 长按按钮 (按住超过1秒)");
  DEBUG_INFO("BUTTON_TEST", "3. 双击按钮 (快速两次单击)");
  DEBUG_INFO("BUTTON_TEST", "=================================");
}

void loop() {
  unsigned long currentTime = millis();
  
  // 更新输入管理器
  testInput.update();
  
  // 检查按钮事件
  if (testInput.hasButtonEvent()) {
    ButtonEvent event = testInput.getButtonEvent();
    buttonPressCount++;
    
    DEBUG_INFO("BUTTON_TEST", "=== 按钮事件 #%d ===", buttonPressCount);
    DEBUG_INFO("BUTTON_TEST", "事件类型: %s", 
               (event.state == BUTTON_PRESSED) ? "单击" :
               (event.state == BUTTON_LONG_PRESSED) ? "长按" :
               (event.state == BUTTON_DOUBLE_PRESSED) ? "双击" : "未知");
    DEBUG_INFO("BUTTON_TEST", "按下时间: %lu ms", event.pressTime);
    DEBUG_INFO("BUTTON_TEST", "释放时间: %lu ms", event.releaseTime);
    DEBUG_INFO("BUTTON_TEST", "持续时间: %lu ms", event.duration);
    
    // 统计不同类型的事件
    switch (event.state) {
      case BUTTON_PRESSED:
        singleClickCount++;
        DEBUG_INFO("BUTTON_TEST", "✓ 单击检测成功!");
        testInput.playTone(1000, 200);
        break;
        
      case BUTTON_LONG_PRESSED:
        longPressCount++;
        DEBUG_INFO("BUTTON_TEST", "✓ 长按检测成功!");
        testInput.playTone(1500, 300);
        break;
        
      case BUTTON_DOUBLE_PRESSED:
        doubleClickCount++;
        DEBUG_INFO("BUTTON_TEST", "✓ 双击检测成功!");
        testInput.playSuccessSound();
        break;
        
      default:
        DEBUG_WARN("BUTTON_TEST", "未知按钮事件类型: %d", event.state);
        break;
    }
    
    // 显示详细的按钮信息
    testInput.printButtonInfo();
    
    testInput.clearButtonEvent();
  }
  
  // 定期状态报告
  if (currentTime - lastStatusReport >= statusReportInterval) {
    DEBUG_INFO("BUTTON_TEST", "=== 定期状态报告 ===");
    DEBUG_INFO("BUTTON_TEST", "运行时间: %s", 
               DiagnosticUtils::formatUptime(currentTime - testStartTime).c_str());
    
    // 当前按钮状态
    bool currentState = digitalRead(BUTTON_PIN);
    DEBUG_INFO("BUTTON_TEST", "当前按钮状态: %s (%s)", 
               currentState ? "HIGH" : "LOW",
               currentState == BUTTON_PRESSED_STATE ? "按下" : "松开");
    
    // 事件统计
    DEBUG_INFO("BUTTON_TEST", "事件统计:");
    DEBUG_INFO("BUTTON_TEST", "- 总按钮事件: %d", buttonPressCount);
    DEBUG_INFO("BUTTON_TEST", "- 单击次数: %d", singleClickCount);
    DEBUG_INFO("BUTTON_TEST", "- 长按次数: %d", longPressCount);
    DEBUG_INFO("BUTTON_TEST", "- 双击次数: %d", doubleClickCount);
    
    // 内存使用情况
    DEBUG_MEMORY();
    
    lastStatusReport = currentTime;
  }
  
  delay(10);
}

// 演示按钮状态实时监控
void demonstrateRealTimeMonitoring() {
  DEBUG_INFO("BUTTON_TEST", "=== 实时按钮状态监控 ===");
  DEBUG_INFO("BUTTON_TEST", "监控10秒，请操作按钮...");
  
  unsigned long startTime = millis();
  bool lastState = digitalRead(BUTTON_PIN);
  
  while (millis() - startTime < 10000) { // 监控10秒
    bool currentState = digitalRead(BUTTON_PIN);
    
    if (currentState != lastState) {
      DEBUG_INFO("BUTTON_TEST", "状态变化: %s -> %s (%s -> %s)",
                 lastState ? "HIGH" : "LOW",
                 currentState ? "HIGH" : "LOW",
                 lastState == BUTTON_PRESSED_STATE ? "按下" : "松开",
                 currentState == BUTTON_PRESSED_STATE ? "按下" : "松开");
      lastState = currentState;
    }
    
    delay(10);
  }
  
  DEBUG_INFO("BUTTON_TEST", "实时监控结束");
}

// 验证按钮配置的正确性
void verifyButtonConfiguration() {
  DEBUG_INFO("BUTTON_TEST", "=== 按钮配置验证 ===");
  
  // 检查配置常量
  DEBUG_INFO("BUTTON_TEST", "配置常量检查:");
  DEBUG_INFO("BUTTON_TEST", "BUTTON_PRESSED_STATE = %s", 
             BUTTON_PRESSED_STATE ? "HIGH" : "LOW");
  DEBUG_INFO("BUTTON_TEST", "BUTTON_RELEASED_STATE = %s", 
             BUTTON_RELEASED_STATE ? "HIGH" : "LOW");
  
  // 验证逻辑一致性
  if (BUTTON_PRESSED_STATE == HIGH && BUTTON_RELEASED_STATE == LOW) {
    DEBUG_INFO("BUTTON_TEST", "✓ 配置逻辑正确: 按下HIGH, 松开LOW");
  } else if (BUTTON_PRESSED_STATE == LOW && BUTTON_RELEASED_STATE == HIGH) {
    DEBUG_INFO("BUTTON_TEST", "✓ 配置逻辑正确: 按下LOW, 松开HIGH");
  } else {
    DEBUG_ERROR("BUTTON_TEST", "✗ 配置逻辑错误: 按下和松开状态相同!");
  }
  
  // 测试引脚读取
  pinMode(BUTTON_PIN, BUTTON_PIN_MODE);
  delay(100);
  
  bool state1 = digitalRead(BUTTON_PIN);
  delay(100);
  bool state2 = digitalRead(BUTTON_PIN);
  
  if (state1 == state2) {
    DEBUG_INFO("BUTTON_TEST", "✓ 引脚读取稳定: %s", state1 ? "HIGH" : "LOW");
  } else {
    DEBUG_WARN("BUTTON_TEST", "⚠ 引脚读取不稳定: %s -> %s", 
               state1 ? "HIGH" : "LOW", state2 ? "HIGH" : "LOW");
  }
}
