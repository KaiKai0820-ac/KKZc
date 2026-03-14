# KKZc - 夸克网盘批量转存工具

一个基于 C++23 + wxWidgets 的桌面应用，用于将夸克网盘分享链接中的文件批量转存到自己的网盘目录。

## 功能特性

- 批量转存：多线程并发处理多个分享链接（1-16 线程可调）
- 智能去重：自动跳过目标目录中已存在的文件，以及历史已转存的链接
- 链接筛选：支持关键词搜索和 AI/非AI 内容分类过滤
- 速率控制：5 档限速可选（不限速 / 20 / 30 / 10 / 5 次/分钟）
- 目录浏览：内置夸克网盘目录浏览器，支持新建文件夹
- 会话日志：自动记录转存结果，支持导出日志

## 快速开始

### 依赖

| 库 | 版本 | 用途 |
|---|---|---|
| wxWidgets | 3.2.6 | GUI 框架 |
| libcurl | 8.7.1 | HTTP 请求 |
| nlohmann/json | 3.11.3 | JSON 解析 |

项目使用 CMake + vcpkg 管理依赖，所有库均为静态链接。

### 构建

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## 使用说明

### 1. 准备 Cookie

程序通过 Cookie 验证你的夸克网盘账号身份。Cookie 可通过以下方式提供（按优先级排序）：

1. 项目根目录下的 `cookies.json`
2. `config/quark_config.json`
3. 在程序界面手动粘贴

**Cookie 获取方式：**

1. 浏览器登录 [夸克网盘](https://pan.quark.cn)
2. 按 F12 打开开发者工具 → Network 标签
3. 刷新页面，找到任意请求，复制请求头中的 `Cookie` 值

**cookies.json 格式：**

```json
{
  "cookies": "key1=value1; key2=value2; key3=value3"
}
```

也支持数组格式：

```json
{
  "cookies": ["key1=value1", "key2=value2", "key3=value3"]
}
```

### 2. 准备 links.json

`links.json` 是分享链接的数据源，放在项目根目录下。格式为 JSON 数组：

```json
[
  {
    "日期": "2026-03-13",
    "名称": "资源名称",
    "夸克网盘": "https://pan.quark.cn/s/abc123def456",
    "百度网盘": "https://pan.baidu.com/s/xyz789"
  },
  {
    "日期": "2026-03-12",
    "名称": "另一个资源",
    "夸克网盘": "https://pan.quark.cn/s/ghi789jkl012",
    "百度网盘": ""
  }
]
```

| 字段 | 说明 | 必填 |
|------|------|------|
| `日期` | 日期标识 | 否 |
| `名称` | 资源显示名称 | 否 |
| `夸克网盘` | 夸克分享链接 | 是 |
| `百度网盘` | 百度分享链接（仅记录，不使用） | 否 |

> 链接支持带提取码格式：`https://pan.quark.cn/s/abc123?pwd=xxxx`

### 3. 运行

1. 启动程序，输入或加载 Cookie
2. 点击「登录验证」确认账号
3. 选择网盘目标保存路径
4. 加载 links.json，按需筛选
5. 勾选要转存的链接，点击「开始转存」

## 持久化文件

| 文件 | 说明 |
|------|------|
| `done_urls.txt` | 已完成的链接记录，下次加载时自动跳过 |
| `transfer_log_*.txt` | 每次转存会话的详细日志 |

## 项目结构

```
src/
├── main.cpp                 # 程序入口
├── api/
│   ├── QuarkApi.h/cpp       # 夸克网盘 API 封装
│   └── CurlInit.h           # libcurl 初始化
├── model/
│   ├── LinkItem.h           # 链接数据模型
│   └── LinkStore.h/cpp      # 链接集合（SAX 流式解析）
├── worker/
│   ├── TransferWorker.h/cpp # 转存执行器
│   ├── ThreadPool.h         # 线程池
│   └── RateLimiter.h        # 速率限制器
└── gui/
    ├── MainFrame.h/cpp      # 主窗口
    ├── VirtualLinkList.h/cpp# 虚拟列表控件
    └── QuarkDirDialog.h/cpp # 目录浏览对话框
```

## License

MIT
