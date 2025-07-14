#include "time_manager.h"

// 构造函数
TimeManager::TimeManager() {
  // 使用配置文件中的默认时间
  currentDateTime.year = DEFAULT_YEAR;
  currentDateTime.month = DEFAULT_MONTH;
  currentDateTime.day = DEFAULT_DAY;
  currentDateTime.hour = DEFAULT_HOUR;
  currentDateTime.minute = DEFAULT_MINUTE;
  currentDateTime.second = DEFAULT_SECOND;
  
  // 计算星期几（简单实现）
  currentDateTime.dayOfWeek = calculateDayOfWeek(currentDateTime.year, currentDateTime.month, currentDateTime.day);
}

// 初始化方法
bool TimeManager::initialize() {
  DEBUG_INFO("TIME", "初始化时间管理器...");
  
  // 显示时间配置信息
  TIME_CONFIG_INFO();
  
  // 初始化Time库，设置初始时间
  setTime(currentDateTime.hour, currentDateTime.minute, currentDateTime.second, 
          currentDateTime.day, currentDateTime.month, currentDateTime.year);
  
  isInitialized = true;
  DEBUG_INFO("TIME", "时间管理器已初始化，当前时间: %s", formatDateTime(currentDateTime).c_str());
  
  return true;
}

// 重置方法
void TimeManager::reset() {
  isInitialized = false;
  DEBUG_INFO("TIME", "时间管理器已重置");
}

// 更新日期时间信息
void TimeManager::updateDateTime() {
  if (timeStatus() != timeNotSet) {
    currentDateTime.year = year();
    currentDateTime.month = month();
    currentDateTime.day = day();
    currentDateTime.hour = hour();
    currentDateTime.minute = minute();
    currentDateTime.second = second();
    currentDateTime.dayOfWeek = weekday() - 1;  // Time库的weekday()从1开始
  }
}

// 获取当前日期时间
DateTime TimeManager::getCurrentDateTime() {
  updateDateTime();
  return currentDateTime;
}

// 获取当前时间戳
time_t TimeManager::getCurrentTimestamp() {
  if (timeStatus() != timeNotSet) {
    return now();
  }
  
  DEBUG_WARN("TIME", "时间库未设置，返回0");
  return 0;
}

// 判断是否为新的一天
bool TimeManager::isNewDay(const DateTime& lastDate) {
  updateDateTime();
  return currentDateTime.year != lastDate.year ||
         currentDateTime.month != lastDate.month ||
         currentDateTime.day != lastDate.day;
}

// 判断是否为同一天
bool TimeManager::isSameDay(const DateTime& date1, const DateTime& date2) {
  return date1.year == date2.year &&
         date1.month == date2.month &&
         date1.day == date2.day;
}

// 计算两个日期的天数差
int TimeManager::getDaysDifference(const DateTime& date1, const DateTime& date2) {
  tm t1 = {};
  t1.tm_year = date1.year - 1900;
  t1.tm_mon = date1.month - 1;
  t1.tm_mday = date1.day;
  
  tm t2 = {};
  t2.tm_year = date2.year - 1900;
  t2.tm_mon = date2.month - 1;
  t2.tm_mday = date2.day;
  
  time_t time1 = mktime(&t1);
  time_t time2 = mktime(&t2);
  
  return difftime(time2, time1) / (60 * 60 * 24);
}

// 格式化日期时间字符串
String TimeManager::formatDateTime(const DateTime& dt) {
  return String(dt.year) + "/" + (dt.month < 10 ? "0" : "") + String(dt.month) + "/" +
         (dt.day < 10 ? "0" : "") + String(dt.day) + " " +
         (dt.hour < 10 ? "0" : "") + String(dt.hour) + ":" +
         (dt.minute < 10 ? "0" : "") + String(dt.minute) + ":" +
         (dt.second < 10 ? "0" : "") + String(dt.second);
}

// 格式化日期字符串
String TimeManager::formatDate(const DateTime& dt) {
  return String(dt.year) + "/" + (dt.month < 10 ? "0" : "") + String(dt.month) + "/" +
         (dt.day < 10 ? "0" : "") + String(dt.day);
}

// 格式化时间字符串
String TimeManager::formatTime(const DateTime& dt) {
  return (dt.hour < 10 ? "0" : "") + String(dt.hour) + ":" +
         (dt.minute < 10 ? "0" : "") + String(dt.minute) + ":" +
         (dt.second < 10 ? "0" : "") + String(dt.second);
}

// 打印日期时间
void TimeManager::printDateTime(const DateTime& dt) {
  DEBUG_PRINTF("日期时间: %s", formatDateTime(dt).c_str());
}

// 打印状态信息
void TimeManager::printStatus() {
  DEBUG_PRINTF("时间管理器状态: %s", isInitialized ? "已初始化" : "未初始化");
  DEBUG_PRINTF("Time库状态: %s", (timeStatus() != timeNotSet) ? "已设置" : "未设置");
  printDateTime(currentDateTime);
}

// 手动设置日期时间
void TimeManager::setDateTime(const DateTime& dt) {
  // 使用Time库设置时间
  setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);
  
  // 更新内部状态
  currentDateTime = dt;
  
  DEBUG_INFO("TIME", "手动设置时间: %s", formatDateTime(dt).c_str());
}

// 调整日期（增加/减少天数）
void TimeManager::adjustDate(int days) {
  // 获取当前时间戳
  time_t current = now();
  
  // 调整天数（一天 = 86400秒）
  current += (days * 86400L);
  
  // 设置新时间
  setTime(current);
  
  // 更新内部状态
  currentDateTime = getCurrentDateTime();
  
  DEBUG_INFO("TIME", "调整日期 %d 天，新日期: %s", days, formatDate(currentDateTime).c_str());
}

// 调整时间（增加/减少小时和分钟）
void TimeManager::adjustTime(int hours, int minutes) {
  // 获取当前时间戳
  time_t current = now();
  
  // 调整时间
  current += (hours * 3600L) + (minutes * 60L);
  
  // 设置新时间
  setTime(current);
  
  // 更新内部状态
  currentDateTime = getCurrentDateTime();
  
  DEBUG_INFO("TIME", "调整时间 %d 小时 %d 分钟，新时间: %s", 
             hours, minutes, formatTime(currentDateTime).c_str());
}

// 计算星期几（使用Zeller公式）
uint8_t TimeManager::calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  if (month < 3) {
    month += 12;
    year--;
  }
  
  uint8_t century = year / 100;
  uint8_t yearOfCentury = year % 100;
  
  uint8_t dayOfWeek = (day + (13 * (month + 1)) / 5 + yearOfCentury + 
                       yearOfCentury / 4 + century / 4 - 2 * century) % 7;
  
  // 转换为0=Sunday格式
  return (dayOfWeek + 6) % 7;
}
