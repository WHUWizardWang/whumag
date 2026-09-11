# whumag

## 构建前须知：缺失的大文件

`resources/high_quality.rcc`（约234MB）因超过 GitHub 单文件100MB的限制，未包含在本仓库中，
需要单独获取后放到 `resources/high_quality.rcc`（与 `whumag.pro` 同级的 `resources` 目录下）。
"自动建图"功能（`AutoReferenceMap`）运行时需要通过 `QResource::registerResource()` 加载这个文件，
构建时 `whumag.pro` 的 `QMAKE_POST_LINK` 步骤会自动把它复制到可执行文件旁边；缺少此文件时该功能会
提示"找不到资源文件"，其余功能不受影响。

获取方式：TODO（待补充分发地址，例如网盘链接或 GitHub Release 附件）。
