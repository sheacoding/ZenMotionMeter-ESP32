#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <u8g2_wqy.h>
#include "config.h"
#include "data_types.h"

class DisplayManager {
private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;
  DisplayData displayData;
  DisplayPage currentPage;

  // 显示状态
  bool isInitialized = false;
  bool needsUpdate = true;
  unsigned long lastUpdate = 0;

  // 动画相关
  int animationFrame = 0;
  unsigned long lastAnimationUpdate = 0;

  // 页面切换
  unsigned long pageChangeTime = 0;
  bool pageTransition = false;

  // 开机动画相关
  unsigned long bootAnimationStartTime = 0;
  bool bootAnimationActive = false;
  int bootAnimationFrame = 0;

  // 主菜单相关
  MainMenuOption currentMenuOption = MENU_START_PRACTICE;
  unsigned long lastMenuScroll = 0;
  
  // 内部方法
  void drawBootAnimationPage(const ZenMotionData& data);
  void drawMainMenuPage(const ZenMotionData& data);
  void drawMainPage(const ZenMotionData& data);
  void drawStatsPage(const ZenMotionData& data);
  void drawSettingsPage(const ZenMotionData& data);
  void drawCalibrationPage(const ZenMotionData& data);
  void drawHistoryPage(const ZenMotionData& data);
  
  // UI组件绘制方法
  void drawStabilityScore(int x, int y, float score, bool isStable);
  void drawTimeDisplay(int x, int y, unsigned long timeMs, bool showSeconds = true);
  void drawProgressBar(int x, int y, int width, int height, float percentage);
  void drawBatteryIcon(int x, int y, float voltage, bool isCharging);
  void drawWiFiIcon(int x, int y, bool connected);
  void drawStatusIcons(const ZenMotionData& data);
  
  // 文本和图形辅助方法
  void drawCenteredText(const String& text, int y, int textSize = 1);
  void drawRightAlignedText(const String& text, int x, int y, int textSize = 1);
  void drawScrollingText(const String& text, int x, int y, int maxWidth);
  void drawFrame(int x, int y, int width, int height);
  
  // 动画方法
  void updateAnimation();
  void drawLoadingAnimation(int x, int y);
  void drawBreakWarning();
  
  // 格式化方法
  String formatTime(unsigned long timeMs, bool showSeconds = true);
  String formatScore(float score);
  String formatDate(uint16_t year, uint8_t month, uint8_t day);
  
public:
  DisplayManager();
  
  // 初始化和配置
  bool initialize();
  void reset();
  bool isReady() const;
  
  // 页面管理
  void setPage(DisplayPage page);
  DisplayPage getCurrentPage() const;
  void nextPage();
  void previousPage();

  // 开机动画管理
  void startBootAnimation();
  bool isBootAnimationActive() const;
  bool isBootAnimationComplete() const;

  // 主菜单管理
  void setMenuOption(MainMenuOption option);
  MainMenuOption getCurrentMenuOption() const;
  void nextMenuOption();
  void previousMenuOption();
  MainMenuOption getSelectedMenuOption() const;
  
  // 显示更新
  void update(const ZenMotionData& data);
  void forceUpdate();
  bool needsRefresh() const;
  
  // 显示控制
  void setBrightness(uint8_t brightness);
  void turnOn();
  void turnOff();
  bool isOn() const;
  
  // 消息显示
  void showMessage(const String& message, int duration = 2000);
  void showWarning(const String& warning);
  void showError(const String& error);
  void clearMessage();
  
  // 特殊显示模式
  void showSplashScreen();
  void showCalibrationProgress(int percentage);
  void showShutdownScreen();
  
  // 调试功能
  void printDisplayInfo() const;
  bool hasError() const;
  String getErrorMessage() const;
};

#endif // DISPLAY_MANAGER_H
