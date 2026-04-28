# Deskflow Portal Clipboard for GNOME Wayland

## 目标
解决 Deskflow 在 Ubuntu 26.04 GNOME Wayland 下的剪贴板问题：
- WlClipboard（wl-copy/wl-paste）虽然能工作，但焦点抢占导致终端/编辑器场景不可用
- 用 Portal Clipboard（GDBus + RemoteDesktop session）替代，进程内调用零焦点抢占

## 架构
- Server 端：InputCapture（键鼠）+ 独立 RemoteDesktop session（剪贴板，clipboard_only=true，不连接 EIS）
- Client 端：已有 RemoteDesktop session 上增加剪贴板功能
- 剪贴板操作：GDBus 直调 org.freedesktop.portal.Clipboard，不依赖 libportal clipboard API
- 优先级：Portal Clipboard > WlClipboard > Disabled

## 状态 (2026-04-28)
- [x] Portal Clipboard 代码已编译进 deskflow-core（SHA 99912b9b）
- [x] EiScreen Wayland 后端正确加载（`using ei screen for wayland`）
- [x] `RequestClipboard` 调用成功，Session 建立成功
- [x] `SelectionOwnerChanged` / `SelectionTransfer` / `SelectionRead` 信号订阅正常
- [x] 日志确认：`wayland clipboard sharing enabled via XDG Portal`
- [x] **修复**: Client 端 SetSelection AccessDenied — libportal 0.9.1 不支持 CLIPBOARD capability，改为创建独立 clipboard-only session
- [x] **修复**: SelectionRead fd 的 EAGAIN 错误 — 将 fd 设为 blocking 模式
- [ ] 需要端到端验证：Server ↔ Client 跨机剪贴板实际数据同步（需要重启 GUI）
- [ ] 需要验证：WlClipboard fallback 在 Portal 不可用时正常工作

## 关键发现
- deskflow-core 必须通过 GUI (QProcess) 启动，或手动设置 `XDG_SESSION_TYPE=wayland`
- 终端直接启动会走 X11 后端（`using legacy x windows screen`），Portal Clipboard 不可用
- `cmake --build` 前需要 `cmake` 重新 configure，否则版本 SHA 是旧的（不影响功能，但调试时易混淆）

## 参考
- InputLeap PR #2366（主力蓝本：RemoteDesktop + GDBus clipboard，KDE 实测通过）
- Deskflow PR #9431（EiClipboard 内存存储层可用）
- libportal PR #214（远期：合并后可替换 GDBus 为 libportal API）
