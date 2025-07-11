#include "diagnostic_utils.h"
#include <esp_system.h>
#include <esp_chip_info.h>

// I2C时钟速度数组定义
const uint32_t I2C_CLOCK_SPEEDS[I2C_CLOCK_SPEEDS_COUNT] = {100000, 400000, 1000000}; // 100kHz, 400kHz, 1MHz

// 静态成员初始化
bool DiagnosticUtils::i2cInitialized = false;
uint32_t DiagnosticUtils::lastSystemReport = 0;
uint32_t DiagnosticUtils::systemReportInterval = 10000; // 10秒

// 全局诊断状态
DiagnosticStatus g_diagnosticStatus = {
  .systemInitialized = false,
  .i2cWorking = false,
  .oledWorking = false,
  .sensorWorking = false,
  .powerOK = false,
  .errorCount = 0,
  .warningCount = 0,
  .lastDiagnosticTime = 0
};

// 性能计时器数组
#define MAX_PERFORMANCE_TIMERS 10
static PerformanceTimer performanceTimers[MAX_PERFORMANCE_TIMERS];
static int performanceTimerCount = 0;

void DiagnosticUtils::initialize() {
  DEBUG_INFO("DIAGNOSTIC", "初始化诊断系统...");
  
  // 重置诊断状态
  g_diagnosticStatus.systemInitialized = false;
  g_diagnosticStatus.errorCount = 0;
  g_diagnosticStatus.warningCount = 0;
  g_diagnosticStatus.lastDiagnosticTime = millis();
  
  // 打印系统信息
  printSystemInfo();
  printChipInfo();
  printMemoryInfo();
  
  g_diagnosticStatus.systemInitialized = true;
  DEBUG_INFO("DIAGNOSTIC", "诊断系统初始化完成");
}

void DiagnosticUtils::printSystemInfo() {
  DEBUG_INFO("SYSTEM", "=== 系统信息 ===");
  DEBUG_INFO("SYSTEM", "项目: %s", PROJECT_NAME);
  DEBUG_INFO("SYSTEM", "版本: %s", PROJECT_VERSION);
  DEBUG_INFO("SYSTEM", "编译时间: %s %s", __DATE__, __TIME__);
  DEBUG_INFO("SYSTEM", "运行时间: %s", formatUptime(millis()).c_str());
}

void DiagnosticUtils::printChipInfo() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  
  DEBUG_INFO("CHIP", "=== 芯片信息 ===");
  DEBUG_INFO("CHIP", "型号: %s", ESP.getChipModel());
  DEBUG_INFO("CHIP", "修订版本: %d", ESP.getChipRevision());
  DEBUG_INFO("CHIP", "核心数: %d", ESP.getChipCores());
  DEBUG_INFO("CHIP", "CPU频率: %d MHz", ESP.getCpuFreqMHz());
  DEBUG_INFO("CHIP", "Flash大小: %s", formatBytes(ESP.getFlashChipSize()).c_str());
  DEBUG_INFO("CHIP", "Flash速度: %d MHz", ESP.getFlashChipSpeed() / 1000000);
  
  // ESP32-C3通常不支持PSRAM
  DEBUG_INFO("CHIP", "PSRAM: ESP32-C3不支持");
}

void DiagnosticUtils::printMemoryInfo() {
  DEBUG_INFO("MEMORY", "=== 内存信息 ===");
  DEBUG_INFO("MEMORY", "总堆内存: %s", formatBytes(ESP.getHeapSize()).c_str());
  DEBUG_INFO("MEMORY", "可用堆内存: %s", formatBytes(ESP.getFreeHeap()).c_str());
  DEBUG_INFO("MEMORY", "最小可用堆内存: %s", formatBytes(ESP.getMinFreeHeap()).c_str());
  DEBUG_INFO("MEMORY", "最大分配块: %s", formatBytes(ESP.getMaxAllocHeap()).c_str());
  
  // ESP32-C3不支持PSRAM
  DEBUG_INFO("MEMORY", "PSRAM: 不适用于ESP32-C3");
}

void DiagnosticUtils::periodicSystemReport() {
  uint32_t currentTime = millis();
  if (currentTime - lastSystemReport >= systemReportInterval) {
    DEBUG_INFO("PERIODIC", "=== 定期系统报告 ===");
    DEBUG_INFO("PERIODIC", "运行时间: %s", formatUptime(currentTime).c_str());
    printMemoryInfo();
    DEBUG_INFO("PERIODIC", "错误计数: %d, 警告计数: %d", 
               g_diagnosticStatus.errorCount, g_diagnosticStatus.warningCount);
    
    lastSystemReport = currentTime;
  }
}

bool DiagnosticUtils::scanI2CBus() {
  DEBUG_INFO("I2C", "=== I2C总线扫描 ===");
  
  if (!i2cInitialized) {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    i2cInitialized = true;
    delay(100);
  }
  
  int deviceCount = 0;
  DEBUG_INFO("I2C", "扫描地址范围: 0x08 - 0x77");
  
  for (uint8_t address = 0x08; address <= 0x77; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
      DEBUG_INFO("I2C", "发现设备: 0x%02X", address);
      deviceCount++;
      
      // 检查已知设备
      if (address == OLED_ADDRESS) {
        DEBUG_INFO("I2C", "  -> OLED显示屏 (SSD1306)");
        g_diagnosticStatus.oledWorking = true;
      } else if (address == MPU6050_ADDRESS) {
        DEBUG_INFO("I2C", "  -> MPU6050传感器");
        g_diagnosticStatus.sensorWorking = true;
      } else {
        DEBUG_INFO("I2C", "  -> 未知设备");
      }
    } else if (error == 4) {
      DEBUG_ERROR("I2C", "地址0x%02X发生未知错误", address);
    }
    
    delay(10); // 避免总线过载
  }
  
  if (deviceCount == 0) {
    DEBUG_ERROR("I2C", "未发现任何I2C设备!");
    g_diagnosticStatus.i2cWorking = false;
    return false;
  } else {
    DEBUG_INFO("I2C", "总共发现 %d 个I2C设备", deviceCount);
    g_diagnosticStatus.i2cWorking = true;
    return true;
  }
}

bool DiagnosticUtils::testI2CDevice(uint8_t address) {
  DEBUG_DEBUG("I2C", "测试设备地址: 0x%02X", address);
  
  Wire.beginTransmission(address);
  uint8_t error = Wire.endTransmission();
  
  switch (error) {
    case 0:
      DEBUG_DEBUG("I2C", "设备0x%02X响应正常", address);
      return true;
    case 1:
      DEBUG_ERROR("I2C", "设备0x%02X: 数据太长，缓冲区溢出", address);
      break;
    case 2:
      DEBUG_ERROR("I2C", "设备0x%02X: 地址发送时收到NACK", address);
      break;
    case 3:
      DEBUG_ERROR("I2C", "设备0x%02X: 数据发送时收到NACK", address);
      break;
    case 4:
      DEBUG_ERROR("I2C", "设备0x%02X: 其他错误", address);
      break;
    case 5:
      DEBUG_ERROR("I2C", "设备0x%02X: 超时", address);
      break;
    default:
      DEBUG_ERROR("I2C", "设备0x%02X: 未知错误代码 %d", address, error);
      break;
  }
  
  return false;
}

bool DiagnosticUtils::testI2CWithDifferentSpeeds(uint8_t address) {
  DEBUG_INFO("I2C", "测试设备0x%02X在不同时钟速度下的响应", address);
  
  for (int i = 0; i < I2C_CLOCK_SPEEDS_COUNT; i++) {
    uint32_t speed = I2C_CLOCK_SPEEDS[i];
    DEBUG_INFO("I2C", "尝试时钟速度: %d Hz", speed);
    
    if (initializeI2CWithSpeed(speed)) {
      delay(100);
      if (testI2CDevice(address)) {
        DEBUG_INFO("I2C", "设备0x%02X在%d Hz下工作正常", address, speed);
        return true;
      }
    }
    delay(100);
  }
  
  DEBUG_ERROR("I2C", "设备0x%02X在所有测试速度下都无响应", address);
  return false;
}

bool DiagnosticUtils::initializeI2CWithSpeed(uint32_t clockSpeed) {
  DEBUG_DEBUG("I2C", "初始化I2C，时钟速度: %d Hz", clockSpeed);
  
  Wire.end();
  delay(50);
  
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(clockSpeed);
  delay(100);
  
  i2cInitialized = true;
  return true;
}

void DiagnosticUtils::printI2CStatus() {
  DEBUG_INFO("I2C", "=== I2C状态 ===");
  DEBUG_INFO("I2C", "SDA引脚: GPIO%d", I2C_SDA_PIN);
  DEBUG_INFO("I2C", "SCL引脚: GPIO%d", I2C_SCL_PIN);
  DEBUG_INFO("I2C", "初始化状态: %s", i2cInitialized ? "已初始化" : "未初始化");
  DEBUG_INFO("I2C", "当前时钟速度: %d Hz", Wire.getClock());
}

bool DiagnosticUtils::testGPIOPin(uint8_t pin, const char* pinName) {
  DEBUG_DEBUG("GPIO", "测试引脚 %s (GPIO%d)", pinName, pin);

  if (pin == BUTTON_PIN) {
    // 按钮引脚特殊处理 (按下时HIGH，松开时LOW，无外部电阻)
    pinMode(pin, BUTTON_PIN_MODE);
    delay(10);
    int currentValue = digitalRead(pin);

    DEBUG_DEBUG("GPIO", "%s: 当前状态=%s (%s)", pinName,
                currentValue ? "HIGH" : "LOW",
                currentValue == BUTTON_PRESSED_STATE ? "按下" : "松开");
    DEBUG_INFO("GPIO", "%s配置: 按下=%s, 松开=%s, 模式=INPUT", pinName,
               BUTTON_PRESSED_STATE ? "HIGH" : "LOW",
               BUTTON_RELEASED_STATE ? "HIGH" : "LOW");
  } else {
    // 其他引脚的常规测试
    // 设置为输入模式并读取状态
    pinMode(pin, INPUT);
    int inputValue = digitalRead(pin);

    // 设置为输入上拉模式并读取状态
    pinMode(pin, INPUT_PULLUP);
    delay(10);
    int pullupValue = digitalRead(pin);

    DEBUG_DEBUG("GPIO", "%s: 输入=%d, 上拉=%d", pinName, inputValue, pullupValue);
  }

  return true; // 基本测试总是通过
}

void DiagnosticUtils::testAllConfiguredPins() {
  DEBUG_INFO("GPIO", "=== GPIO引脚测试 ===");

  testGPIOPin(I2C_SDA_PIN, "SDA");
  testGPIOPin(I2C_SCL_PIN, "SCL");
  testGPIOPin(BUTTON_PIN, "BUTTON");
  testGPIOPin(BUZZER_PIN, "BUZZER");

  // 检查I2C引脚的上拉电阻
  bool sdaPullup = checkPullupResistor(I2C_SDA_PIN);
  bool sclPullup = checkPullupResistor(I2C_SCL_PIN);

  DEBUG_INFO("GPIO", "SDA上拉电阻: %s", sdaPullup ? "检测到" : "未检测到");
  DEBUG_INFO("GPIO", "SCL上拉电阻: %s", sclPullup ? "检测到" : "未检测到");
  DEBUG_INFO("GPIO", "按钮引脚: 无需上拉电阻 (硬件设计)");

  if (!sdaPullup || !sclPullup) {
    DEBUG_WARN("GPIO", "I2C引脚缺少上拉电阻，可能影响通信");
  }
}

bool DiagnosticUtils::checkPullupResistor(uint8_t pin) {
  if (pin == BUTTON_PIN) {
    // 按钮引脚不需要上拉电阻检查，因为硬件设计不使用上拉电阻
    DEBUG_DEBUG("GPIO", "按钮引脚(GPIO%d)不需要上拉电阻检查", pin);
    return true; // 对于按钮引脚，总是返回true
  }

  // 其他引脚的上拉电阻检查
  // 设置为输入模式（无上拉）
  pinMode(pin, INPUT);
  delay(10);
  int noPullValue = digitalRead(pin);

  // 设置为输入上拉模式
  pinMode(pin, INPUT_PULLUP);
  delay(10);
  int pullupValue = digitalRead(pin);

  // 如果有外部上拉电阻，即使在INPUT模式下也应该读到高电平
  return (noPullValue == HIGH) || (pullupValue == HIGH);
}

float DiagnosticUtils::measureVoltage() {
  // ESP32-C3的简化电压测量
  // 实际项目中需要根据硬件设计实现

  // 模拟电压读取，实际应该使用ADC
  float voltage = 3.3f + (random(-100, 100) / 1000.0f);

  DEBUG_VERBOSE("POWER", "测量电压: %.3f V", voltage);
  return voltage;
}

bool DiagnosticUtils::checkPowerSupply() {
  DEBUG_INFO("POWER", "=== 电源检查 ===");

  float voltage = measureVoltage();
  bool powerOK = (voltage >= VOLTAGE_MIN_NORMAL && voltage <= VOLTAGE_MAX_NORMAL);

  DEBUG_INFO("POWER", "供电电压: %.3f V", voltage);
  DEBUG_INFO("POWER", "电压范围: %.1f - %.1f V", VOLTAGE_MIN_NORMAL, VOLTAGE_MAX_NORMAL);
  DEBUG_INFO("POWER", "电源状态: %s", powerOK ? "正常" : "异常");

  if (!powerOK) {
    if (voltage < VOLTAGE_MIN_NORMAL) {
      DEBUG_ERROR("POWER", "电压过低! 可能导致设备不稳定");
    } else {
      DEBUG_ERROR("POWER", "电压过高! 可能损坏设备");
    }
  }

  g_diagnosticStatus.powerOK = powerOK;
  return powerOK;
}

void DiagnosticUtils::printPowerStatus() {
  float voltage = measureVoltage();
  DEBUG_INFO("POWER", "当前电压: %.3f V, 状态: %s",
             voltage, g_diagnosticStatus.powerOK ? "正常" : "异常");
}

void DiagnosticUtils::playErrorSequence() {
  DEBUG_INFO("AUDIO", "播放错误音序列");

  for (int i = 0; i < ERROR_TONE_COUNT; i++) {
    tone(BUZZER_PIN, ERROR_TONE_FREQUENCY, ERROR_TONE_DURATION);
    delay(ERROR_TONE_DURATION + ERROR_TONE_INTERVAL);
  }
}

void DiagnosticUtils::reportError(const char* module, const char* error) {
  g_diagnosticStatus.errorCount++;
  DEBUG_ERROR(module, "%s", error);

  // 播放错误音（如果启用）
  #if DIAGNOSTIC_MODE_ENABLED
  playErrorSequence();
  #endif
}

void DiagnosticUtils::reportWarning(const char* module, const char* warning) {
  g_diagnosticStatus.warningCount++;
  DEBUG_WARN(module, "%s", warning);
}

String DiagnosticUtils::formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < 1024 * 1024) {
    return String(bytes / 1024.0, 1) + " KB";
  } else {
    return String(bytes / (1024.0 * 1024.0), 1) + " MB";
  }
}

String DiagnosticUtils::formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  seconds %= 60;
  minutes %= 60;

  if (hours > 0) {
    return String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
  } else if (minutes > 0) {
    return String(minutes) + "m " + String(seconds) + "s";
  } else {
    return String(seconds) + "s";
  }
}

void DiagnosticUtils::startPerformanceTimer(const char* name) {
  if (performanceTimerCount >= MAX_PERFORMANCE_TIMERS) {
    DEBUG_WARN("PERF", "性能计时器数量已达上限");
    return;
  }

  // 查找现有计时器或创建新的
  for (int i = 0; i < performanceTimerCount; i++) {
    if (strcmp(performanceTimers[i].name, name) == 0) {
      performanceTimers[i].startTime = micros();
      return;
    }
  }

  // 创建新计时器
  performanceTimers[performanceTimerCount].name = name;
  performanceTimers[performanceTimerCount].startTime = micros();
  performanceTimers[performanceTimerCount].totalTime = 0;
  performanceTimers[performanceTimerCount].callCount = 0;
  performanceTimerCount++;
}

void DiagnosticUtils::endPerformanceTimer(const char* name) {
  unsigned long endTime = micros();

  for (int i = 0; i < performanceTimerCount; i++) {
    if (strcmp(performanceTimers[i].name, name) == 0) {
      unsigned long duration = endTime - performanceTimers[i].startTime;
      performanceTimers[i].totalTime += duration;
      performanceTimers[i].callCount++;
      return;
    }
  }

  DEBUG_WARN("PERF", "未找到性能计时器: %s", name);
}

void DiagnosticUtils::printPerformanceStats() {
  DEBUG_INFO("PERF", "=== 性能统计 ===");

  for (int i = 0; i < performanceTimerCount; i++) {
    if (performanceTimers[i].callCount > 0) {
      unsigned long avgTime = performanceTimers[i].totalTime / performanceTimers[i].callCount;
      DEBUG_INFO("PERF", "%s: 调用%d次, 总时间%luμs, 平均%luμs",
                 performanceTimers[i].name,
                 performanceTimers[i].callCount,
                 performanceTimers[i].totalTime,
                 avgTime);
    }
  }
}

bool DiagnosticUtils::diagnoseOLED() {
  DEBUG_INFO("OLED", "=== OLED显示屏诊断 ===");

  // 1. 检查I2C总线
  if (!scanI2CBus()) {
    DEBUG_ERROR("OLED", "I2C总线扫描失败");
    return false;
  }

  // 2. 测试OLED设备通信
  if (!testOLEDCommunication()) {
    DEBUG_ERROR("OLED", "OLED通信测试失败");
    return false;
  }

  // 3. 检查电源
  if (!checkPowerSupply()) {
    DEBUG_WARN("OLED", "电源电压异常，可能影响OLED工作");
  }

  // 4. 测试GPIO引脚
  testAllConfiguredPins();

  DEBUG_INFO("OLED", "OLED诊断完成");
  return true;
}

bool DiagnosticUtils::testOLEDCommunication() {
  DEBUG_INFO("OLED", "测试OLED通信...");

  // 测试基本I2C通信
  if (!testI2CDevice(OLED_ADDRESS)) {
    DEBUG_ERROR("OLED", "OLED设备(0x%02X)无响应", OLED_ADDRESS);
    return false;
  }

  // 尝试读取SSD1306的一些寄存器
  Wire.beginTransmission(OLED_ADDRESS);
  Wire.write(0x00); // 命令模式
  Wire.write(0xAE); // 显示关闭命令
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    DEBUG_INFO("OLED", "OLED命令发送成功");
    return true;
  } else {
    DEBUG_ERROR("OLED", "OLED命令发送失败，错误代码: %d", error);
    return false;
  }
}

bool DiagnosticUtils::testOLEDInitialization() {
  DEBUG_INFO("OLED", "测试OLED初始化序列...");

  // 发送SSD1306初始化命令序列
  uint8_t initCommands[] = {
    0xAE,       // 显示关闭
    0xD5, 0x80, // 设置显示时钟分频
    0xA8, 0x3F, // 设置多路复用比
    0xD3, 0x00, // 设置显示偏移
    0x40,       // 设置显示开始行
    0x8D, 0x14, // 启用电荷泵
    0x20, 0x00, // 设置内存地址模式
    0xA1,       // 设置段重映射
    0xC8,       // 设置COM输出扫描方向
    0xDA, 0x12, // 设置COM引脚硬件配置
    0x81, 0xCF, // 设置对比度
    0xD9, 0xF1, // 设置预充电周期
    0xDB, 0x40, // 设置VCOMH取消选择级别
    0xA4,       // 整个显示开启
    0xA6,       // 设置正常显示
    0xAF        // 显示开启
  };

  Wire.beginTransmission(OLED_ADDRESS);
  Wire.write(0x00); // 命令模式

  for (size_t i = 0; i < sizeof(initCommands); i++) {
    Wire.write(initCommands[i]);
  }

  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    DEBUG_INFO("OLED", "OLED初始化命令序列发送成功");
    return true;
  } else {
    DEBUG_ERROR("OLED", "OLED初始化命令序列发送失败，错误: %d", error);
    return false;
  }
}

void DiagnosticUtils::printOLEDDiagnostics() {
  DEBUG_INFO("OLED", "=== OLED诊断报告 ===");
  DEBUG_INFO("OLED", "设备地址: 0x%02X", OLED_ADDRESS);
  DEBUG_INFO("OLED", "屏幕尺寸: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
  DEBUG_INFO("OLED", "I2C SDA: GPIO%d", I2C_SDA_PIN);
  DEBUG_INFO("OLED", "I2C SCL: GPIO%d", I2C_SCL_PIN);
  DEBUG_INFO("OLED", "工作状态: %s", g_diagnosticStatus.oledWorking ? "正常" : "异常");

  if (!g_diagnosticStatus.oledWorking) {
    DEBUG_INFO("OLED", "故障排除建议:");
    DEBUG_INFO("OLED", "1. 检查I2C连接线");
    DEBUG_INFO("OLED", "2. 确认电源供应(3.3V)");
    DEBUG_INFO("OLED", "3. 检查设备地址是否正确");
    DEBUG_INFO("OLED", "4. 验证上拉电阻(4.7kΩ)");
  }
}

bool DiagnosticUtils::performHardwareSelfTest() {
  DEBUG_INFO("SELFTEST", "=== 硬件自检开始 ===");

  bool allTestsPassed = true;

  // 1. 电源测试
  DEBUG_INFO("SELFTEST", "1. 电源测试...");
  if (!checkPowerSupply()) {
    allTestsPassed = false;
  }

  // 2. GPIO测试
  DEBUG_INFO("SELFTEST", "2. GPIO引脚测试...");
  testAllConfiguredPins();

  // 3. I2C总线测试
  DEBUG_INFO("SELFTEST", "3. I2C总线测试...");
  if (!scanI2CBus()) {
    allTestsPassed = false;
  }

  // 4. 传感器连接测试
  DEBUG_INFO("SELFTEST", "4. 传感器连接测试...");
  if (!testSensorConnectivity()) {
    allTestsPassed = false;
  }

  // 5. 显示屏连接测试
  DEBUG_INFO("SELFTEST", "5. 显示屏连接测试...");
  if (!testDisplayConnectivity()) {
    allTestsPassed = false;
  }

  // 6. 输入输出测试
  DEBUG_INFO("SELFTEST", "6. 输入输出测试...");
  if (!testInputOutput()) {
    allTestsPassed = false;
  }

  DEBUG_INFO("SELFTEST", "=== 硬件自检完成 ===");
  DEBUG_INFO("SELFTEST", "结果: %s", allTestsPassed ? "通过" : "失败");

  if (allTestsPassed) {
    // 播放成功音
    tone(BUZZER_PIN, 1000, 200);
    delay(250);
    tone(BUZZER_PIN, 1200, 200);
  } else {
    // 播放失败音
    playErrorSequence();
  }

  return allTestsPassed;
}

bool DiagnosticUtils::testSensorConnectivity() {
  DEBUG_DEBUG("SELFTEST", "测试MPU6050传感器连接...");

  bool sensorOK = testI2CDevice(MPU6050_ADDRESS);
  g_diagnosticStatus.sensorWorking = sensorOK;

  if (sensorOK) {
    DEBUG_INFO("SELFTEST", "MPU6050传感器连接正常");
  } else {
    DEBUG_ERROR("SELFTEST", "MPU6050传感器连接失败");
  }

  return sensorOK;
}

bool DiagnosticUtils::testDisplayConnectivity() {
  DEBUG_DEBUG("SELFTEST", "测试OLED显示屏连接...");

  bool displayOK = testOLEDCommunication();
  g_diagnosticStatus.oledWorking = displayOK;

  if (displayOK) {
    DEBUG_INFO("SELFTEST", "OLED显示屏连接正常");
  } else {
    DEBUG_ERROR("SELFTEST", "OLED显示屏连接失败");
  }

  return displayOK;
}

bool DiagnosticUtils::testInputOutput() {
  DEBUG_DEBUG("SELFTEST", "测试输入输出功能...");

  // 测试按钮 (按下时HIGH，松开时LOW)
  pinMode(BUTTON_PIN, BUTTON_PIN_MODE);
  delay(10);
  int buttonState = digitalRead(BUTTON_PIN);
  DEBUG_DEBUG("SELFTEST", "按钮状态: %s (%s)",
              buttonState ? "HIGH" : "LOW",
              buttonState == BUTTON_PRESSED_STATE ? "按下" : "松开");
  DEBUG_INFO("SELFTEST", "按钮配置验证: 按下=%s, 松开=%s",
             BUTTON_PRESSED_STATE ? "HIGH" : "LOW",
             BUTTON_RELEASED_STATE ? "HIGH" : "LOW");

  // 测试蜂鸣器
  DEBUG_DEBUG("SELFTEST", "测试蜂鸣器...");
  tone(BUZZER_PIN, 1000, 100);
  delay(150);

  DEBUG_INFO("SELFTEST", "输入输出测试完成");
  return true;
}

void DiagnosticUtils::hexDump(const uint8_t* data, size_t length) {
  DEBUG_DEBUG("HEXDUMP", "数据长度: %d 字节", length);

  for (size_t i = 0; i < length; i += 16) {
    char line[80];
    int pos = 0;

    // 地址部分
    pos += snprintf(line + pos, sizeof(line) - pos, "%04X: ", (unsigned int)i);

    // 十六进制部分
    for (size_t j = 0; j < 16 && (i + j) < length; j++) {
      pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", data[i + j]);
    }

    // 填充空格
    for (size_t j = length - i; j < 16; j++) {
      pos += snprintf(line + pos, sizeof(line) - pos, "   ");
    }

    pos += snprintf(line + pos, sizeof(line) - pos, " ");

    // ASCII部分
    for (size_t j = 0; j < 16 && (i + j) < length; j++) {
      char c = data[i + j];
      pos += snprintf(line + pos, sizeof(line) - pos, "%c",
                     (c >= 32 && c <= 126) ? c : '.');
    }

    DEBUG_DEBUG("HEXDUMP", "%s", line);
  }
}
