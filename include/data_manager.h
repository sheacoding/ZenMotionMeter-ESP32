#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "data_types.h"

class DataManager {
private:
  // 当前会话数据
  PracticeSession currentSession;
  
  // 统计数据
  DailyStats todayStats;
  DailyStats historyStats[MAX_HISTORY_DAYS];
  
  // 系统设置
  SystemSettings settings;
  
  // 数据保存相关
  unsigned long lastSaveTime = 0;
  bool dataChanged = false;
  
  // 内部方法
  void initializeDefaultSettings();
  void updateTodayStats();
  void saveToEEPROM();
  void loadFromEEPROM();
  bool isNewDay();
  void rotateHistoryData();
  uint8_t getCurrentDayOfWeek();
  void calculateDailyStats();
  
public:
  DataManager();
  
  // 初始化
  bool initialize();
  void reset();
  
  // 会话管理
  void startSession();
  void pauseSession();
  void resumeSession();
  void stopSession();
  bool isSessionActive() const;
  bool isSessionPaused() const;
  
  // 会话数据
  PracticeSession getCurrentSession() const;
  unsigned long getSessionDuration() const;
  void updateSessionStability(float score);
  
  // 统计数据
  DailyStats getTodayStats() const;
  DailyStats getHistoryStats(int daysAgo) const;
  void addBreakEvent();
  
  // 设置管理
  SystemSettings getSettings() const;
  void updateSettings(const SystemSettings& newSettings);
  void resetSettings();
  
  // 数据持久化
  void saveData();
  void loadData();
  bool needsSave() const;
  void forceSave();
  
  // 数据查询
  unsigned long getTotalPracticeTime() const;
  int getTotalSessions() const;
  float getAverageStability() const;
  float getBestStability() const;
  int getTotalBreaks() const;
  
  // 历史数据
  void getWeeklyStats(unsigned long& totalTime, int& totalSessions, float& avgStability);
  void getMonthlyStats(unsigned long& totalTime, int& totalSessions, float& avgStability);
  
  // 数据导出/导入
  String exportData() const;
  bool importData(const String& data);
  
  // 调试功能
  void printSessionInfo() const;
  void printTodayStats() const;
  void printSettings() const;
  void printAllData() const;
};

#endif // DATA_MANAGER_H
