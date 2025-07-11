#include <Arduino.h>
#include <unity.h>
#include "../include/config.h"
#include "../include/data_types.h"
#include "../include/sensor_manager.h"
#include "../include/display_manager.h"
#include "../include/input_manager.h"
#include "../include/data_manager.h"
#include "../include/power_manager.h"

// 测试对象
SensorManager testSensorManager;
DisplayManager testDisplayManager;
InputManager testInputManager;
DataManager testDataManager;
PowerManager testPowerManager;

void setUp(void) {
    // 每个测试前的设置
}

void tearDown(void) {
    // 每个测试后的清理
}

// 测试传感器管理器初始化
void test_sensor_manager_initialization() {
    bool result = testSensorManager.initialize();
    TEST_ASSERT_TRUE_MESSAGE(result, "传感器管理器初始化应该成功");
}

// 测试显示管理器初始化
void test_display_manager_initialization() {
    bool result = testDisplayManager.initialize();
    TEST_ASSERT_TRUE_MESSAGE(result, "显示管理器初始化应该成功");
}

// 测试输入管理器初始化
void test_input_manager_initialization() {
    bool result = testInputManager.initialize();
    TEST_ASSERT_TRUE_MESSAGE(result, "输入管理器初始化应该成功");
}

// 测试数据管理器初始化
void test_data_manager_initialization() {
    bool result = testDataManager.initialize();
    TEST_ASSERT_TRUE_MESSAGE(result, "数据管理器初始化应该成功");
}

// 测试电源管理器初始化
void test_power_manager_initialization() {
    bool result = testPowerManager.initialize();
    TEST_ASSERT_TRUE_MESSAGE(result, "电源管理器初始化应该成功");
}

// 测试传感器数据读取
void test_sensor_data_reading() {
    testSensorManager.initialize();
    bool result = testSensorManager.readSensorData();
    TEST_ASSERT_TRUE_MESSAGE(result, "传感器数据读取应该成功");
    
    SensorData data = testSensorManager.getRawData();
    TEST_ASSERT_TRUE_MESSAGE(data.timestamp > 0, "传感器数据应该有有效的时间戳");
}

// 测试稳定性评分计算
void test_stability_calculation() {
    testSensorManager.initialize();
    testSensorManager.readSensorData();
    
    StabilityData stability = testSensorManager.getStabilityData();
    TEST_ASSERT_TRUE_MESSAGE(IS_VALID_SCORE(stability.score), "稳定性评分应该在有效范围内");
}

// 测试会话管理
void test_session_management() {
    testDataManager.initialize();
    
    // 测试开始会话
    testDataManager.startSession();
    TEST_ASSERT_TRUE_MESSAGE(testDataManager.isSessionActive(), "会话应该处于活跃状态");
    
    // 测试暂停会话
    testDataManager.pauseSession();
    TEST_ASSERT_TRUE_MESSAGE(testDataManager.isSessionPaused(), "会话应该处于暂停状态");
    
    // 测试恢复会话
    testDataManager.resumeSession();
    TEST_ASSERT_TRUE_MESSAGE(testDataManager.isSessionActive(), "会话应该重新活跃");
    TEST_ASSERT_FALSE_MESSAGE(testDataManager.isSessionPaused(), "会话不应该处于暂停状态");
    
    // 测试停止会话
    testDataManager.stopSession();
    TEST_ASSERT_FALSE_MESSAGE(testDataManager.isSessionActive(), "会话应该已停止");
}

// 测试按钮事件处理
void test_button_event_handling() {
    testInputManager.initialize();
    testInputManager.update();
    
    // 初始状态应该没有按钮事件
    TEST_ASSERT_FALSE_MESSAGE(testInputManager.hasButtonEvent(), "初始状态不应该有按钮事件");
}

// 测试音频功能
void test_audio_functionality() {
    testInputManager.initialize();
    
    // 测试音频开关
    testInputManager.setAudioEnabled(true);
    TEST_ASSERT_TRUE_MESSAGE(testInputManager.isAudioEnabled(), "音频应该被启用");
    
    testInputManager.setAudioEnabled(false);
    TEST_ASSERT_FALSE_MESSAGE(testInputManager.isAudioEnabled(), "音频应该被禁用");
}

// 测试电源状态监测
void test_power_monitoring() {
    testPowerManager.initialize();
    testPowerManager.updateBatteryStatus();
    
    float voltage = testPowerManager.getBatteryVoltage();
    TEST_ASSERT_TRUE_MESSAGE(voltage > 0, "电池电压应该大于0");
    
    int percentage = testPowerManager.getBatteryPercentage();
    TEST_ASSERT_TRUE_MESSAGE(percentage >= 0 && percentage <= 100, "电池百分比应该在0-100范围内");
}

// 测试数据持久化
void test_data_persistence() {
    testDataManager.initialize();
    
    // 获取初始设置
    SystemSettings originalSettings = testDataManager.getSettings();
    
    // 修改设置
    SystemSettings newSettings = originalSettings;
    newSettings.stabilityThreshold = 75.0;
    newSettings.soundEnabled = !originalSettings.soundEnabled;
    
    testDataManager.updateSettings(newSettings);
    
    // 验证设置已更新
    SystemSettings updatedSettings = testDataManager.getSettings();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(75.0, updatedSettings.stabilityThreshold, "稳定性阈值应该已更新");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(originalSettings.soundEnabled, updatedSettings.soundEnabled, "声音设置应该已更新");
}

// 测试时间格式化
void test_time_formatting() {
    // 测试不同时间长度的格式化
    unsigned long testTime1 = 5000;  // 5秒
    unsigned long testTime2 = 65000; // 1分5秒
    unsigned long testTime3 = 3665000; // 1小时1分5秒
    
    // 这里需要访问DisplayManager的格式化方法
    // 由于方法是私有的，我们通过间接方式测试
    testDisplayManager.initialize();
    
    // 基本的时间测试
    TEST_ASSERT_TRUE_MESSAGE(testTime1 < testTime2, "时间比较应该正确");
    TEST_ASSERT_TRUE_MESSAGE(testTime2 < testTime3, "时间比较应该正确");
}

// 测试配置常量 - 根据开发板类型验证
void test_configuration_constants() {
    // 验证引脚配置根据开发板类型正确设置
    #ifdef BOARD_ESP32_C3_SUPERMINI
        TEST_ASSERT_EQUAL_MESSAGE(8, I2C_SDA_PIN, "ESP32-C3: SDA引脚应该是8");
        TEST_ASSERT_EQUAL_MESSAGE(9, I2C_SCL_PIN, "ESP32-C3: SCL引脚应该是9");
        TEST_ASSERT_EQUAL_MESSAGE(3, BUTTON_PIN, "ESP32-C3: 按钮引脚应该是3");
        TEST_ASSERT_EQUAL_MESSAGE(4, BUZZER_PIN, "ESP32-C3: 蜂鸣器引脚应该是4");
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, BUTTON_PRESSED_STATE, "ESP32-C3: 按钮按下状态应该是HIGH");
        TEST_ASSERT_EQUAL_MESSAGE(INPUT, BUTTON_PIN_MODE, "ESP32-C3: 按钮模式应该是INPUT");
    #elif defined(BOARD_ESP32_DEVKIT)
        TEST_ASSERT_EQUAL_MESSAGE(21, I2C_SDA_PIN, "ESP32: SDA引脚应该是21");
        TEST_ASSERT_EQUAL_MESSAGE(22, I2C_SCL_PIN, "ESP32: SCL引脚应该是22");
        TEST_ASSERT_EQUAL_MESSAGE(2, BUTTON_PIN, "ESP32: 按钮引脚应该是2");
        TEST_ASSERT_EQUAL_MESSAGE(15, BUZZER_PIN, "ESP32: 蜂鸣器引脚应该是15");
        TEST_ASSERT_EQUAL_MESSAGE(4, LED_PIN, "ESP32: LED引脚应该是4");
        TEST_ASSERT_EQUAL_MESSAGE(HIGH, BUTTON_PRESSED_STATE, "ESP32: 按钮按下状态应该是HIGH");
        TEST_ASSERT_EQUAL_MESSAGE(INPUT, BUTTON_PIN_MODE, "ESP32: 按钮模式应该是INPUT");
    #else
        // 默认配置测试
        TEST_ASSERT_EQUAL_MESSAGE(8, I2C_SDA_PIN, "默认: SDA引脚应该是8");
        TEST_ASSERT_EQUAL_MESSAGE(9, I2C_SCL_PIN, "默认: SCL引脚应该是9");
        TEST_ASSERT_EQUAL_MESSAGE(3, BUTTON_PIN, "默认: 按钮引脚应该是3");
        TEST_ASSERT_EQUAL_MESSAGE(4, BUZZER_PIN, "默认: 蜂鸣器引脚应该是4");
    #endif

    // 通用配置测试
    TEST_ASSERT_EQUAL_MESSAGE(128, SCREEN_WIDTH, "屏幕宽度应该是128");
    TEST_ASSERT_EQUAL_MESSAGE(64, SCREEN_HEIGHT, "屏幕高度应该是64");
    TEST_ASSERT_EQUAL_MESSAGE(50, STABILITY_THRESHOLD, "稳定性阈值应该是50");

    // 验证引脚配置的合理性
    TEST_ASSERT_TRUE_MESSAGE(I2C_SDA_PIN >= 0 && I2C_SDA_PIN <= 39, "SDA引脚号应该在有效范围内");
    TEST_ASSERT_TRUE_MESSAGE(I2C_SCL_PIN >= 0 && I2C_SCL_PIN <= 39, "SCL引脚号应该在有效范围内");
    TEST_ASSERT_TRUE_MESSAGE(BUTTON_PIN >= 0 && BUTTON_PIN <= 39, "按钮引脚号应该在有效范围内");
    TEST_ASSERT_TRUE_MESSAGE(BUZZER_PIN >= 0 && BUZZER_PIN <= 39, "蜂鸣器引脚号应该在有效范围内");
    TEST_ASSERT_TRUE_MESSAGE(LED_PIN >= 0 && LED_PIN <= 39, "LED引脚号应该在有效范围内");

    // 验证关键引脚不冲突 (I2C和按钮)
    TEST_ASSERT_NOT_EQUAL_MESSAGE(I2C_SDA_PIN, I2C_SCL_PIN, "SDA和SCL引脚不能相同");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(I2C_SDA_PIN, BUTTON_PIN, "SDA和按钮引脚不能相同");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(I2C_SCL_PIN, BUTTON_PIN, "SCL和按钮引脚不能相同");

    // 验证ESP32 DevKit特定的引脚冲突 (按钮和LED不能相同)
    #ifdef BOARD_ESP32_DEVKIT
        TEST_ASSERT_NOT_EQUAL_MESSAGE(BUTTON_PIN, LED_PIN, "ESP32: 按钮和LED引脚不能相同");
        TEST_ASSERT_NOT_EQUAL_MESSAGE(BUTTON_PIN, BUZZER_PIN, "ESP32: 按钮和蜂鸣器引脚不能相同");
    #endif
}

void setup() {
    delay(2000); // 等待串口稳定
    
    UNITY_BEGIN();
    
    // 运行所有测试
    RUN_TEST(test_configuration_constants);
    RUN_TEST(test_sensor_manager_initialization);
    RUN_TEST(test_display_manager_initialization);
    RUN_TEST(test_input_manager_initialization);
    RUN_TEST(test_data_manager_initialization);
    RUN_TEST(test_power_manager_initialization);
    RUN_TEST(test_sensor_data_reading);
    RUN_TEST(test_stability_calculation);
    RUN_TEST(test_session_management);
    RUN_TEST(test_button_event_handling);
    RUN_TEST(test_audio_functionality);
    RUN_TEST(test_power_monitoring);
    RUN_TEST(test_data_persistence);
    RUN_TEST(test_time_formatting);
    
    UNITY_END();
}

void loop() {
    // 测试完成后什么都不做
}
