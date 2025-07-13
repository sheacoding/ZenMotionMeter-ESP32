#include "display_manager.h"
#include "diagnostic_utils.h"
#include "data_manager.h"

DisplayManager::DisplayManager() : display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE) {
  currentPage = PAGE_BOOT_ANIMATION;
  displayData.needUpdate = true;
  displayData.lastUpdate = 0;
  displayData.brightness = 255;
  displayData.isOn = true;

  // 初始化新增的成员变量
  animationFrame = 0;
  lastAnimationUpdate = 0;
  pageChangeTime = 0;
  pageTransition = false;
  bootAnimationStartTime = 0;
  bootAnimationActive = false;
  bootAnimationFrame = 0;
  currentMenuOption = MENU_START_PRACTICE;
  lastMenuScroll = 0;
}

bool DisplayManager::initialize() {
  DEBUG_INFO("DISPLAY", "开始初始化OLED显示屏...");

  // 执行OLED专项诊断
  if (!DiagnosticUtils::diagnoseOLED()) {
    DEBUG_ERROR("DISPLAY", "OLED诊断失败，尝试恢复...");

    // 尝试不同的I2C时钟速度
    if (!DiagnosticUtils::testI2CWithDifferentSpeeds(OLED_ADDRESS)) {
      DEBUG_ERROR("DISPLAY", "所有I2C速度测试失败");
      DiagnosticUtils::reportError("DISPLAY", "OLED设备无响应");
      return false;
    }
  }

  // 尝试初始化OLED显示屏
  DEBUG_INFO("DISPLAY", "尝试初始化SSD1306...");
  PERFORMANCE_START("OLED_INIT");

  bool initSuccess = false;
  for (int attempt = 1; attempt <= 3; attempt++) {
    DEBUG_INFO("DISPLAY", "初始化尝试 %d/3", attempt);

    display.begin();
    if (display.getDisplayHeight() > 0) {
      DEBUG_INFO("DISPLAY", "SSD1306初始化成功 (尝试 %d)", attempt);
      initSuccess = true;
      break;
    } else {
      DEBUG_WARN("DISPLAY", "SSD1306初始化失败 (尝试 %d)", attempt);
      delay(100 * attempt); // 递增延迟
    }
  }

  PERFORMANCE_END("OLED_INIT");

  if (!initSuccess) {
    DEBUG_ERROR("DISPLAY", "OLED显示屏初始化失败!");
    DiagnosticUtils::reportError("DISPLAY", "SSD1306初始化失败");
    return false;
  }

  // 设置显示参数
  DEBUG_DEBUG("DISPLAY", "配置显示参数...");
  
  // 调试：打印显示器实际参数
  DEBUG_INFO("DISPLAY", "显示器实际宽度: %d", display.getDisplayWidth());
  DEBUG_INFO("DISPLAY", "显示器实际高度: %d", display.getDisplayHeight());
  DEBUG_INFO("DISPLAY", "SCREEN_WIDTH宏定义: %d", SCREEN_WIDTH);
  DEBUG_INFO("DISPLAY", "SCREEN_HEIGHT宏定义: %d", SCREEN_HEIGHT);
  
  // 检查宏定义与实际显示器参数是否匹配
  if (display.getDisplayWidth() != SCREEN_WIDTH) {
    DEBUG_WARN("DISPLAY", "警告：显示器实际宽度(%d)与SCREEN_WIDTH(%d)不匹配!", 
               display.getDisplayWidth(), SCREEN_WIDTH);
  }
  if (display.getDisplayHeight() != SCREEN_HEIGHT) {
    DEBUG_WARN("DISPLAY", "警告：显示器实际高度(%d)与SCREEN_HEIGHT(%d)不匹配!", 
               display.getDisplayHeight(), SCREEN_HEIGHT);
  }
  
  display.clearBuffer();
  display.enableUTF8Print();  // 启用UTF8支持
  display.setFont(u8g2_font_wqy12_t_gb2312);  // 使用GB2312中文字体
  display.setFontDirection(0);
  



  
  // 启动开机动画
  startBootAnimation();

  isInitialized = true;
  g_diagnosticStatus.oledWorking = true;
  DEBUG_INFO("DISPLAY", "OLED显示屏初始化成功!");

  return true;
}

void DisplayManager::reset() {
  if (!isInitialized) return;

  display.clearBuffer();
  display.sendBuffer();
  currentPage = PAGE_MAIN;
  needsUpdate = true;
  animationFrame = 0;
  pageTransition = false;
}

bool DisplayManager::isReady() const {
  return isInitialized;
}

void DisplayManager::setPage(DisplayPage page) {
  if (currentPage != page) {
    currentPage = page;
    needsUpdate = true;
    pageChangeTime = millis();
    pageTransition = true;
  }
}

DisplayPage DisplayManager::getCurrentPage() const {
  return currentPage;
}

void DisplayManager::nextPage() {
  DisplayPage nextPage = static_cast<DisplayPage>((currentPage + 1) % 7);
  setPage(nextPage);
}

void DisplayManager::previousPage() {
  DisplayPage prevPage = static_cast<DisplayPage>((currentPage + 6) % 7);
  setPage(prevPage);
}

void DisplayManager::update(const ZenMotionData& data) {
  if (!isInitialized || !displayData.isOn) return;
  
  // 检查是否需要更新
  unsigned long currentTime = millis();
  if (!needsUpdate && (currentTime - lastUpdate) < DISPLAY_UPDATE_INTERVAL) {
    return;
  }
  
  // 更新动画
  updateAnimation();

  // 使用firstPage/nextPage循环进行显示
  display.firstPage();
  do {
    // 根据当前页面绘制内容
    switch (currentPage) {
      case PAGE_BOOT_ANIMATION:
        drawBootAnimationPage(data);
        break;
      case PAGE_MAIN_MENU:
        drawMainMenuPage(data);
        break;
      case PAGE_MAIN:
        drawMainPage(data);
        break;
      case PAGE_STATS:
        drawStatsPage(data);
        break;
      case PAGE_SETTINGS:
        drawSettingsPage(data);
        break;
      case PAGE_CALIBRATION:
        drawCalibrationPage(data);
        break;
      case PAGE_HISTORY:
        drawHistoryPage(data);
        break;
    }

    // 绘制状态图标
    drawStatusIcons(data);

    // 如果有破定警告，显示警告
    if (!data.stability.isStable) {
      drawBreakWarning();
    }
  } while (display.nextPage());
  
  lastUpdate = currentTime;
  needsUpdate = false;
}

void DisplayManager::forceUpdate() {
  needsUpdate = true;
}

bool DisplayManager::needsRefresh() const {
  return needsUpdate;
}

void DisplayManager::setBrightness(uint8_t brightness) {
  displayData.brightness = brightness;
  // u8g2设置对比度
  display.setContrast(brightness);
}

void DisplayManager::turnOn() {
  if (!isInitialized) return;

  display.setPowerSave(0);  // 0 = 开启显示
  displayData.isOn = true;
  needsUpdate = true;
}

void DisplayManager::turnOff() {
  if (!isInitialized) return;

  display.setPowerSave(1);  // 1 = 关闭显示
  displayData.isOn = false;
}

bool DisplayManager::isOn() const {
  return displayData.isOn;
}

void DisplayManager::showMessage(const String& message, int duration) {
  if (!isInitialized) return;

  // 设置中文字体
  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 使用firstPage/nextPage循环显示中文
  display.firstPage();
  do {
    // 精确计算文本宽度实现水平居中
    // 使用getUTF8Width计算UTF-8编码的中文文本宽度
    int textWidth = display.getUTF8Width(message.c_str());
    int x = (SCREEN_WIDTH - textWidth) / 2;

    // 确保居中计算结果在有效范围内
    if (x < 0) x = 0;
    if (x + textWidth > SCREEN_WIDTH) x = SCREEN_WIDTH - textWidth;

    // 垂直居中计算 - 使用屏幕中心位置
    int y = SCREEN_HEIGHT / 2 + 6;  // +6是字体高度的大约一半，确保垂直居中

    // 确保Y坐标在有效范围内
    if (y < 12) y = 12;  // 最小Y坐标（字体基线位置）
    if (y > SCREEN_HEIGHT - 2) y = SCREEN_HEIGHT - 2;  // 最大Y坐标

    display.setCursor(x, y);
    display.print(message);
  } while (display.nextPage());

  delay(duration);
  needsUpdate = true;
}

void DisplayManager::showWarning(const String& warning) {
  showMessage("⚠ " + warning, 3000);
}

void DisplayManager::showError(const String& error) {
  showMessage("✗ " + error, 5000);
}

void DisplayManager::clearMessage() {
  needsUpdate = true;
}

void DisplayManager::showSplashScreen() {
  if (!isInitialized) return;

  // 使用firstPage/nextPage循环显示启动画面
  display.firstPage();
  do {
    // 显示项目名称 (中文字体) - 动态居中
    display.setFont(u8g2_font_wqy12_t_gb2312);
    String title = "气定神闲仪";
    // 使用getUTF8Width计算中文文本宽度
    int titleWidth = display.getUTF8Width(title.c_str());
    int titleX = (SCREEN_WIDTH - titleWidth) / 2;
    if (titleX < 0) titleX = 0;
    if (titleX + titleWidth > SCREEN_WIDTH) titleX = SCREEN_WIDTH - titleWidth;
    display.drawUTF8(titleX, 20, title.c_str());

    // 显示英文名称 (小字体) - 动态居中
    display.setFont(u8g2_font_6x10_tf);
    String subtitle = "Zen-Motion Meter";
    int subtitleWidth = display.getStrWidth(subtitle.c_str());
    int subtitleX = (SCREEN_WIDTH - subtitleWidth) / 2;
    if (subtitleX < 0) subtitleX = 0;
    if (subtitleX + subtitleWidth > SCREEN_WIDTH) subtitleX = SCREEN_WIDTH - subtitleWidth;
    display.drawStr(subtitleX, 40, subtitle.c_str());

    // // 显示版本信息 - 动态居中
    // String version = "v" + String(PROJECT_VERSION);
    // int versionWidth = display.getStrWidth(version.c_str());
    // int versionX = (SCREEN_WIDTH - versionWidth) / 2;
    // if (versionX < 0) versionX = 0;
    // if (versionX + versionWidth > SCREEN_WIDTH) versionX = SCREEN_WIDTH - versionWidth;
    // display.setCursor(versionX, 55);
    // display.print(version);
  } while (display.nextPage());

  delay(2000);
  needsUpdate = true;
}

void DisplayManager::showCalibrationProgress(int percentage) {
  if (!isInitialized) return;

  display.clearBuffer();
  display.setFont(u8g2_font_wqy12_t_chinese3);

  drawCenteredText("传感器校准中...", 15, 1);

  // 绘制进度条
  drawProgressBar(20, 30, 88, 10, percentage);

  // 显示百分比 - 精确水平居中
  String percentText = String(percentage) + "%";
  // 必须在设置字体后立即计算文本宽度
  int percentWidth = display.getStrWidth(percentText.c_str());
  int percentX = (SCREEN_WIDTH - percentWidth) / 2;
  // 确保居中计算结果在有效范围内
  if (percentX < 0) percentX = 0;
  if (percentX + percentWidth > SCREEN_WIDTH) percentX = SCREEN_WIDTH - percentWidth;
  display.drawStr(percentX, 55, percentText.c_str());

  display.sendBuffer();
}

void DisplayManager::showShutdownScreen() {
  if (!isInitialized) return;

  display.clearBuffer();
  display.setFont(u8g2_font_wqy12_t_chinese3);

  drawCenteredText("设备关闭中...", 30, 1);
  drawCenteredText("感谢使用", 45, 1);

  display.sendBuffer();
  delay(2000);

  display.clearDisplay();
  display.display();
}

void DisplayManager::drawMainPage(const ZenMotionData& data) {
  // 1. 设置大字体显示稳定性评分 - 精确水平居中
  display.setFont(u8g2_font_logisoso28_tn);  // 稍微缩小字体，从32改为28
  String scoreText = formatScore(data.stability.score);
  int scoreWidth = display.getStrWidth(scoreText.c_str());
  int scoreX = (SCREEN_WIDTH - scoreWidth) / 2;
  if (scoreX < 0) scoreX = 0;
  if (scoreX + scoreWidth > SCREEN_WIDTH) scoreX = SCREEN_WIDTH - scoreWidth;
  display.drawStr(scoreX, 28, scoreText.c_str());  // Y坐标从30调整到28

  // 2. 根据稳定状态决定是否显示标签
  if (data.stability.isStable) {
    // 稳定时显示"稳定性评分"标签
    display.setFont(u8g2_font_wqy12_t_gb2312);
    String label = "稳定性评分";
    int labelWidth = display.getUTF8Width(label.c_str());
    int labelX = (SCREEN_WIDTH - labelWidth) / 2;
    if (labelX < 0) labelX = 0;
    if (labelX + labelWidth > SCREEN_WIDTH) labelX = SCREEN_WIDTH - labelWidth;
    display.drawUTF8(labelX, 42, label.c_str());  // Y坐标从45调整到42
  } else {
    // 不稳定时显示破定提醒（闪烁效果）
    if ((millis() / 500) % 2 == 0) {
      display.setFont(u8g2_font_wqy12_t_gb2312);
      String warning = "! 破定提醒 !";
      int warningWidth = display.getUTF8Width(warning.c_str());
      int warningX = (SCREEN_WIDTH - warningWidth) / 2;
      if (warningX < 0) warningX = 0;
      display.drawUTF8(warningX, 42, warning.c_str());  // 使用相同的Y坐标
    }
  }

  // 3. 底部信息栏 - 修复重叠问题
  // 练习时长（左下角）
  display.setFont(u8g2_font_wqy12_t_gb2312);
  String practiceLabel = "练习:";
  int practiceLabelWidth = display.getUTF8Width(practiceLabel.c_str());
  display.drawUTF8(2, 64, practiceLabel.c_str());
  
  // 切换到英文字体显示时间，确保正确计算X坐标
  display.setFont(u8g2_font_6x10_tf);
  String practiceTime = formatTime(data.currentSession.duration, true);  // 改为true以显示秒数
  int practiceTimeX = 2 + practiceLabelWidth + 2;  // 使用预先计算的宽度
  display.drawStr(practiceTimeX, 64, practiceTime.c_str());
  
  // 累计时长（右下角）
  // 计算实时累计时长：今日已完成时长 + 当前练习时长
  unsigned long realtimeTotalTime = data.todayStats.totalTime;
  if (data.status.currentState == STATE_PRACTICING || data.status.currentState == STATE_PAUSED) {
    realtimeTotalTime += data.currentSession.duration;
  }
  
  // 先计算时间文本宽度
  String totalTime = formatTime(realtimeTotalTime, false);
  int totalTimeWidth = display.getStrWidth(totalTime.c_str());
  
  // 切换到中文字体显示"累计:"
  display.setFont(u8g2_font_wqy12_t_gb2312);
  String totalLabel = "累计:";
  int totalLabelWidth = display.getUTF8Width(totalLabel.c_str());
  int totalStartX = SCREEN_WIDTH - totalTimeWidth - totalLabelWidth - 4;
  display.drawUTF8(totalStartX, 64, totalLabel.c_str());
  
  // 切换到英文字体显示时间
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(totalStartX + totalLabelWidth + 2, 64, totalTime.c_str());
}

void DisplayManager::drawStatsPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 标题 - 精确水平居中
  String title = "统计信息";
  // 必须在设置字体后立即计算文本宽度
  // 使用getUTF8Width计算中文文本宽度
  int titleWidth = display.getUTF8Width(title.c_str());
  int titleX = (SCREEN_WIDTH - titleWidth) / 2;
  // 确保居中计算结果在有效范围内
  if (titleX < 0) titleX = 0;
  if (titleX + titleWidth > SCREEN_WIDTH) titleX = SCREEN_WIDTH - titleWidth;
  display.setCursor(titleX, 12);
  display.print(title);

  // 今日统计
  display.setFont(u8g2_font_6x10_tf);
  String line1 = "今日练习: " + String(data.todayStats.sessionCount) + "次";
  display.setCursor(0, 25);
  display.print(line1);

  String line2 = "今日时长: " + formatTime(data.todayStats.totalTime);
  display.setCursor(0, 35);
  display.print(line2);

  String line3 = "平均评分: " + formatScore(data.todayStats.avgStability);
  display.setCursor(0, 45);
  display.print(line3);

  String line4 = "最佳评分: " + formatScore(data.todayStats.bestStability);
  display.setCursor(0, 55);
  display.print(line4);

  String line5 = "破定次数: " + String(data.todayStats.totalBreaks);
  display.setCursor(0, 65);
  display.print(line5);
}

void DisplayManager::drawSettingsPage(const ZenMotionData& data) {
  // 如果在日期时间编辑模式，显示日期时间编辑页面
  if (settingsState.inDateTimeEdit) {
    drawDateTimeEditPage(data);
    return;
  }
  
    display.setFont(u8g2_font_wqy12_t_gb2312);
    
    // 标题
    drawCenteredText("设置", 12, 1);
    
    // 绘制分隔线
    display.drawHLine(0, 14, SCREEN_WIDTH);
    
    // 设置项列表
    int y = 20;
    int lineHeight = 16;  // 增加行高来避免显示重叠
  
  // 计算可见项目的范围
  int startItem = settingsState.scrollOffset;
  int endItem = min(startItem + settingsState.maxVisibleItems, (int)SETTINGS_ITEM_COUNT);
  
  // 只显示当前屏幕可见的设置项
  for (int i = startItem; i < endItem; i++) {
    SettingsMenuItem item = (SettingsMenuItem)i;
    
    // 高亮当前选中的项
    if (item == settingsState.currentItem) {
      display.drawBox(0, y - 8, SCREEN_WIDTH, 9);
      display.setDrawColor(0);
    }
    
    // 显示设置项名称 - 使用中文字体
    display.setFont(u8g2_font_wqy12_t_gb2312);
    String itemText = getSettingsItemText(item);
    display.drawUTF8(2, y, itemText.c_str());
    
    // 显示当前值
    String valueText;
    if (item == SETTINGS_DATE_TIME) {
      // 特殊处理日期时间显示
      extern TimeManager* timeManager;
      if (timeManager) {
        DateTime currentDT = timeManager->getCurrentDateTime();
        valueText = String(currentDT.year) + "/" + 
                   String(currentDT.month) + "/" + 
                   String(currentDT.day);
      } else {
        valueText = "--/--/--";
      }
    } else {
      valueText = getSettingsValueText(item, data.settings);
    }
    
    // 右对齐显示值 - 根据内容选择字体
    if (valueText.indexOf("开启") >= 0 || valueText.indexOf("关闭") >= 0 || 
        valueText.indexOf("分钟") >= 0) {
      // 中文内容使用中文字体
      display.setFont(u8g2_font_wqy12_t_gb2312);
      int valueWidth = display.getUTF8Width(valueText.c_str());
      display.drawUTF8(SCREEN_WIDTH - valueWidth - 2, y, valueText.c_str());
    } else {
      // 纯数字使用英文字体
      display.setFont(u8g2_font_6x10_tf);
      int valueWidth = display.getStrWidth(valueText.c_str());
      display.drawStr(SCREEN_WIDTH - valueWidth - 2, y, valueText.c_str());
    }
    
    // 恢复绘制颜色
    if (item == settingsState.currentItem) {
      display.setDrawColor(1);
    }
    
    y += lineHeight;
  }
  
  // 显示滚动指示器
  if (SETTINGS_ITEM_COUNT > settingsState.maxVisibleItems) {
    // 右侧滚动条
    int scrollBarHeight = 45;  // 滚动条总高度
    int scrollBarY = 18;       // 滚动条起始位置
    int scrollBarX = SCREEN_WIDTH - 2;
    
    // 绘制滚动条背景
    display.drawVLine(scrollBarX, scrollBarY, scrollBarHeight);
    
    // 计算滚动指示器位置 - 修复边界问题
    int maxScrollOffset = (int)SETTINGS_ITEM_COUNT - settingsState.maxVisibleItems;
    if (maxScrollOffset <= 0) maxScrollOffset = 1;  // 避免除零错误
    
    int indicatorHeight = max(3, scrollBarHeight * settingsState.maxVisibleItems / (int)SETTINGS_ITEM_COUNT);
    int indicatorY = scrollBarY + (scrollBarHeight - indicatorHeight) * settingsState.scrollOffset / maxScrollOffset;
    
    // 确保指示器不超出滚动条范围
    if (indicatorY < scrollBarY) indicatorY = scrollBarY;
    if (indicatorY + indicatorHeight > scrollBarY + scrollBarHeight) {
      indicatorY = scrollBarY + scrollBarHeight - indicatorHeight;
    }
    
    // 绘制滚动指示器
    display.drawBox(scrollBarX - 1, indicatorY, 3, indicatorHeight);
  }
  
  // 移除底部提示，让界面更简洁
}

void DisplayManager::drawCalibrationPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 标题
  drawCenteredText("传感器校准", 12, 1);

  // 使用中文字体显示中文内容
  if (data.calibration.isCalibrated) {
    display.drawUTF8(10, 25, "校准状态: 已完成");
    display.drawUTF8(10, 40, "校准时间: ");
    // 这里可以显示校准时间的格式化版本
    display.drawUTF8(10, 55, "按按钮重新校准");
  } else {
    display.drawUTF8(10, 25, "校准状态: 未校准");
    display.drawUTF8(10, 40, "请保持设备静止");
    display.drawUTF8(10, 55, "按按钮开始校准");
  }
}

void DisplayManager::drawHistoryPage(const ZenMotionData& data) {
  // 标题 - 使用中文字体并居中
  display.setFont(u8g2_font_wqy12_t_gb2312);
  String title = "历史记录";
  int titleWidth = display.getUTF8Width(title.c_str());
  int titleX = (SCREEN_WIDTH - titleWidth) / 2;
  if (titleX < 0) titleX = 0;
  display.drawUTF8(titleX, 12, title.c_str());

  // 绘制分隔线
  display.drawHLine(0, 14, SCREEN_WIDTH);

  // 今日数据部分 - 使用框架突出显示
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(2, 24, "Today:");
  
  // 今日练习时长和次数在同一行
  String todayLine1 = formatTime(data.todayStats.totalTime) + " / " + 
                      String(data.todayStats.sessionCount) + "x";
  display.drawStr(45, 24, todayLine1.c_str());
  
  // 今日评分和破定次数
  String todayLine2 = "Score:" + formatScore(data.todayStats.avgStability) + 
                      " Brk:" + String(data.todayStats.totalBreaks);
  display.drawStr(2, 34, todayLine2.c_str());
  
  // 绘制分隔线
  display.drawHLine(0, 37, SCREEN_WIDTH);
  
  // 本周统计标题
  display.drawStr(2, 47, "Week Summary:");
  
  // 计算本周数据（简化版，显示最近7天累计）
  unsigned long weekTotalTime = data.todayStats.totalTime;
  int weekSessions = data.todayStats.sessionCount;
  float weekAvgScore = data.todayStats.avgStability;
  
  // 如果有历史数据访问，这里应该累加历史数据
  // 目前简化处理，只显示今日数据作为示例
  
  // 本周练习时长
  String weekTime = "Time: " + formatTime(weekTotalTime);
  display.drawStr(2, 57, weekTime.c_str());
  
  // 本周练习次数和平均分
  String weekStats = "Sess:" + String(weekSessions) + 
                     " Avg:" + formatScore(weekAvgScore);
  display.drawStr(2, 64, weekStats.c_str());
  
  // 在右上角显示日期（如果有RTC的话）
  // display.setFont(u8g2_font_5x7_tf);
  // display.drawStr(SCREEN_WIDTH - 35, 8, "12/07");
}

void DisplayManager::drawStabilityScore(int x, int y, float score, bool isStable) {
  display.setFont(u8g2_font_logisoso32_tn);
  String scoreText = String((int)score);
  display.drawStr(x, y, scoreText.c_str());

  // 绘制稳定性指示器
  if (isStable) {
    display.drawDisc(x + 50, y - 8, 3, U8G2_DRAW_ALL);
  } else {
    display.drawCircle(x + 50, y - 8, 3, U8G2_DRAW_ALL);
  }
}

void DisplayManager::drawTimeDisplay(int x, int y, unsigned long timeMs, bool showSeconds) {
  display.setFont(u8g2_font_6x10_tf);
  String timeStr = formatTime(timeMs, showSeconds);
  display.drawStr(x, y, timeStr.c_str());
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, float percentage) {
  // 绘制边框
  display.drawFrame(x, y, width, height);

  // 绘制填充
  int fillWidth = (int)((width - 2) * percentage / 100.0);
  if (fillWidth > 0) {
    display.drawBox(x + 1, y + 1, fillWidth, height - 2);
  }
}

void DisplayManager::drawBatteryIcon(int x, int y, float voltage, bool isCharging) {
  // 计算电池电量百分比
  float percentage = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
  if (percentage > 100) percentage = 100;
  if (percentage < 0) percentage = 0;

  // 绘制电池外框
  display.drawFrame(x, y, 12, 6);
  display.drawBox(x + 12, y + 1, 2, 4);  // 电池正极

  // 绘制电量
  int fillWidth = (int)(10 * percentage / 100.0);
  if (fillWidth > 0) {
    display.drawBox(x + 1, y + 1, fillWidth, 4);
  }

  // 如果在充电，显示闪电符号
  if (isCharging) {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(x + 15, y + 6, "⚡");
  }
}


void DisplayManager::drawStatusIcons(const ZenMotionData& data) {
  // 绘制电池图标
  drawBatteryIcon(SCREEN_WIDTH - 20, 0, data.status.batteryVoltage, false);

  // 如果有错误，显示错误图标
  if (data.status.sensorError || data.status.displayError) {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 8, "!");
  }
}

void DisplayManager::drawCenteredText(const String& text, int y, int textSize) {
  // textSize参数在u8g2中通过字体设置，这里忽略
  // 注意：调用此方法前必须先设置正确的字体

  // 根据U8G2文档，drawStr/drawUTF8的x参数是文本左边缘位置
  // 所以居中计算应该是：(屏幕宽度 - 文本宽度) / 2
  // 使用getUTF8Width计算UTF-8编码的文本宽度
  int textWidth = display.getUTF8Width(text.c_str());
  int x = (SCREEN_WIDTH - textWidth) / 2;

  // 确保居中计算结果在有效范围内
  if (x < 0) x = 0;

  // 使用drawUTF8方法确保UTF-8中文字符正确显示
  display.drawUTF8(x, y, text.c_str());
}

void DisplayManager::drawRightAlignedText(const String& text, int x, int y, int textSize) {
  // textSize参数在u8g2中通过字体设置，这里忽略
  int w = display.getStrWidth(text.c_str());
  display.drawStr(x - w, y, text.c_str());
}

void DisplayManager::drawFrame(int x, int y, int width, int height) {
  display.drawFrame(x, y, width, height);
}

void DisplayManager::updateAnimation() {
  unsigned long currentTime = millis();
  if (currentTime - lastAnimationUpdate > 500) { // 500ms动画间隔
    animationFrame = (animationFrame + 1) % 4;
    lastAnimationUpdate = currentTime;
  }
}

void DisplayManager::drawLoadingAnimation(int x, int y) {
  const char* frames[] = {"|", "/", "-", "\\"};
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(x, y, frames[animationFrame]);
}

void DisplayManager::drawBreakWarning() {
  // 闪烁警告
  if ((millis() / 250) % 2 == 0) {
    display.setFont(u8g2_font_wqy12_t_chinese3);
    drawCenteredText("-- 破定提醒 --", 35, 1);
  }
}

String DisplayManager::formatTime(unsigned long timeMs, bool showSeconds) {
  unsigned long totalSeconds = timeMs / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  String result = "";

  if (hours > 0) {
    result += String(hours) + "h ";
  }

  if (minutes > 0 || hours > 0) {
    if (hours > 0 && minutes < 10) {
      result += "0";
    }
    result += String(minutes) + "m";
  }

  if (showSeconds && hours == 0) {
    if (minutes > 0) {
      result += " ";
    }
    if (minutes > 0 && seconds < 10) {
      result += "0";
    }
    result += String(seconds) + "s";
  }

  if (result.length() == 0) {
    result = showSeconds ? "0s" : "0m";
  }

  return result;
}

String DisplayManager::formatScore(float score) {
  if (score < 0) {
    return "--";
  }
  return String((int)score);
}

String DisplayManager::formatDate(uint16_t year, uint8_t month, uint8_t day) {
  return String(year) + "/" +
         (month < 10 ? "0" : "") + String(month) + "/" +
         (day < 10 ? "0" : "") + String(day);
}

void DisplayManager::printDisplayInfo() const {
  DEBUG_PRINTLN("=== 显示管理器信息 ===");
  DEBUG_PRINTF("初始化状态: %s\n", isInitialized ? "已初始化" : "未初始化");
  DEBUG_PRINTF("当前页面: %d\n", currentPage);
  DEBUG_PRINTF("显示状态: %s\n", displayData.isOn ? "开启" : "关闭");
  DEBUG_PRINTF("亮度: %d\n", displayData.brightness);
  DEBUG_PRINTF("需要更新: %s\n", needsUpdate ? "是" : "否");
  DEBUG_PRINTF("最后更新: %lu ms\n", lastUpdate);
}

bool DisplayManager::hasError() const {
  return !isInitialized;
}

String DisplayManager::getErrorMessage() const {
  if (!isInitialized) {
    return "显示屏未初始化";
  }
  return "";
}

// ==================== 开机动画管理 ====================

void DisplayManager::startBootAnimation() {
  bootAnimationActive = true;
  bootAnimationStartTime = millis();
  bootAnimationFrame = 0;
  currentPage = PAGE_BOOT_ANIMATION;
  needsUpdate = true;
  DEBUG_INFO("DISPLAY", "开机动画已启动");
}

bool DisplayManager::isBootAnimationActive() const {
  return bootAnimationActive;
}

bool DisplayManager::isBootAnimationComplete() const {
  if (!bootAnimationActive) return true;
  return (millis() - bootAnimationStartTime) >= BOOT_ANIMATION_DURATION;
}

// ==================== 主菜单管理 ====================

void DisplayManager::setMenuOption(MainMenuOption option) {
  if (currentMenuOption != option) {
    currentMenuOption = option; 
    lastMenuScroll = millis();
    needsUpdate = true;
    DEBUG_DEBUG("DISPLAY", "菜单选项切换到: %d", option);
  }
}

MainMenuOption DisplayManager::getCurrentMenuOption() const {
  return currentMenuOption;
}

void DisplayManager::nextMenuOption() {
  MainMenuOption nextOption = static_cast<MainMenuOption>((currentMenuOption + 1) % MENU_OPTION_COUNT);
  setMenuOption(nextOption);
}

void DisplayManager::previousMenuOption() {
  MainMenuOption prevOption = static_cast<MainMenuOption>((currentMenuOption + MENU_OPTION_COUNT - 1) % MENU_OPTION_COUNT);
  setMenuOption(prevOption);
}

MainMenuOption DisplayManager::getSelectedMenuOption() const {
  return currentMenuOption;
}

// ==================== 页面绘制方法 ====================

void DisplayManager::drawBootAnimationPage(const ZenMotionData& data) {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - bootAnimationStartTime;

  // 更新动画帧
  if (currentTime - lastAnimationUpdate > BOOT_ANIMATION_FRAME_DELAY) {
    bootAnimationFrame = (bootAnimationFrame + 1) % BOOT_ANIMATION_FRAMES;
    lastAnimationUpdate = currentTime;
  }

  // 检查动画是否完成
  if (elapsed >= BOOT_ANIMATION_DURATION) {
    bootAnimationActive = false;
    // 不在这里切换页面，由主程序统一管理状态
    DEBUG_INFO("DISPLAY", "开机动画完成");
    return;
  }

  // 计算进度百分比，确保范围在0-100之间
  int progress = (elapsed * 100) / BOOT_ANIMATION_DURATION;
  if (progress > 100) progress = 100;
  if (progress < 0) progress = 0;

  // ==================== 精确居中布局绘制 ====================

  // 1. 中文主标题 - 精确水平居中计算
  display.setFont(u8g2_font_wqy12_t_gb2312);
  String title = "气定神闲仪";
  // 必须在设置字体后立即计算文本宽度
  // 使用getUTF8Width计算中文文本宽度
  int titleWidth = display.getUTF8Width(title.c_str());
  int titleX = (SCREEN_WIDTH - titleWidth) / 2;
  
  // 确保居中计算结果在有效范围内
  if (titleX < 0) titleX = 0;
  if (titleX + titleWidth > SCREEN_WIDTH) titleX = SCREEN_WIDTH - titleWidth;
  
  // 使用drawUTF8方法绘制中文字符
  display.drawUTF8(titleX, BOOT_TITLE_Y, title.c_str());

  // 2. 英文副标题 - 精确水平居中计算
  display.setFont(u8g2_font_6x10_tf);
  String subtitle = "Zen-Motion Meter";
  // 必须在设置字体后立即计算文本宽度
  int subtitleWidth = display.getStrWidth(subtitle.c_str());
  int subtitleX = (SCREEN_WIDTH - subtitleWidth) / 2;
  
  // 确保居中计算结果在有效范围内
  if (subtitleX < 0) subtitleX = 0;
  if (subtitleX + subtitleWidth > SCREEN_WIDTH) subtitleX = SCREEN_WIDTH - subtitleWidth;
  
  // 使用drawStr方法绘制英文字符
  display.drawStr(subtitleX, BOOT_SUBTITLE_Y, subtitle.c_str());

  // 3. 版本信息 - 精确水平居中计算
  // 保持当前字体设置u8g2_font_6x10_tf
  String version = "v" + String(PROJECT_VERSION);
  // 必须在设置字体后立即计算文本宽度
  int versionWidth = display.getStrWidth(version.c_str());
  int versionX = (SCREEN_WIDTH - versionWidth) / 2;
  // 确保居中计算结果在有效范围内
  if (versionX < 0) versionX = 0;
  if (versionX + versionWidth > SCREEN_WIDTH) versionX = SCREEN_WIDTH - versionWidth;
  // 使用drawStr方法绘制版本信息
  display.drawStr(versionX, BOOT_VERSION_Y, version.c_str());

  // 4. 进度条 - 精确水平居中
  int progressBarX = (SCREEN_WIDTH - BOOT_PROGRESS_BAR_WIDTH) / 2;

  // 绘制进度条边框
  display.drawFrame(progressBarX, BOOT_PROGRESS_BAR_Y,
                   BOOT_PROGRESS_BAR_WIDTH, BOOT_PROGRESS_BAR_HEIGHT);

  // 绘制进度条填充
  int fillWidth = (BOOT_PROGRESS_BAR_WIDTH - 2) * progress / 100;
  if (fillWidth > 0) {
    display.drawBox(progressBarX + 1, BOOT_PROGRESS_BAR_Y + 1,
                   fillWidth, BOOT_PROGRESS_BAR_HEIGHT - 2);
  }

  // 5. 进度文本 - 动态精确水平居中（文本长度随百分比变化）
  // 先设置英文字体用于进度百分比
  display.setFont(u8g2_font_6x10_tf);
  String progressPercent = String(progress) + "%";
  int percentWidth = display.getStrWidth(progressPercent.c_str());
  
  // 计算总宽度 - “初始化中...”宽度56像素（每个中文字符12像素，半角点2像素，空格6像素）
  int chineseTextWidth = 56;  // “初始化中...” 的宽度
  int totalWidth = chineseTextWidth + percentWidth;
  int startX = (SCREEN_WIDTH - totalWidth) / 2;
  
  // 确保不超出屏幕范围
  if (startX < 0) startX = 0;
  
  // 绘制“初始化中... ” - 使用中文字体
  display.setFont(u8g2_font_wqy12_t_gb2312);
  display.drawUTF8(startX, BOOT_PROGRESS_TEXT_Y, "初始化中... ");
  
  // 绘制百分比 - 使用英文字体
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(startX + chineseTextWidth, BOOT_PROGRESS_TEXT_Y, progressPercent.c_str());

  // 6. 动画效果 - 加载指示器（闪烁点）
  if (bootAnimationFrame % 2 == 0 && progress < 100) {
    // 在进度文本右侧添加闪烁的加载指示点
    int loadingDotX = startX + totalWidth + 4;
    int loadingDotY = BOOT_PROGRESS_TEXT_Y - 3;
    // 确保装饰点不超出屏幕边界
    if (loadingDotX < SCREEN_WIDTH - 2) {
      display.drawDisc(loadingDotX, loadingDotY, BOOT_DOT_RADIUS, U8G2_DRAW_ALL);
    }
  }

  // 7. 装饰性视觉元素 - 在动画中期显示，增强视觉效果
  if (progress >= 30 && progress <= 70) {
    // 在主标题两侧添加对称的装饰点
    int leftDotX = titleX - BOOT_DOT_OFFSET;
    int rightDotX = titleX + titleWidth + BOOT_DOT_OFFSET - 2;
    int decorDotY = BOOT_TITLE_Y - 5;

    // 确保装饰点在屏幕范围内
    if (leftDotX >= BOOT_DOT_RADIUS) {
      display.drawDisc(leftDotX, decorDotY, BOOT_DOT_RADIUS, U8G2_DRAW_ALL);
    }
    if (rightDotX <= SCREEN_WIDTH - BOOT_DOT_RADIUS - 1) {
      display.drawDisc(rightDotX, decorDotY, BOOT_DOT_RADIUS, U8G2_DRAW_ALL);
    }
  }

  // 8. 完成状态指示 - 当进度达到100%时显示完成标记
  if (progress >= 100) {
    // 在进度条右侧显示完成标记
    int checkX = progressBarX + BOOT_PROGRESS_BAR_WIDTH + 4;
    int checkY = BOOT_PROGRESS_BAR_Y + 1;
    if (checkX < SCREEN_WIDTH - 3) {
      // 绘制简单的勾选标记
      display.drawPixel(checkX, checkY + 1);
      display.drawPixel(checkX + 1, checkY + 2);
      display.drawPixel(checkX + 2, checkY);
    }
  }
}

void DisplayManager::drawMainMenuPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 菜单选项文本
  const char* menuTexts[] = {
    "开始练习",
    "历史数据",
    "系统设置",
    "传感器校准"
  };

  // 绘制菜单选项
  int startY = MENU_START_Y;
  for (int i = 0; i < MENU_OPTION_COUNT; i++) {
    int itemY = startY + i * (MENU_ITEM_HEIGHT + MENU_ITEM_SPACING);

    // 如果是当前选中项，绘制高亮背景
    if (i == currentMenuOption) {
      // 绘制高亮背景
      display.drawBox(5, itemY - 9, SCREEN_WIDTH - 10, MENU_ITEM_HEIGHT);

      // 设置反色显示（黑底白字）
      display.setDrawColor(0);  // 设置为黑色（擦除模式）
      
      // 选中项也需要居中显示
      int textWidth = display.getUTF8Width(menuTexts[i]);
      int centeredX = (SCREEN_WIDTH - textWidth) / 2;
      if (centeredX < 0) centeredX = 0;
      if (centeredX + textWidth > SCREEN_WIDTH) centeredX = SCREEN_WIDTH - textWidth;
      display.setCursor(centeredX, itemY);
      display.print(menuTexts[i]);
      display.setDrawColor(1);  // 恢复白色（正常模式）
    } else {
      // 正常显示
      int textWidth = display.getUTF8Width(menuTexts[i]); // 获取菜单选项宽度用于计算居中
      int centeredX = (SCREEN_WIDTH - textWidth) / 2;
      if (centeredX < 0) centeredX = 0; // 确保不超出左边界
      if (centeredX + textWidth > SCREEN_WIDTH) centeredX = SCREEN_WIDTH - textWidth; // 确保不超出右边界
      display.setCursor(centeredX, itemY);
      display.print(menuTexts[i]);
    }
  }

  // 移除底部操作提示，以给菜单项更多空间
}

void DisplayManager::drawDateTimeEditPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_gb2312);
  
  // 标题
  drawCenteredText("日期时间设置", 12, 1);
  
  // 绘制分隔线
  display.drawHLine(0, 14, SCREEN_WIDTH);
  
  // 获取当前编辑的日期时间
  DateTime& editDT = settingsState.editingDateTime;
  
  // 显示日期时间项
  display.setFont(u8g2_font_wqy12_t_gb2312);  // 使用中文字体避免乱码
  int y = 28;
  int lineHeight = 16;  // 增加行高避免重叠
  
  // 年月日时分
  DateTimeEditItem items[] = {DATETIME_YEAR, DATETIME_MONTH, DATETIME_DAY, DATETIME_HOUR, DATETIME_MINUTE};
  const char* itemNames[] = {"年:", "月:", "日:", "时:", "分:"};
  
  for (int i = 0; i < DATETIME_ITEM_COUNT; i++) {
    DateTimeEditItem item = items[i];
    
    // 高亮当前编辑的项
    if (item == settingsState.currentDateTimeItem && settingsState.editMode) {
      display.drawBox(40, y - 8, 40, 9);
      display.setDrawColor(0);
    }
    
    // 显示项名
    display.drawUTF8(2, y, itemNames[i]);  // 使用UTF8确保中文显示正确
    
    // 显示值
    String value;
    switch (item) {
      case DATETIME_YEAR:
        value = String(editDT.year);
        break;
      case DATETIME_MONTH:
        value = String(editDT.month);
        if (editDT.month < 10) value = "0" + value;
        break;
      case DATETIME_DAY:
        value = String(editDT.day);
        if (editDT.day < 10) value = "0" + value;
        break;
      case DATETIME_HOUR:
        value = String(editDT.hour);
        if (editDT.hour < 10) value = "0" + value;
        break;
      case DATETIME_MINUTE:
        value = String(editDT.minute);
        if (editDT.minute < 10) value = "0" + value;
        break;
    }
    
    display.drawStr(45, y, value.c_str());
    
    // 恢复绘制颜色
    if (item == settingsState.currentDateTimeItem && settingsState.editMode) {
      display.setDrawColor(1);
    }
    
    // 在当前选中项旁边显示箭头
    if (item == settingsState.currentDateTimeItem) {
      display.drawStr(30, y, ">");
    }
    
    y += lineHeight;
  }
  
  // 显示完整日期时间
  display.drawHLine(0, 58, SCREEN_WIDTH);
  display.setFont(u8g2_font_6x10_tf);  // 使用英文字体显示数字和符号
  String fullDateTime = String(editDT.year) + "/" + 
                       (editDT.month < 10 ? "0" : "") + String(editDT.month) + "/" +
                       (editDT.day < 10 ? "0" : "") + String(editDT.day) + " " +
                       (editDT.hour < 10 ? "0" : "") + String(editDT.hour) + ":" +
                       (editDT.minute < 10 ? "0" : "") + String(editDT.minute);
  int fullDateTimeWidth = display.getStrWidth(fullDateTime.c_str());
  int fullDateTimeX = (SCREEN_WIDTH - fullDateTimeWidth) / 2;
  if (fullDateTimeX < 0) fullDateTimeX = 0;
  display.drawStr(fullDateTimeX, 62, fullDateTime.c_str());
  
  // 移除按键提示信息以避免遮挡
}

void DisplayManager::enterDateTimeEdit() {
  settingsState.inDateTimeEdit = true;
  settingsState.editMode = false;
  settingsState.currentDateTimeItem = DATETIME_YEAR;
  
  // 初始化编辑的日期时间为当前时间
  extern TimeManager* timeManager;
  if (timeManager) {
    settingsState.editingDateTime = timeManager->getCurrentDateTime();
  } else {
    // 默认时间
    settingsState.editingDateTime.year = 2024;
    settingsState.editingDateTime.month = 1;
    settingsState.editingDateTime.day = 1;
    settingsState.editingDateTime.hour = 0;
    settingsState.editingDateTime.minute = 0;
  }
  
  needsUpdate = true;
}

void DisplayManager::exitDateTimeEdit() {
  settingsState.inDateTimeEdit = false;
  settingsState.editMode = false;
  needsUpdate = true;
}

void DisplayManager::adjustDateTimeValue(bool increase) {
  if (!settingsState.inDateTimeEdit) return;
  
  // 获取方向（增加为1，减少为-1）
  int direction = increase ? 1 : -1;
  
  // 调用辅助函数调整日期时间值
  ::adjustDateTimeValue(settingsState.currentDateTimeItem, settingsState.editingDateTime, direction);
  
  needsUpdate = true;
}

void DisplayManager::adjustSettingValue(bool increase) {
  if (settingsState.inDateTimeEdit) return;  // 在日期时间编辑模式下不处理
  
  // 获取方向（增加为1，减少为-1）
  int direction = increase ? 1 : -1;
  
  // 需要获取和更新系统设置
  extern DataManager dataManager;
  SystemSettings settings = dataManager.getSettings();
  
  // 调用辅助函数调整设置值
  ::adjustSettingValue(settingsState.currentItem, settings, direction);
  
  // 保存更新后的设置
  dataManager.updateSettings(settings);
  
  needsUpdate = true;
}
