#ifndef CONFIG_H
#define CONFIG_H

// ==================== 项目信息 ====================
#define PROJECT_NAME "气定神闲仪 (Zen-Motion Meter)"
#define PROJECT_VERSION "1.0.0"
#define AUTHOR "Zen-Motion Team"

// ==================== 硬件引脚配置 ====================
// 智能引脚配置 - 根据开发板类型自动选择

#ifdef BOARD_ESP32_C3_SUPERMINI
  // ESP32-C3 SuperMini 引脚配置
  #ifndef I2C_SDA_PIN
    #define I2C_SDA_PIN 8
  #endif
  #ifndef I2C_SCL_PIN
    #define I2C_SCL_PIN 9
  #endif
  #ifndef BUTTON_PIN
    #define BUTTON_PIN 3
  #endif
  #ifndef BUZZER_PIN
    #define BUZZER_PIN 4
  #endif
  #ifndef LED_PIN
    #define LED_PIN 2
  #endif
  #ifndef BUTTON_PRESSED_STATE
    #define BUTTON_PRESSED_STATE HIGH    // 按下时为高电平
  #endif
  #ifndef BUTTON_RELEASED_STATE
    #define BUTTON_RELEASED_STATE LOW    // 松开时为低电平
  #endif
  #ifndef BUTTON_PIN_MODE
    #define BUTTON_PIN_MODE INPUT        // 不使用内部上拉/下拉电阻
  #endif

#elif defined(BOARD_ESP32_DEVKIT)
  // ESP32 DevKit 引脚配置
  #ifndef I2C_SDA_PIN
    #define I2C_SDA_PIN 21               // ESP32默认SDA引脚
  #endif
  #ifndef I2C_SCL_PIN
    #define I2C_SCL_PIN 22               // ESP32默认SCL引脚
  #endif
  #ifndef BUTTON_PIN
    #define BUTTON_PIN 2                 // 使用GPIO2作为按钮
  #endif
  #ifndef BUZZER_PIN
    #define BUZZER_PIN 15                // 使用GPIO15作为蜂鸣器
  #endif
  #ifndef LED_PIN
    #define LED_PIN 4                    // 使用GPIO4作为LED指示灯
  #endif
  #ifndef BUTTON_PRESSED_STATE
    #define BUTTON_PRESSED_STATE HIGH    // 按下时为高电平
  #endif
  #ifndef BUTTON_RELEASED_STATE
    #define BUTTON_RELEASED_STATE LOW    // 松开时为低电平
  #endif
  #ifndef BUTTON_PIN_MODE
    #define BUTTON_PIN_MODE INPUT        // 不使用内部上拉电阻
  #endif

#else
  // 默认配置 (兼容ESP32-C3 SuperMini)
  #ifndef I2C_SDA_PIN
    #define I2C_SDA_PIN 8
  #endif
  #ifndef I2C_SCL_PIN
    #define I2C_SCL_PIN 9
  #endif
  #ifndef BUTTON_PIN
    #define BUTTON_PIN 3
  #endif
  #ifndef BUZZER_PIN
    #define BUZZER_PIN 4
  #endif
  #ifndef LED_PIN
    #define LED_PIN 2
  #endif
  #ifndef BUTTON_PRESSED_STATE
    #define BUTTON_PRESSED_STATE HIGH
  #endif
  #ifndef BUTTON_RELEASED_STATE
    #define BUTTON_RELEASED_STATE LOW
  #endif
  #ifndef BUTTON_PIN_MODE
    #define BUTTON_PIN_MODE INPUT
  #endif
#endif

// 引脚配置验证和信息输出
#if DEBUG
  #define PRINT_PIN_CONFIG() \
    do { \
      DEBUG_INFO("CONFIG", "=== 引脚配置信息 ==="); \
      DEBUG_INFO("CONFIG", "I2C SDA: GPIO%d", I2C_SDA_PIN); \
      DEBUG_INFO("CONFIG", "I2C SCL: GPIO%d", I2C_SCL_PIN); \
      DEBUG_INFO("CONFIG", "按钮: GPIO%d (%s时触发)", BUTTON_PIN, \
        (BUTTON_PRESSED_STATE == HIGH) ? "HIGH" : "LOW"); \
      DEBUG_INFO("CONFIG", "蜂鸣器: GPIO%d", BUZZER_PIN); \
      DEBUG_INFO("CONFIG", "LED: GPIO%d", LED_PIN); \
      DEBUG_INFO("CONFIG", "按钮模式: %s", \
        (BUTTON_PIN_MODE == INPUT_PULLUP) ? "INPUT_PULLUP" : \
        (BUTTON_PIN_MODE == INPUT_PULLDOWN) ? "INPUT_PULLDOWN" : "INPUT"); \
    } while(0)
#else
  #define PRINT_PIN_CONFIG()
#endif
  
// ==================== 传感器配置 ====================
// MPU6050配置
#define MPU6050_ADDRESS 0x68
#define MPU6050_SAMPLE_RATE 100  // Hz
#define ACCEL_SCALE_FACTOR 16384.0  // ±2g
#define GYRO_SCALE_FACTOR 131.0     // ±250°/s

// 稳定性评分配置
#define STABILITY_THRESHOLD 50      // 破定提醒阈值
#define STABILITY_WINDOW_SIZE 20    // 滑动窗口大小
#define CALIBRATION_SAMPLES 100     // 校准样本数量

// ==================== 显示配置 ====================
// OLED显示屏配置
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// UI更新频率
#define DISPLAY_UPDATE_INTERVAL 100  // ms
#define SENSOR_READ_INTERVAL 50      // ms

// ==================== 开机动画配置 ====================
#define BOOT_ANIMATION_DURATION 4000  // 开机动画持续时间 (ms)
#define BOOT_ANIMATION_FRAMES 8       // 动画帧数
#define BOOT_ANIMATION_FRAME_DELAY 500 // 每帧延迟 (ms)

// ==================== 主菜单配置 ====================
#define MENU_ITEM_HEIGHT 12          // 菜单项高度
#define MENU_ITEM_SPACING 2          // 菜单项间距
#define MENU_SCROLL_DELAY 200        // 菜单滚动延迟 (ms)
#define MENU_HIGHLIGHT_WIDTH 2       // 高亮边框宽度

// ==================== 时间配置 ====================
// 计时器配置
#define DEFAULT_PRACTICE_TIME 300000  // 默认5分钟 (ms)
#define MAX_PRACTICE_TIME 3600000     // 最大1小时 (ms)
#define TIME_DISPLAY_FORMAT 1         // 0: 秒, 1: 分:秒

// 数据保存间隔
#define DATA_SAVE_INTERVAL 60000     // 1分钟保存一次

// ==================== 音频配置 ====================
// 蜂鸣器配置
#define BUZZER_FREQUENCY 2000        // Hz
#define BUZZER_DURATION 200          // ms
#define BUZZER_BREAK_FREQUENCY 1500  // 破定提醒频率
#define BUZZER_SUCCESS_FREQUENCY 2500 // 成功提醒频率

// ==================== 电源管理配置 ====================
// 休眠配置
#define SLEEP_TIMEOUT 300000         // 5分钟无操作进入休眠
#define DEEP_SLEEP_TIMEOUT 1800000   // 30分钟进入深度休眠

// 电池监测
#define BATTERY_LOW_VOLTAGE 3.3      // 低电量阈值 (V)
#define BATTERY_CRITICAL_VOLTAGE 3.0 // 严重低电量阈值 (V)

// ==================== 数据存储配置 ====================
// EEPROM地址分配
#define EEPROM_SIZE 512
#define EEPROM_TOTAL_TIME_ADDR 0     // 累计时长 (4字节)
#define EEPROM_SESSION_COUNT_ADDR 4  // 练习次数 (4字节)
#define EEPROM_BEST_SCORE_ADDR 8     // 最佳评分 (4字节)
#define EEPROM_SETTINGS_ADDR 12      // 设置数据起始地址

// 历史数据配置
#define MAX_HISTORY_DAYS 7           // 保存7天历史数据
#define DAILY_DATA_SIZE 16           // 每日数据大小 (字节)

// ==================== 调试配置 ====================
#define DEBUG 1

// 调试级别定义
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_DEBUG   4
#define DEBUG_LEVEL_VERBOSE 5

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_LEVEL_DEBUG
#endif

// 串口配置
#define DEBUG_SERIAL_SPEED 115200
#define DEBUG_BUFFER_SIZE 256

// 调试宏定义
#if DEBUG
  #define DEBUG_INIT() Serial.begin(DEBUG_SERIAL_SPEED); delay(1000)
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(format, ...) Serial.printf(format, ##__VA_ARGS__)

  // 带时间戳的调试输出
  #define DEBUG_LOG(level, module, format, ...) \
    do { \
      if (level <= DEBUG_LEVEL) { \
        Serial.printf("[%08lu][%s][%s] ", millis(), module, \
          (level == DEBUG_LEVEL_ERROR) ? "ERROR" : \
          (level == DEBUG_LEVEL_WARN) ? "WARN" : \
          (level == DEBUG_LEVEL_INFO) ? "INFO" : \
          (level == DEBUG_LEVEL_DEBUG) ? "DEBUG" : "VERBOSE"); \
        Serial.printf(format, ##__VA_ARGS__); \
        Serial.println(); \
      } \
    } while(0)

  // 模块特定的调试宏
  #define DEBUG_ERROR(module, format, ...) DEBUG_LOG(DEBUG_LEVEL_ERROR, module, format, ##__VA_ARGS__)
  #define DEBUG_WARN(module, format, ...) DEBUG_LOG(DEBUG_LEVEL_WARN, module, format, ##__VA_ARGS__)
  #define DEBUG_INFO(module, format, ...) DEBUG_LOG(DEBUG_LEVEL_INFO, module, format, ##__VA_ARGS__)
  #define DEBUG_DEBUG(module, format, ...) DEBUG_LOG(DEBUG_LEVEL_DEBUG, module, format, ##__VA_ARGS__)
  #define DEBUG_VERBOSE(module, format, ...) DEBUG_LOG(DEBUG_LEVEL_VERBOSE, module, format, ##__VA_ARGS__)

  // 内存使用情况报告
  #define DEBUG_MEMORY() \
    do { \
      DEBUG_INFO("MEMORY", "Free heap: %d bytes, Min free: %d bytes", \
        ESP.getFreeHeap(), ESP.getMinFreeHeap()); \
    } while(0)

  // 系统信息报告
  #define DEBUG_SYSTEM_INFO() \
    do { \
      DEBUG_INFO("SYSTEM", "Chip: %s, Cores: %d, Freq: %dMHz", \
        ESP.getChipModel(), ESP.getChipCores(), ESP.getCpuFreqMHz()); \
      DEBUG_INFO("SYSTEM", "Flash: %dKB, PSRAM: %dKB", \
        ESP.getFlashChipSize()/1024, ESP.getPsramSize()/1024); \
      DEBUG_MEMORY(); \
    } while(0)

#else
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(format, ...)
  #define DEBUG_LOG(level, module, format, ...)
  #define DEBUG_ERROR(module, format, ...)
  #define DEBUG_WARN(module, format, ...)
  #define DEBUG_INFO(module, format, ...)
  #define DEBUG_DEBUG(module, format, ...)
  #define DEBUG_VERBOSE(module, format, ...)
  #define DEBUG_MEMORY()
  #define DEBUG_SYSTEM_INFO()
#endif

// ==================== 硬件诊断配置 ====================
#define DIAGNOSTIC_MODE_ENABLED 1
#define I2C_SCAN_ENABLED 1
#define HARDWARE_SELF_TEST_ENABLED 1

// I2C诊断配置
#define I2C_TIMEOUT_MS 1000
#define I2C_RETRY_COUNT 3
#define I2C_CLOCK_SPEEDS_COUNT 3
extern const uint32_t I2C_CLOCK_SPEEDS[I2C_CLOCK_SPEEDS_COUNT];

// 电源监测配置
#define VOLTAGE_SAMPLES 10
#define VOLTAGE_SAMPLE_INTERVAL 100  // ms
#define VOLTAGE_MIN_NORMAL 3.0f      // V
#define VOLTAGE_MAX_NORMAL 3.6f      // V

// 错误音调配置
#define ERROR_TONE_FREQUENCY 800
#define ERROR_TONE_DURATION 200
#define ERROR_TONE_COUNT 3
#define ERROR_TONE_INTERVAL 300

// ==================== 系统状态定义 ====================
enum SystemState {
  STATE_BOOT_ANIMATION, // 开机动画状态
  STATE_MAIN_MENU,      // 主菜单状态
  STATE_IDLE,           // 空闲状态（保留兼容性）
  STATE_CALIBRATING,    // 校准状态
  STATE_PRACTICING,     // 练习状态
  STATE_PAUSED,         // 暂停状态
  STATE_MENU,           // 旧菜单状态（保留兼容性）
  STATE_SETTINGS,       // 设置状态
  STATE_HISTORY,        // 历史数据状态
  STATE_SLEEP           // 休眠状态
};

// ==================== 按钮状态定义 ====================
enum ButtonState {
  BUTTON_IDLE,
  BUTTON_PRESSED,
  BUTTON_LONG_PRESSED,
  BUTTON_DOUBLE_PRESSED
};

// ==================== 显示页面定义 ====================
enum DisplayPage {
  PAGE_BOOT_ANIMATION,  // 开机动画页面
  PAGE_MAIN_MENU,       // 主菜单页面
  PAGE_MAIN,            // 主页面（练习界面）
  PAGE_STATS,           // 统计页面
  PAGE_SETTINGS,        // 设置页面
  PAGE_CALIBRATION,     // 校准页面
  PAGE_HISTORY          // 历史页面
};

// ==================== 主菜单选项定义 ====================
enum MainMenuOption {
  MENU_START_PRACTICE,  // 开始练习
  MENU_HISTORY_DATA,    // 历史数据
  MENU_SYSTEM_SETTINGS, // 系统设置
  MENU_SENSOR_CALIBRATION, // 传感器校准
  MENU_OPTION_COUNT     // 菜单选项总数
};

// I2C时钟速度数组声明
extern const uint32_t I2C_CLOCK_SPEEDS[I2C_CLOCK_SPEEDS_COUNT];

#endif // CONFIG_H
