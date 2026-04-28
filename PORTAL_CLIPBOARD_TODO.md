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

## 参考
- InputLeap PR #2366（主力蓝本：RemoteDesktop + GDBus clipboard，KDE 实测通过）
- Deskflow PR #9431（EiClipboard 内存存储层可用）
- libportal PR #214（远期：合并后可替换 GDBus 为 libportal API）
