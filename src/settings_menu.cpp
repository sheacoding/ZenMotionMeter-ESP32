#include "settings_menu.h"
#include "config.h"

const char* getSettingsItemText(SettingsMenuItem item) {
  switch (item) {
    case SETTINGS_STABILITY_THRESHOLD:
      return "稳定阈值";
    case SETTINGS_SOUND:
      return "声音";
    case SETTINGS_AUTO_SLEEP:
      return "自动休眠";
    case SETTINGS_PRACTICE_TIME:
      return "练习时长";
    case SETTINGS_DATE_TIME:
      return "日期时间";
    default:
      return "未知";
  }
}

const char* getDateTimeItemText(DateTimeEditItem item) {
  switch (item) {
    case DATETIME_YEAR:
      return "年";
    case DATETIME_MONTH:
      return "月";
    case DATETIME_DAY:
      return "日";
    case DATETIME_HOUR:
      return "时";
    case DATETIME_MINUTE:
      return "分";
    default:
      return "未知";
  }
}

String getSettingsValueText(SettingsMenuItem item, const SystemSettings& settings) {
  switch (item) {
    case SETTINGS_STABILITY_THRESHOLD:
      return String(settings.stabilityThreshold);
    case SETTINGS_SOUND:
      return settings.soundEnabled ? "开启" : "关闭";
    case SETTINGS_AUTO_SLEEP:
      return settings.autoSleep ? "开启" : "关闭";
    case SETTINGS_PRACTICE_TIME:
      return String(settings.practiceTime / 60000) + "分钟";
    default:
      return "";
  }
}

String getDateTimeValueText(DateTimeEditItem item, const DateTime& dt) {
  switch (item) {
    case DATETIME_YEAR:
      return String(dt.year);
    case DATETIME_MONTH:
      return String(dt.month);
    case DATETIME_DAY:
      return String(dt.day);
    case DATETIME_HOUR:
      return String(dt.hour);
    case DATETIME_MINUTE:
      return String(dt.minute);
    default:
      return "";
  }
}

void adjustSettingValue(SettingsMenuItem item, SystemSettings& settings, int direction) {
  switch (item) {
    case SETTINGS_STABILITY_THRESHOLD:
      settings.stabilityThreshold += direction;
      if (settings.stabilityThreshold < 0) settings.stabilityThreshold = 0;
      break;
    case SETTINGS_SOUND:
      settings.soundEnabled = !settings.soundEnabled;
      break;
    case SETTINGS_AUTO_SLEEP:
      settings.autoSleep = !settings.autoSleep;
      break;
    case SETTINGS_PRACTICE_TIME:
      settings.practiceTime += direction * 60000;
      if (settings.practiceTime < 0) settings.practiceTime = 0;
      break;
    default:
      break;
  }
}

void adjustDateTimeValue(DateTimeEditItem item, DateTime& dt, int direction) {
  switch (item) {
    case DATETIME_YEAR:
      dt.year += direction;
      break;
    case DATETIME_MONTH:
      dt.month += direction;
      if (dt.month > 12) dt.month = 1;
      if (dt.month < 1) dt.month = 12;
      break;
    case DATETIME_DAY:
      dt.day += direction;
      if (dt.day < 1) dt.day = getDateTimeMaxValue(DATETIME_DAY, dt);
      if (dt.day > getDateTimeMaxValue(DATETIME_DAY, dt)) dt.day = 1;
      break;
    case DATETIME_HOUR:
      dt.hour += direction;
      if (dt.hour >= 24) dt.hour = 0;
      if (dt.hour < 0) dt.hour = 23;
      break;
    case DATETIME_MINUTE:
      dt.minute += direction;
      if (dt.minute >= 60) dt.minute = 0;
      if (dt.minute < 0) dt.minute = 59;
      break;
    default:
      break;
  }
}

int getDateTimeMinValue(DateTimeEditItem item) {
  switch (item) {
    case DATETIME_YEAR:
      return 2000; // 假设最小年份为2000
    case DATETIME_MONTH:
      return 1;
    case DATETIME_DAY:
      return 1;
    case DATETIME_HOUR:
      return 0;
    case DATETIME_MINUTE:
      return 0;
    default:
      return 0;
  }
}

int getDateTimeMaxValue(DateTimeEditItem item, const DateTime& dt) {
  switch (item) {
    case DATETIME_YEAR:
      return 2099; // 假设最大年份为2099
    case DATETIME_MONTH:
      return 12;
    case DATETIME_DAY:
      if (dt.month == 2) {
        return (dt.year % 4 == 0 && (dt.year % 100 != 0 || dt.year % 400 == 0)) ? 29 : 28;
      }
      if (dt.month == 4 || dt.month == 6 || dt.month == 9 || dt.month == 11) {
        return 30;
      }
      return 31;
    case DATETIME_HOUR:
      return 23;
    case DATETIME_MINUTE:
      return 59;
    default:
      return 0;
  }
}

// 更新设置菜单滚动状态
void updateSettingsMenuScroll(SettingsMenuState& settingsState) {
  int currentIndex = (int)settingsState.currentItem;
  
  // 计算最大滚动偏移量
  int maxScrollOffset = SETTINGS_ITEM_COUNT - settingsState.maxVisibleItems;
  if (maxScrollOffset < 0) maxScrollOffset = 0;
  
  // 计算当前项是否在可见范围内
  int visibleStart = settingsState.scrollOffset;
  int visibleEnd = settingsState.scrollOffset + settingsState.maxVisibleItems - 1;
  
  // 如果当前项在可见范围之上，向上滚动
  if (currentIndex < visibleStart) {
    settingsState.scrollOffset = currentIndex;
  }
  // 如果当前项在可见范围之下，向下滚动
  else if (currentIndex > visibleEnd) {
    settingsState.scrollOffset = currentIndex - settingsState.maxVisibleItems + 1;
  }
  
  // 确保滚动偏移量在有效范围内
  if (settingsState.scrollOffset < 0) {
    settingsState.scrollOffset = 0;
  }
  if (settingsState.scrollOffset > maxScrollOffset) {
    settingsState.scrollOffset = maxScrollOffset;
  }
}

