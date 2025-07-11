# 气定神闲仪 (Zen-Motion Meter)

基于ESP32-C3 SuperMini的武术稳定性训练设备

## 项目概述

气定神闲仪是一款专为武术练习者设计的智能训练设备，通过实时监测身体稳定性，提供科学的训练反馈和数据记录。设备特别适用于站桩、马步、太极等传统武术动作的练习。

## 主要功能

- **实时稳定性评分**: 基于MPU6050传感器的6轴运动检测，实时计算稳定性评分(0-100分)
- **破定提醒**: 当稳定性低于设定阈值时，通过蜂鸣器发出提醒
- **练习时长记录**: 自动记录每次练习的时长和累计时长
- **数据统计**: 按天统计练习数据，支持历史数据查看
- **智能校准**: 自动传感器校准功能，确保测量精度
- **电源管理**: 智能休眠模式，延长电池续航时间

## 硬件配置

### 主要组件
- **主控**: ESP32-C3 SuperMini
- **传感器**: MPU6050 (6轴陀螺仪+加速度计)
- **显示**: 0.96寸OLED显示屏 (SSD1306驱动)
- **交互**: 单按钮控制
- **反馈**: 蜂鸣器
- **电源**: 锂电池供电

### 引脚配置
```
SDA: GPIO 8
SCL: GPIO 9
按钮: GPIO 3
蜂鸣器: GPIO 4
```

## 软件架构

### 模块化设计
- **SensorManager**: 传感器数据采集和稳定性算法
- **DisplayManager**: OLED显示和UI管理
- **InputManager**: 按钮输入和蜂鸣器控制
- **DataManager**: 数据存储和会话管理
- **PowerManager**: 电源管理和休眠控制

### 核心算法
稳定性评分基于以下参数计算：
- 加速度偏差 (与重力1g的偏差)
- 角速度幅值
- 历史数据方差
- 综合加权评分

## 使用说明

### 基本操作
- **单击**: 开始/暂停/恢复练习
- **长按**: 停止练习/进入菜单/退出菜单
- **双击**: 开始传感器校准

### 显示界面
- **主页面**: 显示实时稳定性评分、练习时长、累计时长
- **统计页面**: 显示今日练习统计数据
- **设置页面**: 显示系统设置和电池状态
- **校准页面**: 显示传感器校准状态
- **历史页面**: 显示历史练习记录

### 练习流程
1. 设备开机后自动进入主页面
2. 将设备佩戴在腰部(丹田位置)
3. 单击按钮开始练习
4. 保持稳定姿势，观察实时评分
5. 听到蜂鸣器提醒时调整姿势
6. 长按按钮停止练习

## 开发环境

### 必需工具
- PlatformIO IDE
- ESP32-C3开发环境
- Git

### 依赖库
- MPU6050库 (electroniccats/MPU6050@^1.0.0)
- U8g2库 (olikraus/U8g2@^2.35.19) - 支持中文显示
- Bounce2库 (thomasfredericks/Bounce2@^2.71) - 专业按钮防抖
- Wire库 (I2C通信)
- EEPROM库 (数据存储)

### 编译和上传

#### 支持的开发板环境
- **esp32-c3-devkitm-1**: ESP32-C3 SuperMini (默认)
- **esp32dev**: ESP32 DevKit
- **esp32-c3-release**: ESP32-C3 生产版本
- **esp32dev-release**: ESP32 DevKit 生产版本

```bash
# 克隆项目
git clone <repository-url>
cd zenMotionMeter

# ESP32-C3 SuperMini (默认环境)
pio run -e esp32-c3-devkitm-1
pio run -e esp32-c3-devkitm-1 --target upload

# ESP32 DevKit
pio run -e esp32dev
pio run -e esp32dev --target upload

# 生产版本 (禁用调试)
pio run -e esp32-c3-release --target upload

# 监视串口输出
pio device monitor --baud 115200
```

### 测试
```bash
# 运行单元测试
pio test

# 运行特定测试
pio test -f test_modules
```

## 配置说明

### 多环境引脚配置

#### ESP32-C3 SuperMini
```cpp
// 引脚配置
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define BUTTON_PIN 3
#define BUZZER_PIN 4
#define LED_PIN 2

// 按钮配置
#define BUTTON_PRESSED_STATE HIGH    // 按下时高电平
#define BUTTON_PIN_MODE INPUT        // 无内部电阻
```

#### ESP32 DevKit
```cpp
// 引脚配置
#define I2C_SDA_PIN 21              // ESP32默认I2C引脚
#define I2C_SCL_PIN 22
#define BUTTON_PIN 2
#define BUZZER_PIN 15
#define LED_PIN 4                   // LED指示灯

// 按钮配置
#define BUTTON_PRESSED_STATE HIGH    // 按下时高电平
#define BUTTON_PIN_MODE INPUT        // 不使用内部上拉
```

### 系统配置参数
```cpp
// 稳定性配置
#define STABILITY_THRESHOLD 50      // 破定提醒阈值
#define STABILITY_WINDOW_SIZE 20    // 滑动窗口大小

// 时间配置
#define DEFAULT_PRACTICE_TIME 300000  // 默认5分钟
#define SLEEP_TIMEOUT 300000         // 5分钟无操作休眠
```

### 系统设置
可通过菜单界面调整的设置：
- 稳定性阈值 (30-80)
- 声音开关
- 显示亮度
- 默认练习时长
- 自动休眠开关

## 数据格式

### 练习会话数据
```cpp
struct PracticeSession {
  unsigned long startTime;     // 开始时间
  unsigned long duration;      // 持续时间
  float avgStability;          // 平均稳定性
  float maxStability;          // 最高稳定性
  int breakCount;              // 破定次数
  bool completed;              // 是否完成
};
```

### 每日统计数据
```cpp
struct DailyStats {
  unsigned long totalTime;     // 总练习时间
  int sessionCount;            // 练习次数
  float avgStability;          // 平均稳定性
  float bestStability;         // 最佳稳定性
  int totalBreaks;             // 总破定次数
};
```

## 故障排除

### 常见问题
1. **传感器初始化失败**
   - 检查I2C连接
   - 确认MPU6050电源供应
   - 重新校准传感器

2. **显示屏无显示**
   - 检查OLED连接
   - 确认I2C地址(0x3C)
   - 检查电源电压

3. **按钮无响应**
   - 检查按钮连接
   - 确认上拉电阻
   - 检查防抖设置

4. **蜂鸣器无声音**
   - 检查蜂鸣器连接
   - 确认音频开关状态
   - 检查PWM输出

### 调试模式
启用调试输出：
```cpp
#define DEBUG 1
```

查看调试信息：
```bash
pio device monitor --baud 115200
```

## 版本历史

### v1.0.0 (当前版本) - 2025-07-11
- ✅ **完整的系统架构**: 模块化设计，支持多环境配置
- ✅ **专业按钮处理**: 集成Bounce2库，支持单击/双击/长按
- ✅ **中文界面支持**: 使用U8g2库，完美显示中文字符
- ✅ **开机动画系统**: 4秒品牌展示和初始化进度
- ✅ **主菜单导航**: 开始练习、历史数据、系统设置、传感器校准
- ✅ **状态机管理**: 完整的状态转换和验证逻辑
- ✅ **硬件诊断**: 完整的I2C扫描、GPIO测试、内存监控
- ✅ **电源管理**: 智能电压监控和电量显示
- ✅ **数据持久化**: EEPROM存储和数据管理
- ✅ **性能监控**: 实时内存统计和性能分析
- ✅ **多平台支持**: ESP32 DevKit + ESP32-C3 SuperMini

## 贡献指南

欢迎提交Issue和Pull Request来改进项目。

### 开发规范
- 遵循现有代码风格
- 添加适当的注释
- 编写单元测试
- 更新文档

## 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 项目Issues页面
- 邮箱: [your-email@example.com]

---

**注意**: 本设备仅用于武术训练辅助，不能替代专业指导。使用时请注意安全。
