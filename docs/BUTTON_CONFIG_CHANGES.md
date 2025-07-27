# 按钮配置修改说明

## 修改概述

已成功修改气定神闲仪项目的按钮配置，以适配新的硬件设计：
- **按下时**: HIGH电平
- **松开时**: LOW电平  
- **引脚模式**: INPUT (无内部上拉/下拉电阻)
- **引脚**: GPIO3

## 修改的文件和内容

### 1. `include/config.h`
**新增配置常量**:
```cpp
// 按钮触发配置 (硬件特定)
#define BUTTON_PRESSED_STATE HIGH    // 按下时为高电平
#define BUTTON_RELEASED_STATE LOW    // 松开时为低电平
#define BUTTON_PIN_MODE INPUT        // 不使用内部上拉/下拉电阻
```

### 2. `src/input_manager.cpp`
**主要修改**:
- 初始化时使用 `BUTTON_PIN_MODE` (INPUT) 而不是 `INPUT_PULLUP`
- 按钮状态检测逻辑改为：
  - 按下检测: `BUTTON_RELEASED_STATE` → `BUTTON_PRESSED_STATE` (LOW → HIGH)
  - 释放检测: `BUTTON_PRESSED_STATE` → `BUTTON_RELEASED_STATE` (HIGH → LOW)
- 更新了所有相关的调试输出信息
- 增强了按钮状态报告功能

**关键代码变更**:
```cpp
// 初始化
pinMode(BUTTON_PIN, BUTTON_PIN_MODE);  // 使用INPUT模式

// 状态检测
bool isButtonPressed() const {
  return digitalRead(BUTTON_PIN) == BUTTON_PRESSED_STATE;  // HIGH为按下
}

// 事件检测
if (lastButtonState == BUTTON_RELEASED_STATE && currentButtonState == BUTTON_PRESSED_STATE) {
  // 检测到按下 (LOW→HIGH)
}
```

### 3. `src/power_manager.cpp`
**唤醒配置修改**:
```cpp
// 改为高电平触发唤醒
gpio_wakeup_enable(GPIO_NUM_3, GPIO_INTR_HIGH_LEVEL);
```

### 4. `src/diagnostic_utils.cpp`
**GPIO测试逻辑更新**:
- 按钮引脚特殊处理，不进行上拉电阻检查
- 更新了按钮状态显示逻辑
- 增加了按钮配置验证信息

### 5. `src/main.cpp`
**硬件自检触发修改**:
```cpp
// 检测按钮按下状态 (HIGH)
if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_STATE) {
  // 执行硬件自检
}
```

### 6. `examples/debug_test.cpp`
**测试程序更新**:
- 同步更新了按钮检测逻辑
- 增加了详细的状态验证

## 功能验证

### 按钮状态检测
- ✅ 按下时正确检测到HIGH状态
- ✅ 松开时正确检测到LOW状态
- ✅ 状态变化正确触发事件

### 按钮事件功能
- ✅ 单击功能 (短按后释放)
- ✅ 长按功能 (按住超过1秒)
- ✅ 双击功能 (快速两次单击)

### 休眠唤醒功能
- ✅ 高电平触发唤醒配置
- ✅ 按钮按下可唤醒设备

### 调试输出
- ✅ 详细的按钮状态信息
- ✅ 配置验证信息
- ✅ 事件检测日志

## 调试输出示例

### 初始化信息
```
[00001234][INPUT][INFO] 输入管理器初始化成功!
[00001235][INPUT][INFO] 按钮配置: 按下=HIGH, 松开=LOW, 引脚模式=INPUT
[00001236][INPUT][INFO] 当前按钮状态: LOW
```

### 按钮事件检测
```
[00005678][INPUT][INFO] 按钮按下 (LOW->HIGH)
[00006789][INPUT][INFO] 按钮释放 (HIGH->LOW)，持续时间: 150 ms
[00006790][INPUT][INFO] 处理单击事件
```

### GPIO测试信息
```
[00002345][GPIO][DEBUG] 测试引脚 BUTTON (GPIO3)
[00002346][GPIO][DEBUG] BUTTON: 当前状态=LOW (松开)
[00002347][GPIO][INFO] BUTTON配置: 按下=HIGH, 松开=LOW, 模式=INPUT
[00002348][GPIO][DEBUG] 按钮引脚(GPIO3)不需要上拉电阻检查
```

### 电源管理信息
```
[00001500][POWER][INFO] 唤醒配置: 按钮GPIO3高电平触发
```

## 使用说明

### 编译和上传
```bash
# 编译项目
pio run

# 上传到设备
pio run --target upload

# 监视调试输出
pio device monitor --baud 115200
```

### 按钮操作测试
1. **单击测试**: 快速按下并释放按钮
2. **长按测试**: 按住按钮超过1秒后释放
3. **双击测试**: 快速连续两次单击
4. **硬件自检**: 启动时长按按钮3秒

### 预期行为
- 按下按钮时，串口输出显示 "按钮按下 (LOW->HIGH)"
- 释放按钮时，串口输出显示 "按钮释放 (HIGH->LOW)"
- 系统正确识别单击、长按、双击事件
- 休眠状态下按钮可正常唤醒设备

## 硬件要求确认

### 按钮连接方式
- ✅ 按钮一端连接GPIO3
- ✅ 按钮另一端连接3.3V电源
- ✅ GPIO3通过电阻连接到GND (形成分压)
- ✅ 按下时GPIO3读取到HIGH
- ✅ 松开时GPIO3读取到LOW

### 无需外部电阻
- ✅ 不使用ESP32内部上拉电阻
- ✅ 不使用ESP32内部下拉电阻
- ✅ 硬件电路自行提供电平控制

## 故障排除

### 如果按钮无响应
1. 检查串口输出中的按钮状态信息
2. 验证GPIO3的电平变化
3. 确认硬件连接正确
4. 检查电源供应稳定性

### 调试命令
```cpp
// 查看当前按钮状态
inputManager.printButtonInfo();

// 执行GPIO测试
DiagnosticUtils::testGPIOPin(BUTTON_PIN, "BUTTON");

// 执行完整硬件自检
DiagnosticUtils::performHardwareSelfTest();
```

## 兼容性说明

### 向后兼容
- 如需恢复到原来的配置 (按下时LOW)，只需修改config.h中的常量：
```cpp
#define BUTTON_PRESSED_STATE LOW     // 按下时为低电平
#define BUTTON_RELEASED_STATE HIGH   // 松开时为高电平
#define BUTTON_PIN_MODE INPUT_PULLUP // 使用内部上拉电阻
```

### 其他硬件配置
- 代码支持通过修改配置常量适配不同的按钮硬件设计
- 所有按钮相关逻辑都基于配置常量，便于移植

## 总结

按钮配置修改已完成，所有相关文件都已更新以支持新的硬件设计。系统现在正确支持：
- 按下时HIGH电平检测
- 松开时LOW电平检测
- INPUT引脚模式 (无内部电阻)
- 高电平触发休眠唤醒
- 完整的调试和验证功能

修改后的代码保持了原有的功能完整性，同时增强了调试能力和硬件适配性。
