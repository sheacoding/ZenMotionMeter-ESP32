#include "input_manager.h"

InputManager::InputManager() {
  // 初始化按钮事件
  buttonEvent.state = BUTTON_IDLE;
  buttonEvent.pressTime = 0;
  buttonEvent.releaseTime = 0;
  buttonEvent.duration = 0;
  buttonEvent.processed = true;
  hasEvent = false;

  // 初始化双击检测
  waitingForDoubleClick = false;
  lastClickTime = 0;
  buttonPressTime = 0;

  // 初始化音频数据
  audioData.enabled = true;
  audioData.frequency = 0;
  audioData.duration = 0;
  audioData.isPlaying = false;
  audioData.startTime = 0;
}

bool InputManager::initialize() {
  // 初始化Bounce2按钮对象
  button.attach(BUTTON_PIN, BUTTON_PIN_MODE);
  button.interval(25); // 25ms防抖间隔

  // 初始化蜂鸣器引脚
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  DEBUG_INFO("INPUT", "输入管理器初始化成功!");
  DEBUG_INFO("INPUT", "按钮配置: 按下=%s, 松开=%s, 引脚模式=%s",
             BUTTON_PRESSED_STATE ? "HIGH" : "LOW",
             BUTTON_RELEASED_STATE ? "HIGH" : "LOW",
             "INPUT");
  DEBUG_INFO("INPUT", "当前按钮状态: %s", button.read() ? "HIGH" : "LOW");

  return true;
}

void InputManager::reset() {
  // 重置按钮状态
  buttonEvent.state = BUTTON_IDLE;
  buttonEvent.processed = true;
  hasEvent = false;
  waitingForDoubleClick = false;
  buttonPressTime = 0;
  lastClickTime = 0;

  // 停止蜂鸣器
  stopBuzzer();

  DEBUG_DEBUG("INPUT", "输入管理器重置，当前按钮状态: %s",
              button.read() ? "HIGH" : "LOW");
}

void InputManager::update() {
  // 更新Bounce2按钮状态
  button.update();

  // 处理按钮事件
  processButtonEvent();

  // 更新蜂鸣器状态
  if (buzzerActive && audioData.isPlaying) {
    unsigned long currentTime = millis();
    if (currentTime - buzzerStartTime >= audioData.duration) {
      stopBuzzer();
    }
  }
}

void InputManager::processButtonEvent() {
  unsigned long currentTime = millis();

  // 添加实时按钮状态监控（每秒输出一次）
  static unsigned long lastStatusReport = 0;
  if (currentTime - lastStatusReport > 1000) {
    DEBUG_DEBUG("INPUT", "按钮状态监控: 当前=%s, 原始GPIO=%d",
                button.read() ? "HIGH" : "LOW",
                digitalRead(BUTTON_PIN));
    lastStatusReport = currentTime;
  }

  // 检测按钮按下事件 (LOW -> HIGH)
  if (button.rose()) {
    buttonPressTime = currentTime;
    buttonEvent.pressTime = currentTime;
    DEBUG_INFO("INPUT", "✓ 按钮按下检测 (Bounce2: LOW->HIGH)");
  }

  // 检测按钮释放事件 (HIGH -> LOW)
  if (button.fell()) {
    buttonEvent.releaseTime = currentTime;
    buttonEvent.duration = currentTime - buttonPressTime;

    DEBUG_INFO("INPUT", "✓ 按钮释放检测 (Bounce2)，持续时间: %lu ms", buttonEvent.duration);

    // 判断是长按还是短按
    if (buttonEvent.duration >= longPressThreshold) {
      buttonEvent.state = BUTTON_LONG_PRESSED;
      buttonEvent.processed = false;
      hasEvent = true;
      DEBUG_INFO("INPUT", "✓ 生成长按事件，持续时间: %lu ms", buttonEvent.duration);
    } else {
      // 检查是否为双击
      if (waitingForDoubleClick &&
          (currentTime - lastClickTime) <= doubleClickThreshold) {
        buttonEvent.state = BUTTON_DOUBLE_PRESSED;
        buttonEvent.processed = false;
        hasEvent = true;
        waitingForDoubleClick = false;
        DEBUG_INFO("INPUT", "✓ 生成双击事件");
      } else {
        // 立即生成单击事件
        buttonEvent.state = BUTTON_PRESSED;
        buttonEvent.processed = false;
        hasEvent = true;
        // 设置等待双击状态，为下次可能的双击做准备
        waitingForDoubleClick = true;
        lastClickTime = currentTime;
        DEBUG_INFO("INPUT", "✓ 生成单击事件，持续时间: %lu ms", buttonEvent.duration);
      }
    }
  }

  // 检查双击超时，清理等待状态
  if (waitingForDoubleClick &&
      (currentTime - lastClickTime) > doubleClickThreshold) {
    waitingForDoubleClick = false;
    DEBUG_DEBUG("INPUT", "双击等待超时，清理状态");
  }
}

void InputManager::handleSingleClick() {
  DEBUG_INFO("INPUT", "处理单击事件");
  // 单击事件将由主程序处理
}

void InputManager::handleDoubleClick() {
  DEBUG_INFO("INPUT", "处理双击事件");
  // 双击事件将由主程序处理
}

void InputManager::handleLongPress() {
  DEBUG_INFO("INPUT", "处理长按事件");
  // 长按事件将由主程序处理
}

ButtonEvent InputManager::getButtonEvent() {
  return buttonEvent;
}

bool InputManager::hasButtonEvent() const {
  bool result = hasEvent && !buttonEvent.processed;
  DEBUG_DEBUG("INPUT", "检查按钮事件: hasEvent=%d, processed=%d, result=%d", hasEvent, buttonEvent.processed, result);
  return result;
}

void InputManager::clearButtonEvent() {
  buttonEvent.processed = true;
  buttonEvent.state = BUTTON_IDLE;
  hasEvent = false;
  DEBUG_DEBUG("INPUT", "✓ 按钮事件已清理");
}

bool InputManager::isButtonPressed() const {
  return button.read() == BUTTON_PRESSED_STATE;
}

bool InputManager::isButtonReleased() const {
  return button.read() == BUTTON_RELEASED_STATE;
}

unsigned long InputManager::getPressDuration() const {
  if (isButtonPressed() && buttonPressTime > 0) {
    return millis() - buttonPressTime;
  }
  return 0;
}

void InputManager::playTone(uint16_t frequency, uint16_t duration) {
  if (!audioData.enabled) {
    return;
  }
  
  audioData.frequency = frequency;
  audioData.duration = duration;
  audioData.isPlaying = true;
  audioData.startTime = millis();
  
  buzzerStartTime = millis();
  buzzerActive = true;
  
  // 使用tone函数播放声音
  tone(BUZZER_PIN, frequency, duration);
  
  DEBUG_PRINTF("播放音调: %d Hz, %d ms\n", frequency, duration);
}

void InputManager::playBreakWarning() {
  playTone(BUZZER_BREAK_FREQUENCY, BUZZER_DURATION);
}

void InputManager::playSuccessSound() {
  playTone(BUZZER_SUCCESS_FREQUENCY, BUZZER_DURATION);
}

void InputManager::playErrorSound() {
  // 播放两声短促的错误提示音
  playTone(800, 100);
  delay(50);
  playTone(600, 100);
}

void InputManager::playStartSound() {
  // 播放上升音调
  playTone(1000, 100);
  delay(50);
  playTone(1200, 100);
  delay(50);
  playTone(1500, 150);
}

void InputManager::playStopSound() {
  // 播放下降音调
  playTone(1500, 100);
  delay(50);
  playTone(1200, 100);
  delay(50);
  playTone(1000, 150);
}

void InputManager::stopBuzzer() {
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  
  audioData.isPlaying = false;
  buzzerActive = false;
  
  DEBUG_PRINTLN("蜂鸣器停止");
}

void InputManager::setAudioEnabled(bool enabled) {
  audioData.enabled = enabled;
  if (!enabled) {
    stopBuzzer();
  }
  DEBUG_PRINTF("音频%s\n", enabled ? "开启" : "关闭");
}

bool InputManager::isAudioEnabled() const {
  return audioData.enabled;
}

void InputManager::setVolume(uint8_t volume) {
  // ESP32-C3的tone函数不支持音量控制
  // 这里可以通过PWM实现简单的音量控制
  // 暂时只记录设置值
  DEBUG_PRINTF("音量设置为: %d%%\n", volume);
}

bool InputManager::isBuzzerActive() const {
  return buzzerActive;
}

AudioData InputManager::getAudioData() const {
  return audioData;
}

void InputManager::printButtonInfo() const {
  DEBUG_INFO("INPUT", "=== 按钮信息 ===");
  DEBUG_INFO("INPUT", "当前状态: %s (%s)",
             isButtonPressed() ? "按下" : "释放",
             digitalRead(BUTTON_PIN) ? "HIGH" : "LOW");
  DEBUG_INFO("INPUT", "按钮配置: 按下=%s, 松开=%s",
             BUTTON_PRESSED_STATE ? "HIGH" : "LOW",
             BUTTON_RELEASED_STATE ? "HIGH" : "LOW");
  DEBUG_INFO("INPUT", "按钮事件状态: %d", buttonEvent.state);
  DEBUG_INFO("INPUT", "事件已处理: %s", buttonEvent.processed ? "是" : "否");
  DEBUG_INFO("INPUT", "按下时间: %lu ms", buttonEvent.pressTime);
  DEBUG_INFO("INPUT", "释放时间: %lu ms", buttonEvent.releaseTime);
  DEBUG_INFO("INPUT", "持续时间: %lu ms", buttonEvent.duration);
  DEBUG_INFO("INPUT", "等待双击: %s", waitingForDoubleClick ? "是" : "否");
}

void InputManager::printAudioInfo() const {
  DEBUG_PRINTLN("=== 音频信息 ===");
  DEBUG_PRINTF("音频开启: %s\n", audioData.enabled ? "是" : "否");
  DEBUG_PRINTF("当前频率: %d Hz\n", audioData.frequency);
  DEBUG_PRINTF("播放时长: %d ms\n", audioData.duration);
  DEBUG_PRINTF("正在播放: %s\n", audioData.isPlaying ? "是" : "否");
  DEBUG_PRINTF("蜂鸣器活跃: %s\n", buzzerActive ? "是" : "否");
}
