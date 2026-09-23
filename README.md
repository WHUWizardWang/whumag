# whumag

## 构建前须知：缺失的大文件

`resources/high_quality.rcc`（约234MB）因超过 GitHub 单文件100MB的限制，未包含在本仓库中，
需要单独获取后放到 `resources/high_quality.rcc`（与 `whumag.pro` 同级的 `resources` 目录下）。
"自动建图"功能（`AutoReferenceMap`）运行时需要通过 `QResource::registerResource()` 加载这个文件，
构建时 `whumag.pro` 的 `QMAKE_POST_LINK` 步骤会自动把它复制到可执行文件旁边；缺少此文件时，
"一键成图"窗口会显示提示并禁用"智能处理"按钮，其余功能不受影响。

获取方式：TODO（待补充分发地址，例如网盘链接或 GitHub Release 附件）。

## 界面与运行选项

界面基于 Qt 5.15 的 Fusion 样式加一份 QSS 主题（`resources/theme/app.qss`），主要构成：

- 主窗口：左侧工作流导轨（数据 → 预处理 → 建图 → 评估 → 导航）、命令栏、工程面板、任务与日志面板。
- 弹出窗口：数据查询、导入数据、新建工程、一键成图、整图建模、匹配导航、数据融合等，布局与样式一致。
- 主题与强调色：菜单"视图"里切换浅色 / 深色 / 跟随系统，选择保存在 QSettings。
- 命令搜索：`Ctrl+K`。
- 文字大小以磅（pt）为单位，随 Windows 的文字缩放（125%、150% 等）一起缩放。

命令行参数：

| 参数 | 作用 |
| --- | --- |
| `--offline` | 跳过数据库登录，离线工作；依赖数据库的功能会置灰 |
| `--theme=light` / `dark` / `system` | 只对本次运行覆盖已保存的主题 |
| `--console` | （仅 Windows）打开调试控制台窗口 |

新增源码放在 `src/ui`：`thememanager`（主题）、`uiicons`（内置 SVG 图标，由 `tools/gen_ui_assets.py`
根据 `tools/ui_icons.json` 生成）、`uiwidgets` / `formkit` / `resultpreviewpanel`（各窗口共用的控件与布局块）、
`uiscale.h`（随文字缩放的尺寸）。新增 `.cpp` / `.h` 后要写进 `whumag.pro`。

### 界面自动化检查（可选）

用 `qmake "DEFINES+=WHUMAG_UI_TEST"` 构建会带上测试钩子（`src/app/uitesthooks.cpp`）。运行时设置
`WHUMAG_TEST_GRAB=<目录>`，程序会把每个可见的顶层窗口保存为 PNG 后退出；`WHUMAG_TEST_PROJECT`、
`WHUMAG_TEST_QUERY`、`WHUMAG_TEST_SHOW` 等变量用来打开工程、查询窗口或指定窗体，详见该文件的注释。
正常发布构建不要带这个宏。

## 匹配导航

代码在 `src/navigation`：`navdata`（数据类型、背景场格网、文件读写、精度）、`tercom` / `iccp` / `sitan`
（三种算法，不依赖界面）、`navigationrunner`（按所选方法串联算法并保存结果，在后台线程运行）、`navplot`（结果图）、
`navigationform`（窗口）。

输入文件每行一条记录，字段可用空格、制表符、逗号或分号分隔，空行、`#` 注释和表头会跳过：

| 文件 | 内容 |
| --- | --- |
| 背景场 | `x y 磁场值` |
| INS 航迹 | `x y 实测磁场值`（其后的列忽略） |
| 真实航迹（可选） | `x y`，与 INS 航迹逐点对应，只用于计算精度 |

结果写到窗口里选择的输出文件夹：`<方法>_match.txt`（每行 `x,y,实测磁场值`）、`navigation_accuracy.txt`
（各方法相对真实航迹的均方根误差）和结果图 `<方法>.png`。
