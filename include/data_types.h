#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <Arduino.h>

// ==================== 传感器数据结构 ====================
struct SensorData {
  float accelX, accelY, accelZ;    // 加速度数据 (g)
  float gyroX, gyroY, gyroZ;       // 陀螺仪数据 (°/s)
  float temperature;               // 温度数据 (°C)
  unsigned long timestamp;         // 时间戳 (ms)
};

// ==================== 稳定性数据结构 ====================
struct StabilityData {
  float score;                     // 当前稳定性评分 (0-100)
  float avgScore;                  // 平均稳定性评分
  float variance;                  // 方差
  float acceleration_magnitude;    // 加速度幅值
  float gyro_magnitude;           // 角速度幅值
  bool isStable;                  // 是否稳定
  unsigned long lastBreakTime;    // 上次破定时间
  int breakCount;                 // 破定次数
};

// ==================== 练习会话数据结构 ====================
struct PracticeSession {
  unsigned long startTime;         // 开始时间 (ms)
  unsigned long endTime;           // 结束时间 (ms)
  unsigned long duration;          // 持续时间 (ms)
  float avgStability;              // 平均稳定性
  float maxStability;              // 最高稳定性
  float minStability;              // 最低稳定性
  int breakCount;                  // 破定次数
  bool completed;                  // 是否完成
};

// ==================== 每日统计数据结构 ====================
struct DailyStats {
  uint16_t year;                   // 年份
  uint8_t month;                   // 月份
  uint8_t day;                     // 日期
  unsigned long totalTime;         // 总练习时间 (ms)
  int sessionCount;                // 练习次数
  float avgStability;              // 平均稳定性
  float bestStability;             // 最佳稳定性
  int totalBreaks;                 // 总破定次数
};

// ==================== 系统设置数据结构 ====================
struct SystemSettings {
  float stabilityThreshold;        // 稳定性阈值
  bool soundEnabled;               // 声音开关
  uint8_t displayBrightness;       // 显示亮度 (0-255)
  unsigned long practiceTime;      // 默认练习时间 (ms)
  bool autoSleep;                  // 自动休眠开关
  unsigned long sleepTimeout;      // 休眠超时时间 (ms)
  bool calibrationEnabled;         // 自动校准开关
  uint8_t language;                // 语言设置 (0: 中文, 1: 英文)
};

// ==================== 校准数据结构 ====================
struct CalibrationData {
  float accelOffsetX, accelOffsetY, accelOffsetZ;  // 加速度偏移
  float gyroOffsetX, gyroOffsetY, gyroOffsetZ;     // 陀螺仪偏移
  bool isCalibrated;                               // 是否已校准
  unsigned long calibrationTime;                   // 校准时间
};

// ==================== 系统状态数据结构 ====================
struct SystemStatus {
  SystemState currentState;        // 当前系统状态
  DisplayPage currentPage;         // 当前显示页面
  unsigned long lastActivity;      // 最后活动时间
  float batteryVoltage;            // 电池电压
  bool isCharging;                 // 是否在充电
  bool lowBattery;                 // 低电量标志
  unsigned long uptime;            // 运行时间
  bool sensorError;                // 传感器错误标志
  bool displayError;               // 显示错误标志
};

// ==================== 按钮事件数据结构 ====================
struct ButtonEvent {
  ButtonState state;               // 按钮状态
  unsigned long pressTime;         // 按下时间
  unsigned long releaseTime;       // 释放时间
  unsigned long duration;          // 按下持续时间
  bool processed;                  // 是否已处理
};

// ==================== 显示数据结构 ====================
struct DisplayData {
  String line1, line2, line3, line4;  // 显示行内容
  bool needUpdate;                     // 是否需要更新
  unsigned long lastUpdate;            // 最后更新时间
  uint8_t brightness;                  // 亮度
  bool isOn;                          // 是否开启
};

// ==================== 音频数据结构 ====================
struct AudioData {
  bool enabled;                    // 音频开关
  uint16_t frequency;              // 频率
  uint16_t duration;               // 持续时间
  bool isPlaying;                  // 是否正在播放
  unsigned long startTime;         // 开始时间
};

// ==================== 全局数据容器 ====================
struct ZenMotionData {
  SensorData sensor;               // 传感器数据
  StabilityData stability;         // 稳定性数据
  PracticeSession currentSession;  // 当前练习会话
  DailyStats todayStats;          // 今日统计
  SystemSettings settings;         // 系统设置
  CalibrationData calibration;     // 校准数据
  SystemStatus status;             // 系统状态
  ButtonEvent button;              // 按钮事件
  DisplayData display;             // 显示数据
  AudioData audio;                 // 音频数据
};

// ==================== 常用宏定义 ====================
#define INVALID_SCORE -1.0f
#define MAX_STABILITY_SCORE 100.0f
#define MIN_STABILITY_SCORE 0.0f

// 时间转换宏
#define MS_TO_SECONDS(ms) ((ms) / 1000)
#define MS_TO_MINUTES(ms) ((ms) / 60000)
#define SECONDS_TO_MS(s) ((s) * 1000)
#define MINUTES_TO_MS(m) ((m) * 60000)

// 数据有效性检查宏
#define IS_VALID_SCORE(score) ((score) >= MIN_STABILITY_SCORE && (score) <= MAX_STABILITY_SCORE)
#define IS_SENSOR_DATA_VALID(data) ((data).timestamp > 0)

#endif // DATA_TYPES_H
