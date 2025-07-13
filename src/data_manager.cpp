#include "data_manager.h"
#include <time.h>

DataManager::DataManager() {
  // 初始化当前会话
  currentSession.startTime = 0;
  currentSession.endTime = 0;
  currentSession.duration = 0;
  currentSession.avgStability = 0.0;
  currentSession.maxStability = 0.0;
  currentSession.minStability = 100.0;
  currentSession.breakCount = 0;
  currentSession.completed = false;
  
  // 初始化今日统计
  todayStats.year = 0;
  todayStats.month = 0;
  todayStats.day = 0;
  todayStats.totalTime = 0;
  todayStats.sessionCount = 0;
  todayStats.avgStability = 0.0;
  todayStats.bestStability = 0.0;
  todayStats.totalBreaks = 0;
  
  // 初始化历史数据
  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    historyStats[i] = todayStats;
  }
}

bool DataManager::initialize(TimeManager* tm) {
  // 初始化EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // 加载数据
  loadData();
  
  // 设置时间管理器
  timeManager = tm;
  if (timeManager) {
    lastCheckDate = timeManager->getCurrentDateTime();
    // 更新今日日期
    todayStats.year = lastCheckDate.year;
    todayStats.month = lastCheckDate.month;
    todayStats.day = lastCheckDate.day;
  }
  
  // 检查并更新日期
  checkAndUpdateDate();
  
  // 初始化默认设置
  if (settings.stabilityThreshold == 0) {
    initializeDefaultSettings();
  }
  
  DEBUG_PRINTLN("数据管理器初始化成功!");
  return true;
}

void DataManager::reset() {
  // 重置当前会话
  currentSession.startTime = 0;
  currentSession.endTime = 0;
  currentSession.duration = 0;
  currentSession.avgStability = 0.0;
  currentSession.maxStability = 0.0;
  currentSession.minStability = 100.0;
  currentSession.breakCount = 0;
  currentSession.completed = false;
  
  dataChanged = true;
}

void DataManager::initializeDefaultSettings() {
  settings.stabilityThreshold = STABILITY_THRESHOLD;
  settings.soundEnabled = true;
  settings.displayBrightness = 255;
  settings.practiceTime = DEFAULT_PRACTICE_TIME;
  settings.autoSleep = true;
  settings.sleepTimeout = SLEEP_TIMEOUT;
  settings.calibrationEnabled = true;
  settings.language = 0; // 中文
  
  dataChanged = true;
  DEBUG_PRINTLN("使用默认设置");
}

void DataManager::startSession() {
  if (isSessionActive()) {
    return; // 会话已经开始
  }
  
  currentSession.startTime = millis();
  currentSession.endTime = 0;
  currentSession.duration = 0;
  currentSession.avgStability = 0.0;
  currentSession.maxStability = 0.0;
  currentSession.minStability = 100.0;
  currentSession.breakCount = 0;
  currentSession.completed = false;
  
  dataChanged = true;
  DEBUG_PRINTLN("练习会话开始");
}

void DataManager::pauseSession() {
  if (!isSessionActive() || isSessionPaused()) {
    return;
  }
  
  currentSession.endTime = millis();
  currentSession.duration += (currentSession.endTime - currentSession.startTime);
  
  DEBUG_PRINTLN("练习会话暂停");
}

void DataManager::resumeSession() {
  if (!isSessionPaused()) {
    return;
  }
  
  currentSession.startTime = millis();
  currentSession.endTime = 0;
  
  DEBUG_PRINTLN("练习会话恢复");
}

void DataManager::stopSession() {
  if (!isSessionActive()) {
    return;
  }
  
  // 计算最终持续时间
  if (currentSession.endTime == 0) {
    currentSession.endTime = millis();
    currentSession.duration += (currentSession.endTime - currentSession.startTime);
  }
  
  currentSession.completed = true;
  
  // 更新今日统计
  updateTodayStats();
  
  dataChanged = true;
  DEBUG_PRINTF("练习会话结束，持续时间: %lu ms\n", currentSession.duration);
}

bool DataManager::isSessionActive() const {
  return currentSession.startTime > 0 && !currentSession.completed;
}

bool DataManager::isSessionPaused() const {
  return isSessionActive() && currentSession.endTime > 0;
}

PracticeSession DataManager::getCurrentSession() const {
  return currentSession;
}

unsigned long DataManager::getSessionDuration() const {
  if (!isSessionActive()) {
    return currentSession.duration;
  }
  
  if (isSessionPaused()) {
    return currentSession.duration;
  }
  
  // 会话进行中，计算实时持续时间
  return currentSession.duration + (millis() - currentSession.startTime);
}

void DataManager::updateSessionStability(float score) {
  if (!isSessionActive() || isSessionPaused()) {
    return;
  }
  
  // 更新最大和最小稳定性
  if (score > currentSession.maxStability) {
    currentSession.maxStability = score;
  }
  if (score < currentSession.minStability) {
    currentSession.minStability = score;
  }
  
  // 这里可以实现更复杂的平均值计算
  // 简单起见，使用当前值作为平均值的近似
  currentSession.avgStability = score;
}

DailyStats DataManager::getTodayStats() const {
  return todayStats;
}

DailyStats DataManager::getHistoryStats(int daysAgo) const {
  if (daysAgo < 0 || daysAgo >= MAX_HISTORY_DAYS) {
    return DailyStats(); // 返回空统计
  }
  return historyStats[daysAgo];
}

void DataManager::addBreakEvent() {
  if (isSessionActive()) {
    currentSession.breakCount++;
  }
  todayStats.totalBreaks++;
  dataChanged = true;
}

SystemSettings DataManager::getSettings() const {
  return settings;
}

void DataManager::updateSettings(const SystemSettings& newSettings) {
  settings = newSettings;
  dataChanged = true;
  DEBUG_PRINTLN("设置已更新");
}

void DataManager::resetSettings() {
  initializeDefaultSettings();
  DEBUG_PRINTLN("设置已重置为默认值");
}

void DataManager::updateTodayStats() {
  // 更新会话计数
  todayStats.sessionCount++;
  
  // 更新总时间
  todayStats.totalTime += currentSession.duration;
  
  // 更新平均稳定性
  if (todayStats.sessionCount == 1) {
    todayStats.avgStability = currentSession.avgStability;
  } else {
    todayStats.avgStability = (todayStats.avgStability * (todayStats.sessionCount - 1) + 
                              currentSession.avgStability) / todayStats.sessionCount;
  }
  
  // 更新最佳稳定性
  if (currentSession.maxStability > todayStats.bestStability) {
    todayStats.bestStability = currentSession.maxStability;
  }
  
  // 更新破定次数
  todayStats.totalBreaks += currentSession.breakCount;
  
  dataChanged = true;
}

void DataManager::saveData() {
  if (!dataChanged && (millis() - lastSaveTime) < DATA_SAVE_INTERVAL) {
    return;
  }
  
  saveToEEPROM();
  lastSaveTime = millis();
  dataChanged = false;
  
  DEBUG_PRINTLN("数据已保存到EEPROM");
}

void DataManager::loadData() {
  loadFromEEPROM();
  DEBUG_PRINTLN("数据已从EEPROM加载");
}

bool DataManager::needsSave() const {
  return dataChanged || (millis() - lastSaveTime) > DATA_SAVE_INTERVAL;
}

void DataManager::forceSave() {
  saveToEEPROM();
  lastSaveTime = millis();
  dataChanged = false;
  DEBUG_PRINTLN("强制保存数据完成");
}

void DataManager::saveToEEPROM() {
  // 保存今日统计数据
  EEPROM.put(EEPROM_TOTAL_TIME_ADDR, todayStats.totalTime);
  EEPROM.put(EEPROM_SESSION_COUNT_ADDR, todayStats.sessionCount);
  EEPROM.put(EEPROM_BEST_SCORE_ADDR, todayStats.bestStability);

  // 保存设置数据
  EEPROM.put(EEPROM_SETTINGS_ADDR, settings);

  // 保存今日统计完整数据
  EEPROM.put(EEPROM_SETTINGS_ADDR + sizeof(SystemSettings), todayStats);

  // 保存历史数据
  int historyAddr = EEPROM_SETTINGS_ADDR + sizeof(SystemSettings) + sizeof(DailyStats);
  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    EEPROM.put(historyAddr + i * sizeof(DailyStats), historyStats[i]);
  }

  // 写入有效性标志
  EEPROM.write(EEPROM_SIZE - 1, 0xAA);

  EEPROM.commit();
}

void DataManager::loadFromEEPROM() {
  // 检查数据有效性
  if (EEPROM.read(EEPROM_SIZE - 1) != 0xAA) {
    DEBUG_PRINTLN("EEPROM数据无效，使用默认值");
    initializeDefaultSettings();
    return;
  }

  // 加载基本统计数据
  EEPROM.get(EEPROM_TOTAL_TIME_ADDR, todayStats.totalTime);
  EEPROM.get(EEPROM_SESSION_COUNT_ADDR, todayStats.sessionCount);
  EEPROM.get(EEPROM_BEST_SCORE_ADDR, todayStats.bestStability);

  // 加载设置数据
  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);

  // 加载今日统计完整数据
  EEPROM.get(EEPROM_SETTINGS_ADDR + sizeof(SystemSettings), todayStats);

  // 加载历史数据
  int historyAddr = EEPROM_SETTINGS_ADDR + sizeof(SystemSettings) + sizeof(DailyStats);
  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    EEPROM.get(historyAddr + i * sizeof(DailyStats), historyStats[i]);
  }
}

bool DataManager::checkAndUpdateDate() {
  if (!timeManager) {
    // 没有时间管理器，使用简单的时间检查
    static unsigned long lastDayCheck = 0;
    unsigned long currentTime = millis();
    
    // 如果超过24小时，认为是新的一天
    if (currentTime - lastDayCheck > 86400000) { // 24小时 = 86400000ms
      lastDayCheck = currentTime;
      rotateHistoryData();
      return true;
    }
    return false;
  }
  
  // 使用时间管理器检查
  DateTime currentDate = timeManager->getCurrentDateTime();
  if (timeManager->isNewDay(lastCheckDate)) {
    DEBUG_INFO("DATA_MANAGER", "检测到新的一天: %s", 
               timeManager->formatDate(currentDate).c_str());
    
    // 计算天数差
    int daysDiff = timeManager->getDaysDifference(lastCheckDate, currentDate);
    
    // 如果只过了一天，正常轮转
    if (daysDiff == 1) {
      moveTodayToHistory();
    } 
    // 如果过了多天，需要处理空缺的天数
    else if (daysDiff > 1) {
      // 先保存今天的数据
      moveTodayToHistory();
      
      // 对于中间的空缺天数，插入空数据
      for (int i = 1; i < daysDiff - 1 && i < MAX_HISTORY_DAYS; i++) {
        DailyStats emptyDay = {};
        emptyDay.year = lastCheckDate.year;
        emptyDay.month = lastCheckDate.month;
        emptyDay.day = lastCheckDate.day + i;
        
        // 将历史数据向后移动
        for (int j = MAX_HISTORY_DAYS - 1; j > 0; j--) {
          historyStats[j] = historyStats[j - 1];
        }
        historyStats[0] = emptyDay;
      }
    }
    
    // 重置今日数据
    resetTodayStats();
    todayStats.year = currentDate.year;
    todayStats.month = currentDate.month;
    todayStats.day = currentDate.day;
    
    lastCheckDate = currentDate;
    dataChanged = true;
    forceSave(); // 立即保存
    
    return true;
  }
  
  return false;
}

void DataManager::moveTodayToHistory() {
  // 将历史数据向后移动一位
  for (int i = MAX_HISTORY_DAYS - 1; i > 0; i--) {
    historyStats[i] = historyStats[i - 1];
  }
  
  // 将今日数据移到历史数据的第一位
  historyStats[0] = todayStats;
  
  DEBUG_INFO("DATA_MANAGER", "今日数据已保存到历史: %d/%d/%d, 练习%d次, 总时长%lums",
             todayStats.year, todayStats.month, todayStats.day,
             todayStats.sessionCount, todayStats.totalTime);
}

void DataManager::resetTodayStats() {
  todayStats.totalTime = 0;
  todayStats.sessionCount = 0;
  todayStats.avgStability = 0.0;
  todayStats.bestStability = 0.0;
  todayStats.totalBreaks = 0;
  // 日期信息由调用者设置
}

void DataManager::rotateHistoryData() {
  // 将历史数据向后移动一位
  for (int i = MAX_HISTORY_DAYS - 1; i > 0; i--) {
    historyStats[i] = historyStats[i - 1];
  }

  // 将今日数据移到历史数据的第一位
  historyStats[0] = todayStats;

  // 重置今日数据
  todayStats.totalTime = 0;
  todayStats.sessionCount = 0;
  todayStats.avgStability = 0.0;
  todayStats.bestStability = 0.0;
  todayStats.totalBreaks = 0;

  // 更新日期（这里简化处理）
  todayStats.year = 2024;
  todayStats.month = 1;
  todayStats.day = 1;

  dataChanged = true;
  DEBUG_PRINTLN("历史数据已轮转，新的一天开始");
}

unsigned long DataManager::getTotalPracticeTime() const {
  unsigned long total = todayStats.totalTime;
  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    total += historyStats[i].totalTime;
  }
  return total;
}

int DataManager::getTotalSessions() const {
  int total = todayStats.sessionCount;
  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    total += historyStats[i].sessionCount;
  }
  return total;
}

float DataManager::getAverageStability() const {
  float totalStability = 0.0;
  int totalSessions = 0;

  if (todayStats.sessionCount > 0) {
    totalStability += todayStats.avgStability * todayStats.sessionCount;
    totalSessions += todayStats.sessionCount;
  }

  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    if (historyStats[i].sessionCount > 0) {
      totalStability += historyStats[i].avgStability * historyStats[i].sessionCount;
      totalSessions += historyStats[i].sessionCount;
    }
  }

  return totalSessions > 0 ? totalStability / totalSessions : 0.0;
}

float DataManager::getBestStability() const {
  float best = todayStats.bestStability;
  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    if (historyStats[i].bestStability > best) {
      best = historyStats[i].bestStability;
    }
  }
  return best;
}

int DataManager::getTotalBreaks() const {
  int total = todayStats.totalBreaks;
  for (int i = 0; i < MAX_HISTORY_DAYS; i++) {
    total += historyStats[i].totalBreaks;
  }
  return total;
}

void DataManager::getWeeklyStats(unsigned long& totalTime, int& totalSessions, float& avgStability) {
  totalTime = 0;
  totalSessions = 0;
  float totalStability = 0.0;

  // 包括今天
  totalTime += todayStats.totalTime;
  totalSessions += todayStats.sessionCount;
  if (todayStats.sessionCount > 0) {
    totalStability += todayStats.avgStability * todayStats.sessionCount;
  }

  // 过去6天
  for (int i = 0; i < min(6, MAX_HISTORY_DAYS); i++) {
    totalTime += historyStats[i].totalTime;
    totalSessions += historyStats[i].sessionCount;
    if (historyStats[i].sessionCount > 0) {
      totalStability += historyStats[i].avgStability * historyStats[i].sessionCount;
    }
  }

  avgStability = totalSessions > 0 ? totalStability / totalSessions : 0.0;
}

void DataManager::printSessionInfo() const {
  DEBUG_PRINTLN("=== 当前会话信息 ===");
  DEBUG_PRINTF("会话状态: %s\n", isSessionActive() ? (isSessionPaused() ? "暂停" : "进行中") : "未开始");
  DEBUG_PRINTF("开始时间: %lu ms\n", currentSession.startTime);
  DEBUG_PRINTF("持续时间: %lu ms\n", getSessionDuration());
  DEBUG_PRINTF("平均稳定性: %.1f\n", currentSession.avgStability);
  DEBUG_PRINTF("最高稳定性: %.1f\n", currentSession.maxStability);
  DEBUG_PRINTF("最低稳定性: %.1f\n", currentSession.minStability);
  DEBUG_PRINTF("破定次数: %d\n", currentSession.breakCount);
}

void DataManager::printTodayStats() const {
  DEBUG_PRINTLN("=== 今日统计 ===");
  DEBUG_PRINTF("练习次数: %d\n", todayStats.sessionCount);
  DEBUG_PRINTF("总时长: %lu ms\n", todayStats.totalTime);
  DEBUG_PRINTF("平均稳定性: %.1f\n", todayStats.avgStability);
  DEBUG_PRINTF("最佳稳定性: %.1f\n", todayStats.bestStability);
  DEBUG_PRINTF("破定次数: %d\n", todayStats.totalBreaks);
}

void DataManager::printSettings() const {
  DEBUG_PRINTLN("=== 系统设置 ===");
  DEBUG_PRINTF("稳定性阈值: %.1f\n", settings.stabilityThreshold);
  DEBUG_PRINTF("声音开启: %s\n", settings.soundEnabled ? "是" : "否");
  DEBUG_PRINTF("显示亮度: %d\n", settings.displayBrightness);
  DEBUG_PRINTF("练习时长: %lu ms\n", settings.practiceTime);
  DEBUG_PRINTF("自动休眠: %s\n", settings.autoSleep ? "是" : "否");
}
