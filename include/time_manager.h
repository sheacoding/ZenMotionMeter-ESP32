#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include <TimeLib.h>  // Time library for time keeping
#include "config.h"

// 日期时间结构
struct DateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t dayOfWeek;  // 0 = Sunday, 6 = Saturday
};

class TimeManager {
private:
  bool isInitialized = false;
  DateTime currentDateTime;
  
  // 内部方法
  void updateDateTime();
  uint8_t calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day);
  
public:
  TimeManager();
  
  // 初始化
  bool initialize();
  void reset();
  
  // 获取当前时间
  DateTime getCurrentDateTime();
  time_t getCurrentTimestamp();
  
  // 手动设置时间
  void setDateTime(const DateTime& dt);
  void adjustDate(int days);  // 调整天数（正数为增加，负数为减少）
  void adjustTime(int hours, int minutes = 0);  // 调整时间
  
  // 日期比较
  bool isNewDay(const DateTime& lastDate);
  bool isSameDay(const DateTime& date1, const DateTime& date2);
  int getDaysDifference(const DateTime& date1, const DateTime& date2);
  
  // 格式化输出
  String formatDateTime(const DateTime& dt);
  String formatDate(const DateTime& dt);
  String formatTime(const DateTime& dt);
  
  // 调试信息
  void printDateTime(const DateTime& dt);
  void printStatus();
};

#endif // TIME_MANAGER_H
