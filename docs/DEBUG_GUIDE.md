# 气定神闲仪调试指南

## 概述

本文档详细说明了气定神闲仪项目的调试系统使用方法，特别针对OLED显示屏无显示问题的诊断和解决。

## 调试系统特性

### 1. 多级调试输出
- **ERROR**: 严重错误，影响系统功能
- **WARN**: 警告信息，可能影响性能
- **INFO**: 一般信息，系统状态
- **DEBUG**: 调试信息，详细执行过程
- **VERBOSE**: 详细信息，包含所有细节

### 2. 模块化日志
每个日志条目包含：
- 时间戳 (毫秒)
- 模块名称
- 日志级别
- 消息内容

格式: `[时间戳][模块][级别] 消息内容`

### 3. 性能监控
- 函数执行时间测量
- 内存使用情况监控
- 系统资源统计

## 启用调试功能

### 1. 编译时配置
在 `include/config.h` 中确保以下设置：
```cpp
#define DEBUG 1
#define DEBUG_LEVEL DEBUG_LEVEL_DEBUG
```

### 2. PlatformIO配置
在 `platformio.ini` 中确保以下设置：
```ini
monitor_speed = 115200
build_flags = 
    -DDEBUG=1
    -DDEBUG_LEVEL=4
    -DCORE_DEBUG_LEVEL=4
```

### 3. 查看调试输出
```bash
# 编译并上传
pio run --target upload

# 监视串口输出
pio device monitor --baud 115200

# 或者使用过滤器
pio device monitor --baud 115200 --filter esp32_exception_decoder
```

## OLED显示屏问题诊断

### 自动诊断流程

系统启动时会自动执行以下诊断步骤：

1. **I2C总线扫描**
   - 扫描地址范围 0x08-0x77
   - 识别已知设备 (OLED: 0x3C, MPU6050: 0x68)
   - 报告发现的设备数量

2. **OLED设备通信测试**
   - 测试基本I2C通信
   - 发送SSD1306命令
   - 验证设备响应

3. **多时钟速度测试**
   - 尝试 100kHz, 400kHz, 1MHz
   - 找到工作的时钟速度
   - 自动切换到最佳速度

4. **GPIO引脚验证**
   - 检查SDA(GPIO9)和SCL(GPIO8)状态
   - 验证上拉电阻存在
   - 测试引脚功能

5. **电源状态检查**
   - 测量供电电压
   - 验证电压范围 (3.0V-3.6V)
   - 报告电源状态

### 手动硬件自检

在设备启动时长按按钮3秒可触发完整硬件自检：

```
[00001234][MAIN][INFO] 检测到按钮按下，启动硬件自检...
[00004567][SELFTEST][INFO] === 硬件自检开始 ===
[00004568][SELFTEST][INFO] 1. 电源测试...
[00004569][SELFTEST][INFO] 2. GPIO引脚测试...
[00004570][SELFTEST][INFO] 3. I2C总线测试...
[00004571][SELFTEST][INFO] 4. 传感器连接测试...
[00004572][SELFTEST][INFO] 5. 显示屏连接测试...
[00004573][SELFTEST][INFO] 6. 输入输出测试...
[00004574][SELFTEST][INFO] === 硬件自检完成 ===
[00004575][SELFTEST][INFO] 结果: 通过
```

## 常见问题诊断

### 1. OLED无显示

**症状**: 显示屏完全黑屏，无任何显示

**诊断步骤**:
```
查看串口输出中的以下信息：
[时间戳][I2C][INFO] === I2C总线扫描 ===
[时间戳][I2C][INFO] 发现设备: 0x3C
[时间戳][I2C][INFO]   -> OLED显示屏 (SSD1306)
```

**可能原因和解决方案**:

1. **I2C连接问题**
   ```
   [时间戳][I2C][ERROR] 未发现任何I2C设备!
   ```
   - 检查SDA(GPIO9)和SCL(GPIO8)连接
   - 确认接线正确且牢固
   - 检查面包板连接

2. **电源问题**
   ```
   [时间戳][POWER][ERROR] 电压过低! 可能导致设备不稳定
   ```
   - 检查3.3V电源供应
   - 确认电源容量足够
   - 测量实际电压

3. **上拉电阻缺失**
   ```
   [时间戳][GPIO][WARN] I2C引脚缺少上拉电阻，可能影响通信
   ```
   - 添加4.7kΩ上拉电阻到SDA和SCL
   - 检查现有上拉电阻值

4. **设备地址错误**
   ```
   [时间戳][I2C][ERROR] 设备0x3C: 地址发送时收到NACK
   ```
   - 确认OLED设备地址为0x3C
   - 检查设备是否支持该地址

### 2. I2C通信错误

**症状**: 设备被检测到但通信失败

**诊断输出**:
```
[时间戳][I2C][ERROR] 设备0x3C: 数据发送时收到NACK
[时间戳][OLED][ERROR] OLED命令发送失败，错误代码: 3
```

**解决方案**:
1. 降低I2C时钟速度
2. 检查信号完整性
3. 确认设备工作状态

### 3. 初始化失败

**症状**: SSD1306初始化返回失败

**诊断输出**:
```
[时间戳][DISPLAY][WARN] SSD1306初始化失败 (尝试 1)
[时间戳][DISPLAY][ERROR] OLED显示屏初始化失败!
```

**解决方案**:
1. 系统会自动重试3次
2. 尝试不同的I2C时钟速度
3. 检查电源稳定性
4. 重新上电复位

## 调试命令和宏

### 常用调试宏
```cpp
// 基本调试输出
DEBUG_INFO("MODULE", "信息: %s", message);
DEBUG_ERROR("MODULE", "错误: %d", errorCode);
DEBUG_WARN("MODULE", "警告: %.2f", value);

// 性能测量
PERFORMANCE_START("function_name");
// ... 执行代码 ...
PERFORMANCE_END("function_name");

// 内存监控
DEBUG_MEMORY();

// 系统信息
DEBUG_SYSTEM_INFO();
```

### 诊断工具函数
```cpp
// I2C诊断
DiagnosticUtils::scanI2CBus();
DiagnosticUtils::testI2CDevice(0x3C);
DiagnosticUtils::testI2CWithDifferentSpeeds(0x3C);

// OLED诊断
DiagnosticUtils::diagnoseOLED();
DiagnosticUtils::printOLEDDiagnostics();

// 硬件自检
DiagnosticUtils::performHardwareSelfTest();

// 错误报告
DiagnosticUtils::reportError("MODULE", "错误描述");
DiagnosticUtils::reportWarning("MODULE", "警告描述");
```

## 性能分析

### 查看性能统计
系统会自动记录关键函数的执行时间：
```
[时间戳][PERF][INFO] === 性能统计 ===
[时间戳][PERF][INFO] SYSTEM_INIT: 调用1次, 总时间2500000μs, 平均2500000μs
[时间戳][PERF][INFO] OLED_INIT: 调用3次, 总时间150000μs, 平均50000μs
[时间戳][PERF][INFO] SENSOR_READ: 调用100次, 总时间500000μs, 平均5000μs
```

### 内存监控
定期内存报告：
```
[时间戳][MEMORY][INFO] === 内存信息 ===
[时间戳][MEMORY][INFO] 总堆内存: 320.0 KB
[时间戳][MEMORY][INFO] 可用堆内存: 280.5 KB
[时间戳][MEMORY][INFO] 最小可用堆内存: 275.2 KB
[时间戳][MEMORY][INFO] 最大分配块: 110.0 KB
```

## 故障排除流程

### 1. 基本检查
1. 确认调试输出正常显示
2. 检查系统初始化日志
3. 验证硬件连接

### 2. I2C问题
1. 运行I2C总线扫描
2. 检查设备地址
3. 测试不同时钟速度
4. 验证上拉电阻

### 3. OLED问题
1. 执行OLED专项诊断
2. 检查电源电压
3. 验证初始化序列
4. 测试基本显示功能

### 4. 系统问题
1. 执行完整硬件自检
2. 检查内存使用情况
3. 分析性能统计
4. 查看错误计数

## 高级调试技巧

### 1. 自定义调试级别
```cpp
// 临时提高调试级别
#undef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_VERBOSE
```

### 2. 模块特定调试
```cpp
// 只显示特定模块的调试信息
#define DEBUG_FILTER_MODULE "OLED"
```

### 3. 十六进制数据转储
```cpp
uint8_t data[] = {0x00, 0xAE, 0xD5, 0x80};
DiagnosticUtils::hexDump(data, sizeof(data));
```

### 4. 实时监控
```cpp
// 在主循环中添加
DiagnosticUtils::periodicSystemReport();
```

## 联系支持

如果问题仍然存在，请提供以下信息：
1. 完整的串口调试输出
2. 硬件连接图片
3. 使用的具体硬件型号
4. 电源规格和测量电压
5. 任何修改的代码

通过这个全面的调试系统，您应该能够快速定位和解决OLED显示屏的问题。
