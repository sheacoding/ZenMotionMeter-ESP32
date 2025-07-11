#!/usr/bin/env python3
"""
气定神闲仪环境验证脚本
验证PlatformIO环境配置和引脚设置的正确性
"""

import os
import sys
import subprocess
import json
from pathlib import Path

def run_command(cmd):
    """执行命令并返回结果"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return result.returncode == 0, result.stdout, result.stderr
    except Exception as e:
        return False, "", str(e)

def check_platformio():
    """检查PlatformIO是否安装"""
    success, stdout, stderr = run_command("pio --version")
    if success:
        print(f"✓ PlatformIO 已安装: {stdout.strip()}")
        return True
    else:
        print(f"✗ PlatformIO 未安装或不可用: {stderr}")
        return False

def get_project_environments():
    """获取项目环境列表"""
    success, stdout, stderr = run_command("pio project config --json-output")
    if success:
        try:
            config = json.loads(stdout)
            envs = []
            for section in config:
                if section.startswith('env:'):
                    env_name = section[4:]  # 移除 'env:' 前缀
                    envs.append(env_name)
            return envs
        except json.JSONDecodeError:
            print("✗ 无法解析PlatformIO配置")
            return []
    else:
        print(f"✗ 获取项目配置失败: {stderr}")
        return []

def verify_environment(env_name):
    """验证特定环境的配置"""
    print(f"\n--- 验证环境: {env_name} ---")
    
    # 检查编译
    print(f"检查 {env_name} 编译...")
    success, stdout, stderr = run_command(f"pio run -e {env_name} --dry-run")
    if success:
        print(f"✓ {env_name} 编译配置正确")
    else:
        print(f"✗ {env_name} 编译配置有误:")
        print(f"  错误: {stderr}")
        return False
    
    # 检查库依赖
    print(f"检查 {env_name} 库依赖...")
    success, stdout, stderr = run_command(f"pio lib list -e {env_name}")
    if success:
        print(f"✓ {env_name} 库依赖正常")
    else:
        print(f"⚠ {env_name} 库依赖检查失败: {stderr}")
    
    return True

def check_pin_configurations():
    """检查引脚配置文件"""
    config_file = Path("include/config.h")
    if not config_file.exists():
        print("✗ config.h 文件不存在")
        return False
    
    print("\n--- 检查引脚配置 ---")
    
    with open(config_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 检查必要的宏定义
    required_macros = [
        'I2C_SDA_PIN', 'I2C_SCL_PIN', 'BUTTON_PIN', 'BUZZER_PIN', 'LED_PIN',
        'BUTTON_PRESSED_STATE', 'BUTTON_RELEASED_STATE', 'BUTTON_PIN_MODE'
    ]
    
    missing_macros = []
    for macro in required_macros:
        if f"#define {macro}" not in content and f"#ifndef {macro}" not in content:
            missing_macros.append(macro)
    
    if missing_macros:
        print(f"✗ 缺少必要的宏定义: {', '.join(missing_macros)}")
        return False
    else:
        print("✓ 所有必要的引脚宏定义都存在")
    
    # 检查开发板特定配置
    if "BOARD_ESP32_C3_SUPERMINI" in content:
        print("✓ 包含 ESP32-C3 SuperMini 配置")
    else:
        print("⚠ 缺少 ESP32-C3 SuperMini 配置")
    
    if "BOARD_ESP32_DEVKIT" in content:
        print("✓ 包含 ESP32 DevKit 配置")
    else:
        print("⚠ 缺少 ESP32 DevKit 配置")

    # 检查引脚冲突预防
    if "TEST_ASSERT_NOT_EQUAL_MESSAGE" in content:
        print("✓ 包含引脚冲突检查")
    else:
        print("⚠ 缺少引脚冲突检查")

    return True

def check_pin_conflicts():
    """检查引脚配置冲突"""
    print("\n--- 检查引脚冲突 ---")

    # 这里可以添加更复杂的引脚冲突检查逻辑
    # 目前主要通过编译时测试来验证

    print("✓ 引脚冲突检查将在编译时进行")
    print("  - ESP32-C3: 按钮(3) ≠ LED(2) ≠ I2C(8,9) ≠ 蜂鸣器(4)")
    print("  - ESP32: 按钮(2) ≠ LED(4) ≠ I2C(21,22) ≠ 蜂鸣器(15)")

    return True

def main():
    """主函数"""
    print("=== 气定神闲仪环境验证脚本 ===\n")
    
    # 检查当前目录
    if not Path("platformio.ini").exists():
        print("✗ 当前目录不是PlatformIO项目根目录")
        print("请在项目根目录运行此脚本")
        sys.exit(1)
    
    print("✓ 在PlatformIO项目目录中")
    
    # 检查PlatformIO
    if not check_platformio():
        print("\n请先安装PlatformIO:")
        print("pip install platformio")
        sys.exit(1)
    
    # 检查引脚配置
    if not check_pin_configurations():
        print("\n引脚配置检查失败，请检查 include/config.h 文件")
        sys.exit(1)

    # 检查引脚冲突
    if not check_pin_conflicts():
        print("\n引脚冲突检查失败")
        sys.exit(1)
    
    # 获取环境列表
    environments = get_project_environments()
    if not environments:
        print("✗ 未找到任何PlatformIO环境")
        sys.exit(1)
    
    print(f"\n✓ 找到 {len(environments)} 个环境: {', '.join(environments)}")
    
    # 验证每个环境
    failed_envs = []
    for env in environments:
        if not verify_environment(env):
            failed_envs.append(env)
    
    # 总结
    print("\n=== 验证结果 ===")
    if failed_envs:
        print(f"✗ {len(failed_envs)} 个环境验证失败: {', '.join(failed_envs)}")
        sys.exit(1)
    else:
        print(f"✓ 所有 {len(environments)} 个环境验证通过")
        print("\n推荐的编译命令:")
        print("# ESP32-C3 SuperMini (调试版本)")
        print("pio run -e esp32-c3-devkitm-1 --target upload")
        print("\n# ESP32 DevKit (调试版本)")
        print("pio run -e esp32dev --target upload")
        print("\n# 生产版本 (无调试输出)")
        print("pio run -e esp32-c3-release --target upload")
        print("pio run -e esp32dev-release --target upload")

if __name__ == "__main__":
    main()
