# 符号同步星座与谐波自动基频（2026-11-18）

在 `auto_modulation_20261113` 候选固件上完成两项屏端行为改动，并在 65 MSPS 板卡 + UTG2062X 信号源上实机验收。改动只在 Linux 屏端（`src/ui.c`），不改 `V32ANA1!`/`A5 E0` 串口协议，不改 FPGA 侧判据。

## 一、频谱页谐波面板

谐波面板在频谱页常驻展开，默认跟随当前频谱通道；基频可信时自动下发谐波测量配置，不再需要每次换信号都进设置页。判据在 `hrm_auto_update()`：

- 取该通道可靠单音频率（`s_scope_tone_hz`，要求未削顶、非纯噪、非小信号未确认）；
- 与已下发的 FCW 比较，容差 `max(1 Hz, 0.1%)`；通道或频率变化即重新投票；
- 连续 3 帧一致且距上次下发超过 700 ms 才写 UART，避免来回抖动。

基频不可信时**不猜**：面板显示 `CHn —`，H1–H5 与 THD 2–5 全为 `—`，提示 `等待稳定基频`，频谱曲线照常绘制。越界谐波由 `harmonic_receiver.c` 的 `valid_mask` 置无效——THD 仅在五阶全部有效时才成立，不会被当作 0 计入。

| 输入 | 基频参考 | THD 2–5 | H1–H5 |
|---|---|---|---|
| CH1 5.000 MHz 正弦 | `CH1 5.000MHz` | `< NOISE` | H1 `0.0 dBc`，H2–H5 `< NOISE` |
| CH2 1.000 MHz BPSK 1 kBd | `CH2 —` | `—` | 全 `—`（等待稳定基频） |

## 二、手动 BPSK／QPSK 星座

符号同步后每个符号画一个散点（`plot_set_xy_style(plot, points=true, equal_axes=true)`，`draw_xy` 的 `xy_points` 分支画半径 2 圆点），不再是未同步的基带样点连线。锁定时标题为 `BPSK | Constellation`，单位标注 180°/90° 相位模糊。

未锁定时**必须保留可见性**：早先版本把 `plotted` 直接置 0，两条迹线一起隐藏，下图整块空白，无法区分"没信号"与"有信号但没同步"。现改为回退绘制本帧原始 I/Q 云，标题 `BPSK | I/Q 未同步`、单位 `I/Q | 原始样点 · 等待符号同步`、`SYM 0`；同时未锁定不再显示上一段的 `EVM/CFO`（陈旧值）。

| 状态 | 标题 | SYM | 读数 |
|---|---|---|---|
| BPSK 1 kBd PN7 | `BPSK \| Constellation` | 128 | EVM 1.6%，I 轴两点 |
| QPSK 1 kBd PN7 | `QPSK \| Constellation` | 128 | EVM 7.3%，四象限四点 |
| 无调制载波 | `BPSK \| I/Q 未同步` | 0 | `未锁定 · EVM --`，原始 I/Q 云可见 |

截图：`docs/screenshots/14-symbol-constellation-bpsk.png`、`15-symbol-constellation-qpsk.png`、`16-symbol-unlocked-fallback.png`。

## 三、边界与排查记录

**小调制指数 FSK 与 BPSK 在星座上不可分。** 解码器 EVM 门限 `evm < 0.19` 对应符号相位偏离 `√(2−2cosθ)` ≤ 10.9°；该区间内投影 `|cosθ| ≥ 0.982`，任何基于聚簇形状的二次判据都不会触发。要区分三/四电平 FSK，需改用瞬时频率电平直方图（3~4 个离散电平且驻留≈符号周期）或频谱边带，属新增判据，未在本次改动中引入。

**一次"AUTO 把四频移键控判成 BPSK"的排查。** 用户信号源 `FourFSK` 实际设置为 `FSK:FREQuency1=1.0005 MHz`、`F2=50 kHz`、`F3=25 kHz`，与 1 MHz 载波相差 40 倍，不是围绕载波的 FSK。频谱页实测 CH1 能量集中在 25.7 kHz、1 MHz 处无可用谱线。改成围绕载波的四电平（1.00075/1.00025/0.99975 MHz，1 kBd）后，手动 BPSK 不锁定（`SYM 0`），AUTO 判为 `FM/PM`。结论：对合格激励，固件不误判；问题在信号源参数。

## 四、构建与部署

交叉编译工具链在 WSL 发行版 `FPGA-Build-2404`，产物 `arm_build/waveform_demo`；部署后必须比对哈希，避免把旧二进制当新版。

```bash
wsl -d FPGA-Build-2404 -- bash -lc 'cd .../auto_modulation_20261113 && cmake --build arm_build -j 8'
adb -s <serial> push arm_build/waveform_demo /usr/bin/waveform_demo.fix
adb -s <serial> shell 'chmod 755 /usr/bin/waveform_demo.fix && sha256sum /usr/bin/waveform_demo.fix'
adb -s <serial> shell '/etc/init.d/S90waveform_demo stop; mv /usr/bin/waveform_demo.fix /usr/bin/waveform_demo; /etc/init.d/S90waveform_demo start'
```

本次部署校验值 `5fedee08bb65bd593fb7616643a3eaf3becc5220ba40b88bf92050b4ca741320`（本地与板端一致）。回归：`tools/test_bpsk_decoder` 板端运行通过（PN7/CFO/相位/300 ppm 时偏、重复位、断帧、无效标志）。
