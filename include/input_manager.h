#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include <Bounce2.h>
#include "config.h"
#include "data_types.h"

class InputManager {
private:
  // Bounce2按钮对象
  Bounce button;

  // 按钮事件状态
  ButtonEvent buttonEvent;
  bool hasEvent = false;  // 事件标志
  unsigned long buttonPressTime = 0;

  // 长按和双击检测
  unsigned long longPressThreshold = 1000;   // 长按阈值 (ms)
  unsigned long doubleClickThreshold = 300;  // 双击阈值 (ms)
  unsigned long lastClickTime = 0;
  bool waitingForDoubleClick = false;
  bool pendingSingleClick = false;           // 待处理的单击事件
  unsigned long singleClickDelayTime = 0;    // 单击延迟处理时间
  
  // 蜂鸣器状态
  AudioData audioData;
  unsigned long buzzerStartTime = 0;
  bool buzzerActive = false;
  
  // 内部方法
  void processButtonEvent();
  void handleSingleClick();
  void handleDoubleClick();
  void handleLongPress();
  
public:
  InputManager();
  
  // 初始化
  bool initialize();
  void reset();
  
  // 按钮处理
  void update();
  ButtonEvent getButtonEvent();
  bool hasButtonEvent() const;
  void clearButtonEvent();
  
  // 按钮状态查询
  bool isButtonPressed() const;
  bool isButtonReleased() const;
  unsigned long getPressDuration() const;
  
  // 蜂鸣器控制
  void playTone(uint16_t frequency, uint16_t duration);
  void playBreakWarning();
  void playSuccessSound();
  void playErrorSound();
  void playStartSound();
  void playStopSound();
  void stopBuzzer();
  
  // 音频设置
  void setAudioEnabled(bool enabled);
  bool isAudioEnabled() const;
  void setVolume(uint8_t volume);  // 0-100
  
  // 状态查询
  bool isBuzzerActive() const;
  AudioData getAudioData() const;
  
  // 调试功能
  void printButtonInfo() const;
  void printAudioInfo() const;
};

#endif // INPUT_MANAGER_H
