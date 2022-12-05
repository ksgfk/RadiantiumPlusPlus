#pragma once

#include "types.h"

namespace Rad {

/**
 * @brief 日志
 */
class RAD_EXPORT_API Logger {
 public:
  /**
   * @brief 应该在程序开始时调用来初始化全局日志记录
   */
  static void Init();
  /**
   * @brief 应该在程序结束时调用来关闭日志记录
   */
  static void Shutdown();
  /**
   * @brief 获取全局日志记录器 
   */
  static spdlog::logger* Get();
  /**
   * @brief 获取分类的日志记录器。建议缓存获取的记录器，以避免额外开销
   * 
   * @param name 分类名
   */
  static Share<spdlog::logger> GetCategory(const std::string& name);
  /**
   * @brief 设置全局日志记录器的记录等级
   */
  static void SetDefaultLevel(spdlog::level::level_enum level);
};

}  // namespace Rad
