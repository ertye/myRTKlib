This code is forked from:
https://github.com/Kevin-QAQ/RTKLIB-trimmed/blob/rtklib_2.4.3

您需要在 Code::Blocks 项目设置中，告诉链接器去链接包含 `timeGetTime` 函数实现的库文件 **`winmm.lib`**。

具体步骤如下：

1.  打开您的 Code::Blocks 项目。
2.  点击菜单栏的 **"Project" -\> "Build options..."**。
3.  确保您在左侧选择了正确的 **目标（Target）**（例如 "Release" 或 "Debug"）。
4.  切换到 **"Linker settings"** 选项卡。
5.  在右侧的 **"Link libraries"** 或 **"Other linker options"** 区域，手动添加以下库名称：
    ```
    -l**
    ```
    或者直接输入库文件名（Code::Blocks 通常会自动添加 `-l`）：
    ```
    winmm
	ws2_32
    ```
6.  点击 **"OK"** 并重新构建（Rebuild）您的项目。
