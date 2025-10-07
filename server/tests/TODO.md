> **📌 设计原则：轻便简洁，稳定高效**  
> 本项目是鼠标键盘模拟器服务端，避免过度设计和复杂架构。  
> 重点关注：核心功能稳定、代码简洁易维护、性能足够用。

# 单元测试计划

[x] 1. 基础设施搭建
- [x] 配置CMake测试框架
- [x] 添加Unity测试框架
- [x] 创建测试目录结构
- [x] 添加测试覆盖率工具（gcov）
- [x] 添加测试运行脚本

[x] 2. 配置模块测试 (ya_config)

[x] 3. 错误处理模块测试 (ya_error)

[x] 4. 日志模块测试 (ya_logger)

[x] 5. 崩溃转储模块测试 (ya_crash_dump)

[x] 6. 客户端管理模块测试 (ya_client_manager)

[x] 7. 服务器处理模块测试 (ya_server_handler)

[x] 8. 授权模块测试 (ya_authorize)

[x] 9. 事件处理模块测试 (ya_event)

[x] 10. HTTP接口测试 (ya_http)

[-] 11. 服务器核心测试 (ya_server) - 跳过（与全局状态耦合度过高）

[-] 12. 命令处理模块测试 (ya_server_command) - 跳过（依赖全局状态和网络）

[-] 13. 服务发现模块测试 (ya_server_discover) - 跳过（依赖网络和线程）

[-] 14. 会话管理模块测试 (ya_server_session) - 跳过（依赖全局状态和网络）

[x] 15. 工具模块测试 (ya_utils)