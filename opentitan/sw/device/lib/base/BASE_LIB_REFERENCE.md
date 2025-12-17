# OpenTitan Base Library - 漏洞 POC 开发参考手册

## 📌 文档用途

**本文档专为漏洞验证 Agent 生成 POC 代码提供参考**

- ✅ **优先使用 base 库**：base 库提供底层、轻量级接口，适合构造精细化漏洞利用场景
- ⚠️ **避免使用 DIF 库**：DIF 库封装较重、灵活性不足，不适合 POC 开发
- 🎯 **快速定位**：本文档按功能分类，便于快速找到所需接口

---

## 🔍 快速索引

Browser使用场景快速跳转：

- **[BUILD 文件配置](#-build-文件配置)** - ⭐ **必读：依赖配置最佳实践**
- **[硬件直接访问](#-硬件直接访问)** - MMIO、CSR 寄存器操作
- **[位域操作](#-位域操作)** - 寄存器位提取与设置
- **[内存操作](#-内存操作)** - 读写、复制、比较
- **[加固机制](#-加固机制绕过)** - 绕过安全检查的关键接口
- **[辅助工具](#-辅助工具)** - CRC、数学、错误处理
- **[POC 开发指南](#-poc-开发指南)** - 最佳实践和代码示例

---

## ⚙️ BUILD 文件配置

### 标准依赖模板 ⭐ 必须严格遵循

**适用于所有 base 库 POC 开发**：

```python
load("//rules/opentitan:defs.bzl", "fpga_params", "opentitan_test")

opentitan_test(
    name = "{module_name}_poc_test",
    srcs = ["{module_name}_poc_test.c"],
    exec_env = {
        "//hw/top_earlgrey:sim_verilator": None,
        "//hw/top_earlgrey:sim_dv": None,
    },
    fpga = fpga_params(
        changes_otp = False,  # 如果修改 OTP 则设为 True
    ),
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",  # 提供 TOP_EARLGREY_*_BASE_ADDR
        "//hw/top:{module_name}_c_regs",              # 提供 {module_name}_regs.h
        "//sw/device/lib/base:mmio",                  # MMIO 操作
        "//sw/device/lib/base:bitfield",              # 位域操作（可选）
        "//sw/device/lib/runtime:log",                # LOG_INFO 日志
        "//sw/device/lib/testing/test_framework:ottf_main",  # 测试框架入口
    ],
)
```

### 关键依赖说明

| 依赖项 | 用途 | 是否必需 |
|-------|------|--------|
| `//hw/top_earlgrey/sw/autogen:top_earlgrey` | 提供 `TOP_EARLGREY_{MODULE}_BASE_ADDR` 常量 | ✅ 必需 |
| `//hw/top:{module}_c_regs` | 提供 `{module}_regs.h` 寄存器定义头文件 | ✅ 必需 |
| `//sw/device/lib/base:mmio` | 提供 `mmio_region_*()` 函数 | ✅ 必需 |
| `//sw/device/lib/base:bitfield` | 提供 `bitfield_*()` 函数 | ⚠️ 按需 |
| `//sw/device/lib/base:abs_mmio` | 提供 `abs_mmio_*()` 函数 | ⚠️ 按需 |
| `//sw/device/lib/base:memory` | 提供 `read_32()`, `memcpy()` 等 | ⚠️ 按需 |
| `//sw/device/lib/runtime:log` | 提供 `LOG_INFO()` 宏 | ✅ 必需 |
| `//sw/device/lib/testing/test_framework:ottf_main` | 测试框架入口 | ✅ 必需 |

### ❌ 常见错误配置

**错误 1: 使用错误的 test_framework 路径**
```python
# ❌ 错误 - 这是一个目录，不是 Bazel 目标
"//sw/device/lib/testing:test_framework"

# ✅ 正确
"//sw/device/lib/testing/test_framework:ottf_main"
```

**错误 2: 缺少寄存器头文件依赖**
```python
# ❌ 错误 - 缺少 {module}_c_regs，会导致 "{module}_regs.h not found"
deps = [
    "//hw/top_earlgrey/sw/autogen:top_earlgrey",
    "//sw/device/lib/base:mmio",
    # 缺少 "//hw/top:aes_c_regs"
]

# ✅ 正确
deps = [
    "//hw/top_earlgrey/sw/autogen:top_earlgrey",
    "//hw/top:aes_c_regs",  # 提供 aes_regs.h
    "//sw/device/lib/base:mmio",
]
```

**错误 3: 引入 DIF 库依赖（违反 base-only 原则）**
```python
# ❌ 错误 - POC 不应使用 DIF 库
deps = [
    "//sw/device/lib/dif:aes",
    "//sw/device/lib/testing:aes_testutils",
]

# ✅ 正确 - 仅使用 base 库
deps = [
    "//hw/top:aes_c_regs",
    "//sw/device/lib/base:mmio",
]
```

### 特殊情况处理

**需要额外 base 库依赖时**：
```python
deps = [
    # ... 标准依赖 ...
    "//sw/device/lib/base:abs_mmio",  # 如果使用 abs_mmio_*
    "//sw/device/lib/base:memory",    # 如果使用 read_32/write_32
    "//sw/device/lib/base:csr",       # 如果使用 CSR_READ/CSR_WRITE
]
```

**需要汇编文件时**：
```python
opentitan_test(
    name = "exploit_test",
    srcs = [
        "exploit_test.c",
        "payload.S",  # 汇编文件
    ],
    # ... deps 保持不变 ...
)
```

---

## 🔧 硬件直接访问

### abs_mmio.h - 绝对地址 MMIO ⭐ 推荐用于底层漏洞

**适用场景**：
- ✅ ROM 代码漏洞利用
- ✅ 直接修改关键寄存器
- ✅ 绕过 MMIO 区域检查
- ✅ 需要精确控制地址的场景

**核心接口**：
```c
uint32_t abs_mmio_read32(uint32_t addr);
void abs_mmio_write32(uint32_t addr, uint32_t value);
void abs_mmio_write32_shadowed(uint32_t addr, uint32_t value);  // 双写，用于关键寄存器
uint8_t abs_mmio_read8(uint32_t addr);
void abs_mmio_write8(uint32_t addr, uint8_t value);
```

**使用示例**：
```c
// 直接写入寄存器绕过权限检查
abs_mmio_write32(0x40000000, 0xDEADBEEF);

// 读取敏感寄存器
uint32_t secret = abs_mmio_read32(0x40001000);

// 绕过 shadowed 寄存器的双写保护（仅单次写入）
abs_mmio_write32(PROTECTED_REG, ATTACK_VALUE);
```

---

### mmio.h - 区域 MMIO ⭐ 推荐用于结构化访问

**适用场景**：
- ✅ 外设寄存器操作
- ✅ 批量寄存器访问
- ✅ 需要相对偏移的场景

**核心接口**：
```c
mmio_region_t mmio_region_from_addr(uintptr_t address);
uint32_t mmio_region_read32(mmio_region_t base, ptrdiff_t offset);
void mmio_region_write32(mmio_region_t base, ptrdiff_t offset, uint32_t value);
void mmio_region_write32_shadowed(mmio_region_t base, ptrdiff_t offset, uint32_t value);
```

**使用示例**：
```c
// 创建 AES 寄存器区域
mmio_region_t aes_base = mmio_region_from_addr(0x41100000);

// 启动 AES 操作
mmio_region_write32(aes_base, 0x10, 0x1);

// 读取状态
uint32_t status = mmio_region_read32(aes_base, 0x14);
```

---

### csr.h - CSR 寄存器 ⭐ 推荐用于核心控制

**适用场景**：
- ✅ 修改中断使能/优先级
- ✅ 读取/修改性能计数器
- ✅ 访问机器模式寄存器
- ✅ 触发异常/中断

**核心宏**：
```c
CSR_READ(csr, dest)           // 读取 CSR
CSR_WRITE(csr, val)           // 写入 CSR
CSR_SET_BITS(csr, mask)       // 设置位
CSR_CLEAR_BITS(csr, mask)     // 清除位
```

**使用示例**：
```c
// 禁用所有中断
CSR_CLEAR_BITS(CSR_REG_MIE, 0xFFFFFFFF);

// 读取周期计数器
uint32_t cycles;
CSR_READ(CSR_REG_MCYCLE, &cycles);

// 设置机器模式使能位
CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
```

---

## 🎯 位域操作

### bitfield.h - 位域提取与设置 ⭐ 推荐用于寄存器位操作

**适用场景**：
- ✅ 提取寄存器特定字段
- ✅ 修改寄存器部分位
- ✅ 构造特定位模式
- ✅ 分析寄存器状态

**核心接口**：
```c
// 字段操作
uint32_t bitfield_field32_read(uint32_t bitfield, bitfield_field32_t field);
uint32_t bitfield_field32_write(uint32_t bitfield, bitfield_field32_t field, uint32_t value);

// 单位操作
bool bitfield_bit32_read(uint32_t bitfield, bitfield_bit32_index_t bit_index);
uint32_t bitfield_bit32_write(uint32_t bitfield, bitfield_bit32_index_t bit_index, bool value);

// 工具函数
int32_t bitfield_find_first_set32(int32_t bitfield);         // 查找第一个设置位
int32_t bitfield_count_leading_zeroes32(uint32_t bitfield);  // 前导零计数
int32_t bitfield_popcount32(uint32_t bitfield);              // 计数 1 的个数
uint32_t bitfield_byteswap32(uint32_t bitfield);             // 字节交换
```

**使用示例**：
```c
// 提取状态寄存器的错误码字段 [15:8]
bitfield_field32_t err_field = {.mask = 0xFF, .index = 8};
uint32_t err_code = bitfield_field32_read(status_reg, err_field);

// 设置配置寄存器的使能位 [bit 0]
uint32_t config = 0;
config = bitfield_bit32_write(config, 0, true);

// 修改寄存器特定字段
uint32_t new_reg = bitfield_field32_write(old_reg, 
    (bitfield_field32_t){.mask = 0x7, .index = 4}, 0x5);
```

---

## 💾 内存操作

### memory.h - 标准内存函数 ⭐ 推荐用于内存操作

**适用场景**：
- ✅ 直接内存访问
- ✅ 缓冲区溢出测试
- ✅ 内存模式搜索
- ✅ 数据泄露验证

**核心接口**：
```c
// 底层读写（绕过类型检查）
uint32_t read_32(const void *ptr);
void write_32(uint32_t value, void *ptr);
uint64_t read_64(const void *ptr);
void write_64(uint64_t value, void *ptr);

// 标准内存函数
void *memcpy(void *dest, const void *src, size_t len);
void *memset(void *dest, int value, size_t len);
int memcmp(const void *lhs, const void *rhs, size_t len);
void *memchr(const void *ptr, int value, size_t len);

// 反向操作
int memrcmp(const void *lhs, const void *rhs, size_t len);
void *memrchr(const void *ptr, int value, size_t len);
```

**使用示例**：
```c
// 绕过类型检查直接读写内存
uint32_t val = read_32((void*)0x20000000);
write_32(0xDEADBEEF, (void*)0x20000000);

// 搜索内存中的特定值
void* found = memchr(buffer, 0x42, 1024);

// 反向比较（用于小端数值比较）
if (memrcmp(buf1, buf2, 4) == 0) {
    // 小端 uint32 相等
}
```

---

### hardened_memory.h - 加固内存操作（用于测试侧信道）

**适用场景**：
- ✅ 测试侧信道防护有效性
- ✅ 对比加固 vs 非加固实现
- ⚠️ 需要实现 `hardened_memshred_random_word()` 函数

**核心接口**：
```c
void hardened_memcpy(uint32_t *dest, const uint32_t *src, size_t word_len);
void hardened_memshred(uint32_t *dest, size_t word_len);
hardened_bool_t hardened_memeq(const uint32_t *lhs, const uint32_t *rhs, size_t word_len);
```

**使用示例**：
```c
// 对比性能差异（用于侧信道测试）
uint32_t buf1[16], buf2[16];

// 标准实现（快，可能泄露）
memcpy(buf2, buf1, 64);

// 加固实现（慢，防侧信道）
hardened_memcpy(buf2, buf1, 16);  // 注意：单位是 word（4字节）
```

---

## 🛡️ 加固机制绕过

### hardened.h - 加固检查机制 ⭐ 关键安全接口

**适用场景**：
- ✅ 理解加固检查机制
- ✅ 测试故障注入绕过
- ✅ 构造绕过加固布尔值的攻击
- ✅ 利用 launder 函数特性

**关键数据类型**：
```c
// 加固布尔类型（汉明距离 = 8）
kHardenedBoolTrue  = 0x739
kHardenedBoolFalse = 0x1D4

// 字节加固布尔
kHardenedByteBoolTrue  = 0xA5
kHardenedByteBoolFalse = 0x4B
```

**关键绕过点**：
```c
// 1. launder32 - 阻止编译器优化，但不改变值
uint32_t launder32(uint32_t val);
uintptr_t launderw(uintptr_t val);

// 2. barrier32 - 创建重排序屏障
void barrier32(uint32_t val);

// 3. 恒定时间比较（可用于构造定时攻击）
ct_bool32_t ct_seq32(uint32_t a, uint32_t b);
ct_bool32_t ct_sltu32(uint32_t a, uint32_t b);
uint32_t ct_cmov32(ct_bool32_t c, uint32_t a, uint32_t b);
```

**加固检查宏（攻击目标）**：
```c
HARDENED_CHECK_EQ(a, b)  // 双重相等检查，可尝试故障注入
HARDENED_CHECK_NE(a, b)  // 双重不等检查
HARDENED_CHECK_LT(a, b)  // 双重小于检查
HARDENED_TRAP()          // 失败时触发 unimp 指令
```

**使用示例**：
```c
// 理解加固机制
hardened_bool_t status = kHardenedBoolTrue;
if (status == kHardenedBoolTrue) {
    // 需要匹配特定位模式 0x739
}

// launder32 阻止编译器常量传播
uint32_t x = launder32(user_input);
if (x == 0) {
    HARDENED_CHECK_EQ(x, 0);  // 双重检查，可能被故障注入绕过
}

// 攻击思路：构造满足加固布尔值位模式的输入
// 例如：输入 0x739 可能绕过某些检查
```

---

### multibits.h - 多位布尔值

**功能**：多位冗余编码，防止故障注入。

**关键常量**：
```c
kMultiBitBool4True  = 0x6,     kMultiBitBool4False  = 0x9
kMultiBitBool8True  = 0x96,    kMultiBitBool8False  = 0x69
kMultiBitBool16True = 0x9696,  kMultiBitBool16False = 0x6969
kMultiBitBool32True = 0x96969696, kMultiBitBool32False = 0x69696969
```

**POC 使用**：测试多位编码抗故障能力。

---

## 🔨 辅助工具

### crc32.h - CRC32 校验

**适用场景**：
- ✅ 计算修改后的数据 CRC
- ✅ 绕过 CRC 校验
- ✅ 碰撞测试

**核心接口**：
```c
uint32_t crc32(const void *buf, size_t len);  // 一次性计算

// 增量计算
void crc32_init(uint32_t *ctx);
void crc32_add(uint32_t *ctx, const void *buf, size_t len);
uint32_t crc32_finish(const uint32_t *ctx);
```

---

### math.h - 数学函数

```c
uint64_t udiv64_slow(uint64_t a, uint64_t b, uint64_t *rem_out);
size_t ceil_div(size_t a, size_t b);
```

---

### status.h - 错误处理

**核心接口**：
```c
status_t OK_STATUS();
status_t INTERNAL();
status_t INVALID_ARGUMENT();
status_t NOT_FOUND();
bool status_ok(status_t s);
```

**使用示例**：
```c
status_t exploit() {
    if (error_condition) {
        return INTERNAL();
    }
    return OK_STATUS();
}
```

---

### macros.h - 工具宏

**常用宏**：
```c
ARRAYSIZE(array)                 // 数组大小
OT_WARN_UNUSED_RESULT           // 标记返回值必须使用
OT_SECTION(name)                // 指定代码段
OT_ADDRESSABLE_LABEL(kName)     // 创建可寻址标签
OT_SIGNED(value) / OT_UNSIGNED(value)  // 符号转换
```

---

## 📝 POC 开发指南

### 典型 POC 结构

```c
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/base/bitfield.h"

bool test_main(void) {
    // 1. 初始化目标硬件
    mmio_region_t target = mmio_region_from_addr(TARGET_BASE_ADDR);
    
    // 2. 设置漏洞触发条件
    mmio_region_write32(target, CONTROL_OFFSET, VULN_VALUE);
    
    // 3. 触发漏洞
    abs_mmio_write32(TRIGGER_ADDR, EXPLOIT_PATTERN);
    
    // 4. 验证漏洞效果
    uint32_t result = mmio_region_read32(target, STATUS_OFFSET);
    CHECK(result == EXPECTED_VALUE, "Exploit failed");
    
    return true;
}
```

---

### 常见漏洞类型对应接口

| 漏洞类型 | 推荐接口 | 说明 |
|---------|---------|-----|
| **寄存器权限绕过** | `abs_mmio_write32()` | 直接写入特权寄存器 |
| **状态机漏洞** | `mmio_region_write32()` | 特定顺序操作寄存器 |
| **时序攻击** | `ct_*` 系列函数 | 测试恒定时间实现 |
| **加固机制绕过** | `launder32()`, `HARDENED_CHECK_*` | 理解和绕过检查 |
| **内存破坏** | `write_32()`, `memcpy()` | 直接内存操作 |
| **侧信道** | `hardened_memcpy()` | 对比加固/非加固 |
| **CRC 绕过** | `crc32()` | 计算正确的 CRC |

---

### 快速参考卡片

#### 寄存器操作
```c
// 方式1: 绝对地址直接写（推荐用于底层漏洞）
abs_mmio_write32(0x40000000, value);

// 方式2: 区域 + 偏移（推荐用于结构化访问）
mmio_region_t base = mmio_region_from_addr(0x40000000);
mmio_region_write32(base, 0x10, value);

// 提取位域
bitfield_field32_t field = {.mask = 0xFF, .index = 8};
uint32_t bits = bitfield_field32_read(reg_val, field);
```

#### 内存操作
```c
// 直接读写
uint32_t val = read_32(addr);
write_32(value, addr);

// 复制/比较
memcpy(dest, src, len);
if (memcmp(buf1, buf2, len) == 0) { ... }

// 加固版本（慢但防侧信道）
hardened_memcpy(dest, src, word_len);
```

#### CSR 操作
```c
uint32_t val;
CSR_READ(CSR_REG_MSTATUS, &val);
CSR_WRITE(CSR_REG_MIE, 0x888);
CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
CSR_CLEAR_BITS(CSR_REG_MIE, 0xFFFFFFFF);
```

---

### 注意事项

⚠️ **避免使用**：
- DIF 库的高层封装（过于复杂，不灵活）
- 已弃用的 `mmio_region_nonatomic_*` 函数

✅ **推荐使用**：
- `abs_mmio_*` - 底层硬件访问
- `mmio_region_*` - 结构化 MMIO 操作
- `read_32/write_32` - 直接内存访问
- `bitfield_*` - 寄存器位操作
- `launder32` - 理解编译器行为

---

### 调试技巧

```c
// 使用 LOG 输出调试信息
LOG_INFO("Register value: 0x%08x", reg_val);

// 使用 CHECK 验证中间状态
CHECK(condition, "Unexpected state");

// 读取关键寄存器状态
uint32_t debug_val = abs_mmio_read32(DEBUG_REG_ADDR);
```

---

## 🔨 编译与测试

```bash
# 编译 POC
bazel build //path/to:poc_test

# 运行测试
bazel test //path/to:poc_test

# 禁用加固检查（观察绕过效果）
bazel build --copt=-DOT_DISABLE_HARDENING //path/to:poc_test

# 启用详细日志
bazel test //path/to:poc_test --test_output=all
```

---

## 📚 版权与许可

所有文件均为 lowRISC contributors (OpenTitan project) 版权所有，采用 Apache License 2.0 许可证。
