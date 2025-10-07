#ifndef YA_POWER_SCRIPTS_H
#define YA_POWER_SCRIPTS_H

#ifdef __cplusplus
extern "C" {
#endif

// 运行电源脚本（poweroff/restart/sleep），返回 system() 返回码
int ya_run_power_script(const char *script_basename);

#ifdef __cplusplus
}
#endif

#endif // YA_POWER_SCRIPTS_H
