/*
 * 气定神闲仪调试系统测试程序
 * 
 * 这个示例专门用于测试和演示调试系统的功能，
 * 特别是OLED显示屏问题的诊断功能。
 * 
 * 使用方法：
 * 1. 将此文件复制到src/main.cpp
 * 2. 编译并上传到ESP32-C3
 * 3. 使用 pio device monitor 查看调试输出
 * 4. 长按按钮3秒触发硬件自检
 */

#include <Arduino.h>
#include "config.h"
#include "diagnostic_utils.h"
#include "display_manager.h"
#include "sensor_manager.h"
#include "input_manager.h"

// 测试对象
DisplayManager testDisplay;
SensorManager testSensor;
InputManager testInput;

// 测试状态
bool hardwareTestRequested = false;
unsigned long lastDiagnosticReport = 0;
const unsigned long diagnosticReportInterval = 5000; // 5秒

void setup() {
  // 初始化调试系统
  DEBUG_INIT();
  
  DEBUG_INFO("TEST", "=================================");
  DEBUG_INFO("TEST", "气定神闲仪调试系统测试程序");
  DEBUG_INFO("TEST", "版本: %s", PROJECT_VERSION);
  DEBUG_INFO("TEST", "=================================");
  
  // 初始化诊断系统
  DiagnosticUtils::initialize();
  
  // 检查启动时按钮状态 (按下时HIGH)
  pinMode(BUTTON_PIN, BUTTON_PIN_MODE);
  delay(50); // 等待引脚稳定
  if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_STATE) {
    DEBUG_INFO("TEST", "检测到启动时按钮按下 (HIGH)，将在3秒后执行硬件自检");
    delay(3000);
    if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_STATE) {
      hardwareTestRequested = true;
      DEBUG_INFO("TEST", "确认执行硬件自检");
    } else {
      DEBUG_INFO("TEST", "按钮已释放，取消硬件自检");
    }
  } else {
    DEBUG_DEBUG("TEST", "按钮未按下 (LOW)，正常启动测试");
  }
  
  DEBUG_INFO("TEST", "开始组件测试...");
  
  // 测试1: I2C总线扫描
  DEBUG_INFO("TEST", "=== 测试1: I2C总线扫描 ===");
  DiagnosticUtils::scanI2CBus();
  
  // 测试2: OLED专项诊断
  DEBUG_INFO("TEST", "=== 测试2: OLED专项诊断 ===");
  bool oledOK = DiagnosticUtils::diagnoseOLED();
  if (oledOK) {
    DEBUG_INFO("TEST", "✓ OLED诊断通过");
  } else {
    DEBUG_ERROR("TEST", "✗ OLED诊断失败");
    DiagnosticUtils::printOLEDDiagnostics();
  }
  
  // 测试3: 尝试初始化显示管理器
  DEBUG_INFO("TEST", "=== 测试3: 显示管理器初始化 ===");
  PERFORMANCE_START("DISPLAY_INIT_TEST");
  bool displayOK = testDisplay.initialize();
  PERFORMANCE_END("DISPLAY_INIT_TEST");
  
  if (displayOK) {
    DEBUG_INFO("TEST", "✓ 显示管理器初始化成功");
    testDisplay.showMessage("调试测试程序", 2000);
    testDisplay.showMessage("OLED工作正常", 2000);
  } else {
    DEBUG_ERROR("TEST", "✗ 显示管理器初始化失败");
    
    // 详细的故障排除
    DEBUG_INFO("TEST", "开始详细故障排除...");
    
    // 测试不同的I2C时钟速度
    DEBUG_INFO("TEST", "测试不同I2C时钟速度...");
    DiagnosticUtils::testI2CWithDifferentSpeeds(OLED_ADDRESS);
    
    // 检查电源状态
    DEBUG_INFO("TEST", "检查电源状态...");
    DiagnosticUtils::checkPowerSupply();
    
    // 检查GPIO引脚
    DEBUG_INFO("TEST", "检查GPIO引脚...");
    DiagnosticUtils::testAllConfiguredPins();
  }
  
  // 测试4: 传感器测试
  DEBUG_INFO("TEST", "=== 测试4: 传感器测试 ===");
  PERFORMANCE_START("SENSOR_INIT_TEST");
  bool sensorOK = testSensor.initialize();
  PERFORMANCE_END("SENSOR_INIT_TEST");
  
  if (sensorOK) {
    DEBUG_INFO("TEST", "✓ 传感器初始化成功");
  } else {
    DEBUG_ERROR("TEST", "✗ 传感器初始化失败");
    DiagnosticUtils::testI2CDevice(MPU6050_ADDRESS);
  }
  
  // 测试5: 输入管理器测试
  DEBUG_INFO("TEST", "=== 测试5: 输入管理器测试 ===");
  bool inputOK = testInput.initialize();
  if (inputOK) {
    DEBUG_INFO("TEST", "✓ 输入管理器初始化成功");
    testInput.playStartSound();
  } else {
    DEBUG_ERROR("TEST", "✗ 输入管理器初始化失败");
  }
  
  // 如果请求了硬件自检
  if (hardwareTestRequested) {
    DEBUG_INFO("TEST", "=== 执行完整硬件自检 ===");
    DiagnosticUtils::performHardwareSelfTest();
  }
  
  // 打印性能统计
  DiagnosticUtils::printPerformanceStats();
  
  // 打印最终状态
  DEBUG_INFO("TEST", "=== 测试完成 ===");
  DEBUG_INFO("TEST", "OLED状态: %s", g_diagnosticStatus.oledWorking ? "正常" : "异常");
  DEBUG_INFO("TEST", "传感器状态: %s", g_diagnosticStatus.sensorWorking ? "正常" : "异常");
  DEBUG_INFO("TEST", "I2C状态: %s", g_diagnosticStatus.i2cWorking ? "正常" : "异常");
  DEBUG_INFO("TEST", "电源状态: %s", g_diagnosticStatus.powerOK ? "正常" : "异常");
  DEBUG_INFO("TEST", "错误计数: %d", g_diagnosticStatus.errorCount);
  DEBUG_INFO("TEST", "警告计数: %d", g_diagnosticStatus.warningCount);
  
  DEBUG_MEMORY();
  
  DEBUG_INFO("TEST", "进入主循环，按按钮进行交互测试...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // 更新输入管理器
  testInput.update();
  
  // 检查按钮事件
  if (testInput.hasButtonEvent()) {
    ButtonEvent event = testInput.getButtonEvent();
    
    switch (event.state) {
      case BUTTON_PRESSED:
        DEBUG_INFO("TEST", "检测到单击按钮");
        if (testDisplay.isReady()) {
          testDisplay.showMessage("按钮单击", 1000);
        }
        testInput.playTone(1000, 200);
        
        // 执行快速I2C扫描
        DiagnosticUtils::scanI2CBus();
        break;
        
      case BUTTON_LONG_PRESSED:
        DEBUG_INFO("TEST", "检测到长按按钮");
        if (testDisplay.isReady()) {
          testDisplay.showMessage("按钮长按", 1000);
        }
        testInput.playTone(1500, 300);
        
        // 执行OLED诊断
        DiagnosticUtils::diagnoseOLED();
        DiagnosticUtils::printOLEDDiagnostics();
        break;
        
      case BUTTON_DOUBLE_PRESSED:
        DEBUG_INFO("TEST", "检测到双击按钮");
        if (testDisplay.isReady()) {
          testDisplay.showMessage("按钮双击", 1000);
        }
        testInput.playSuccessSound();
        
        // 执行完整硬件自检
        DiagnosticUtils::performHardwareSelfTest();
        break;
        
      default:
        break;
    }
    
    testInput.clearButtonEvent();
  }
  
  // 定期诊断报告
  if (currentTime - lastDiagnosticReport >= diagnosticReportInterval) {
    DEBUG_INFO("TEST", "=== 定期诊断报告 ===");
    
    // 测试传感器读取
    if (g_diagnosticStatus.sensorWorking) {
      if (testSensor.readSensorData()) {
        SensorData data = testSensor.getRawData();
        StabilityData stability = testSensor.getStabilityData();
        DEBUG_INFO("TEST", "传感器数据: 加速度(%.2f,%.2f,%.2f) 稳定性:%.1f", 
                   data.accelX, data.accelY, data.accelZ, stability.score);
      }
    }
    
    // 检查电源状态
    DiagnosticUtils::printPowerStatus();
    
    // 内存使用情况
    DEBUG_MEMORY();
    
    // 系统运行时间
    DEBUG_INFO("TEST", "系统运行时间: %s", 
               DiagnosticUtils::formatUptime(millis()).c_str());
    
    lastDiagnosticReport = currentTime;
  }
  
  // 定期系统报告
  DiagnosticUtils::periodicSystemReport();
  
  delay(100);
}

// 演示如何使用调试宏
void demonstrateDebugMacros() {
  DEBUG_INFO("DEMO", "演示不同级别的调试输出");
  
  DEBUG_ERROR("DEMO", "这是一个错误消息: %d", 404);
  DEBUG_WARN("DEMO", "这是一个警告消息: %.2f", 3.14);
  DEBUG_INFO("DEMO", "这是一个信息消息: %s", "Hello");
  DEBUG_DEBUG("DEMO", "这是一个调试消息");
  DEBUG_VERBOSE("DEMO", "这是一个详细消息");
  
  // 性能测量演示
  PERFORMANCE_START("demo_function");
  delay(100); // 模拟一些工作
  PERFORMANCE_END("demo_function");
  
  // 内存监控演示
  DEBUG_MEMORY();
  
  // 系统信息演示
  DEBUG_SYSTEM_INFO();
}

// 演示错误报告
void demonstrateErrorReporting() {
  DiagnosticUtils::reportError("DEMO", "这是一个演示错误");
  DiagnosticUtils::reportWarning("DEMO", "这是一个演示警告");
  
  // 使用便捷宏
  DIAGNOSTIC_ERROR("DEMO", "使用宏报告错误: %d", 500);
  DIAGNOSTIC_WARNING("DEMO", "使用宏报告警告: %s", "测试");
}
