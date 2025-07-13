#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include <Arduino.h>
#include "config.h"
#include "data_types.h"
#include "time_manager.h"

// 设置菜单状态
struct SettingsMenuState {
  SettingsMenuItem currentItem = SETTINGS_STABILITY_THRESHOLD;
  bool inDateTimeEdit = false;
  DateTimeEditItem currentDateTimeItem = DATETIME_YEAR;
  DateTime editingDateTime;  // 正在编辑的日期时间
  bool editMode = false;     // 是否在编辑模式
  int scrollOffset = 0;      // 滚动偏移量
  static const int maxVisibleItems = 3;  // 屏幕最多显示3个项目，给滚动留出空间
};

// 设置菜单项的显示文本
const char* getSettingsItemText(SettingsMenuItem item);
const char* getDateTimeItemText(DateTimeEditItem item);

// 获取设置项的当前值文本
String getSettingsValueText(SettingsMenuItem item, const SystemSettings& settings);
String getDateTimeValueText(DateTimeEditItem item, const DateTime& dt);

// 调整设置值
void adjustSettingValue(SettingsMenuItem item, SystemSettings& settings, int direction);
void adjustDateTimeValue(DateTimeEditItem item, DateTime& dt, int direction);

// 获取日期时间的有效范围
int getDateTimeMinValue(DateTimeEditItem item);
int getDateTimeMaxValue(DateTimeEditItem item, const DateTime& dt);

// 更新设置菜单滚动状态
void updateSettingsMenuScroll(SettingsMenuState& settingsState);

#endif // SETTINGS_MENU_H
