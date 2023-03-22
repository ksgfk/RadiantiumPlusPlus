<h1 align="center">RadiantiumPlusPlus</h1>
<p align="center">毕业设计</p>

## Introduction

RadiantiumPlusPlus是一个非常简单的光线追踪渲染器, 集美大学诚毅学院2023届本科毕业设计

:(

## Dependence

使用vcpkg作为包管理

* Embree
* oneTBB
* nlohmann-json
* spdlog
* Eigen
* OpenEXR
* stb_image
* OpenVDB

如果构建编辑器或者带预览窗口的应用需要

* glfw 3.3.8 (可选)
* glad (可选)
* imgui 1.89.1 (可选)

glad已经包含在ext/glad里了，不需要额外安装

（编辑器比较简陋，如果设计为嵌入blender会省下很多麻烦）

## Gallery

![](gallery/path_many_ball.png)

![](gallery/coffee.png)

![](gallery/staircase.png)

![](gallery/glass-of-water.png)

![](gallery/mitsuba_banner6.png)

## License

MIT
