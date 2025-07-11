#include "display_manager.h"
#include "diagnostic_utils.h"

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
  display.clearBuffer();
  display.enableUTF8Print();  // 启用UTF8支持
  display.setFont(u8g2_font_wqy12_t_gb2312);  // 使用GB2312中文字体
  display.setFontDirection(0);

  // 测试中文显示功能
  DEBUG_DEBUG("DISPLAY", "测试中文显示功能...");
  display.firstPage();
  do {
    display.setCursor(0, 15);
    display.print("测试中文");
    display.setCursor(0, 30);
    display.print("气定神闲仪");
    display.setCursor(0, 45);
    display.print("OLED Test");
  } while (display.nextPage());
  delay(2000);  // 延长显示时间以便观察

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

  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 使用firstPage/nextPage循环显示中文
  display.firstPage();
  do {
    // 计算文本位置使其居中
    int w = display.getStrWidth(message.c_str());
    int h = display.getFontAscent() - display.getFontDescent();

    int x = (SCREEN_WIDTH - w) / 2;
    int y = (SCREEN_HEIGHT + h) / 2;

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
    // 显示项目名称 (中文字体)
    display.setFont(u8g2_font_wqy12_t_gb2312);
    int w = display.getStrWidth("气定神闲仪");
    int x = (SCREEN_WIDTH - w) / 2;
    display.setCursor(x, 20);
    display.print("气定神闲仪");

    // 显示英文名称和版本 (小字体)
    display.setFont(u8g2_font_6x10_tf);
    w = display.getStrWidth("Zen-Motion Meter");
    x = (SCREEN_WIDTH - w) / 2;
    display.setCursor(x, 40);
    display.print("Zen-Motion Meter");

    String version = "v" + String(PROJECT_VERSION);
    w = display.getStrWidth(version.c_str());
    x = (SCREEN_WIDTH - w) / 2;
    display.setCursor(x, 55);
    display.print(version);
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

  // 显示百分比
  String percentText = String(percentage) + "%";
  int w = display.getStrWidth(percentText.c_str());
  display.drawStr((SCREEN_WIDTH - w) / 2, 55, percentText.c_str());

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
  // 设置大字体显示稳定性评分
  display.setFont(u8g2_font_logisoso32_tn);
  String scoreText = formatScore(data.stability.score);
  int w = display.getStrWidth(scoreText.c_str());
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 30);
  display.print(scoreText);

  // 设置中文字体显示标签
  display.setFont(u8g2_font_wqy12_t_gb2312);
  w = display.getStrWidth("稳定性评分");
  x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 45);
  display.print("稳定性评分");

  // 显示练习时长
  display.setFont(u8g2_font_6x10_tf);
  String practiceTime = "练习时长: " + formatTime(data.currentSession.duration);
  display.setCursor(0, 58);
  display.print(practiceTime);

  // 显示累计时长
  String totalTime = "累计: " + formatTime(data.todayStats.totalTime);
  int totalW = display.getStrWidth(totalTime.c_str());
  display.setCursor(SCREEN_WIDTH - totalW, 58);
  display.print(totalTime);

  // 如果不稳定，显示破定提醒
  if (!data.stability.isStable) {
    display.setFont(u8g2_font_wqy12_t_gb2312);
    w = display.getStrWidth("-- 破定提醒 --");
    x = (SCREEN_WIDTH - w) / 2;
    display.setCursor(x, 50);
    display.print("-- 破定提醒 --");
  }
}

void DisplayManager::drawStatsPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 标题
  int w = display.getStrWidth("统计信息");
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 12);
  display.print("统计信息");

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
  display.setFont(u8g2_font_wqy12_t_chinese3);

  // 标题
  drawCenteredText("设置", 12, 1);

  // 设置项
  display.setFont(u8g2_font_6x10_tf);
  String line1 = "稳定阈值: " + String((int)data.settings.stabilityThreshold);
  display.drawStr(0, 25, line1.c_str());

  String line2 = "声音: " + String(data.settings.soundEnabled ? "开启" : "关闭");
  display.drawStr(0, 35, line2.c_str());

  String line3 = "自动休眠: " + String(data.settings.autoSleep ? "开启" : "关闭");
  display.drawStr(0, 45, line3.c_str());

  String line4 = "练习时长: " + String(data.settings.practiceTime / 60000) + "分钟";
  display.drawStr(0, 55, line4.c_str());

  String line5 = "电池: " + String(data.status.batteryVoltage, 1) + "V";
  display.drawStr(0, 65, line5.c_str());
}

void DisplayManager::drawCalibrationPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_chinese3);

  // 标题
  drawCenteredText("传感器校准", 12, 1);

  display.setFont(u8g2_font_6x10_tf);
  if (data.calibration.isCalibrated) {
    display.drawStr(0, 25, "校准状态: 已完成");
    display.drawStr(0, 40, "校准时间: ");
    // 这里可以显示校准时间的格式化版本
    display.drawStr(0, 55, "按按钮重新校准");
  } else {
    display.drawStr(0, 25, "校准状态: 未校准");
    display.drawStr(0, 40, "请保持设备静止");
    display.drawStr(0, 55, "按按钮开始校准");
  }
}

void DisplayManager::drawHistoryPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_chinese3);

  // 标题
  drawCenteredText("历史记录", 12, 1);

  // 显示最近几次练习的简要信息
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(0, 25, "最近练习记录:");

  // 这里可以显示历史数据
  // 由于空间限制，只显示关键信息
  String line1 = "今日: " + formatTime(data.todayStats.totalTime);
  display.drawStr(0, 40, line1.c_str());

  String line2 = "评分: " + formatScore(data.todayStats.avgStability);
  display.drawStr(0, 50, line2.c_str());

  String line3 = "次数: " + String(data.todayStats.sessionCount);
  display.drawStr(0, 60, line3.c_str());
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

void DisplayManager::drawWiFiIcon(int x, int y, bool connected) {
  if (connected) {
    // 绘制WiFi信号图标
    display.drawPixel(x, y + 3);
    display.drawHLine(x - 1, y + 2, 3);
    display.drawHLine(x - 2, y + 1, 5);
    display.drawHLine(x - 3, y, 7);
  } else {
    // 绘制断开的WiFi图标
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(x, y + 6, "X");
  }
}

void DisplayManager::drawStatusIcons(const ZenMotionData& data) {
  // 绘制电池图标
  drawBatteryIcon(SCREEN_WIDTH - 20, 0, data.status.batteryVoltage, false);

  // 绘制WiFi图标 (如果需要)
  // drawWiFiIcon(SCREEN_WIDTH - 40, 0, false);

  // 如果有错误，显示错误图标
  if (data.status.sensorError || data.status.displayError) {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 8, "!");
  }
}

void DisplayManager::drawCenteredText(const String& text, int y, int textSize) {
  // textSize参数在u8g2中通过字体设置，这里忽略
  int w = display.getStrWidth(text.c_str());
  int x = (SCREEN_WIDTH - w) / 2;
  display.drawStr(x, y, text.c_str());
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
    setPage(PAGE_MAIN_MENU);
    DEBUG_INFO("DISPLAY", "开机动画完成，切换到主菜单");
    return;
  }

  // 绘制开机动画
  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 显示项目名称
  String title = "气定神闲仪";
  int w = display.getStrWidth(title.c_str());
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 20);
  display.print(title);

  // 显示英文名称
  display.setFont(u8g2_font_6x10_tf);
  String subtitle = "Zen-Motion Meter";
  w = display.getStrWidth(subtitle.c_str());
  x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 35);
  display.print(subtitle);

  // 显示版本信息
  String version = "v" + String(PROJECT_VERSION);
  w = display.getStrWidth(version.c_str());
  x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 48);
  display.print(version);

  // 显示初始化进度
  int progress = (elapsed * 100) / BOOT_ANIMATION_DURATION;
  String progressText = "初始化中... " + String(progress) + "%";
  w = display.getStrWidth(progressText.c_str());
  x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 60);
  display.print(progressText);

  // 绘制进度条
  int barWidth = 80;
  int barHeight = 4;
  int barX = (SCREEN_WIDTH - barWidth) / 2;
  int barY = 55;

  // 进度条边框
  display.drawFrame(barX, barY, barWidth, barHeight);

  // 进度条填充
  int fillWidth = (barWidth - 2) * progress / 100;
  if (fillWidth > 0) {
    display.drawBox(barX + 1, barY + 1, fillWidth, barHeight - 2);
  }
}

void DisplayManager::drawMainMenuPage(const ZenMotionData& data) {
  display.setFont(u8g2_font_wqy12_t_gb2312);

  // 显示标题
  String title = "主菜单";
  int w = display.getStrWidth(title.c_str());
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 12);
  display.print(title);

  // 菜单选项文本
  const char* menuTexts[] = {
    "开始练习",
    "历史数据",
    "系统设置",
    "传感器校准"
  };

  // 绘制菜单选项
  int startY = 25;
  for (int i = 0; i < MENU_OPTION_COUNT; i++) {
    int itemY = startY + i * (MENU_ITEM_HEIGHT + MENU_ITEM_SPACING);

    // 如果是当前选中项，绘制高亮背景
    if (i == currentMenuOption) {
      // 绘制高亮背景
      display.drawBox(5, itemY - 10, SCREEN_WIDTH - 10, MENU_ITEM_HEIGHT);

      // 设置反色显示（黑底白字）
      display.setDrawColor(0);  // 设置为黑色（擦除模式）
      display.setCursor(10, itemY);
      display.print(menuTexts[i]);
      display.setDrawColor(1);  // 恢复白色（正常模式）

      // 绘制选择指示器
      display.setCursor(SCREEN_WIDTH - 15, itemY);
      display.print("◄");
    } else {
      // 正常显示
      display.setCursor(10, itemY);
      display.print(menuTexts[i]);
    }
  }

  // 显示操作提示
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(5, 60);
  display.print("单击:切换 长按:确认");
}
