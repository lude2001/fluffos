# 紧急 #
## vm/internal/simul_efun.cc ##
- init_simul_efun
  在 strcat(buf, ".c") 中可能溢出，更改为 strncat!!!
  搜索完整源代码以查找其他出现
- 可移植性问题
    添加 gnulib 模块

# 有时间 #
## 文档 ##
- 完成 docs/efun/contrib [packages/contrib]
- 检查 efun::shadowp/efun::query_shadowing
- 扩展 efun:implode 的文档（缺少三参数形式）
- efun::defer 的文档缺失

## packages/contrib/contrib.cc ##
network_stats
- 使最大端口数可配置？
  - 如果是，查找可能的缓冲区溢出!!!

query_num
- 重写代码？使用 goto

string_difference
- 重命名？有不同种类的字符串差异...

## packages/core/ed.cc ##
- 对于新 API 的写操作，缺少 'check_valid_path(...)'。

## packages/core/file.cc ##
- 如果可用，添加对 'lstat' 的支持
  - efun::get_dir
  - efun::file_size
  - efun::stat / 新 efun::lstat ？
- 移除 master::valid_link ？
  
  efun::link 在内部使用 efun::rename 及其继承的安全检查
  对 master::valid_write 进行双重检查，或者将
  master::valid_link 简化为一个简单开关：mudlib _一般_ 允许/禁止
  链接
- 替代方案：
  从 efun::link 中移除对 efun::rename 的依赖，并在所有相关函数中正确支持
  链接

## 路径处理 ##
- 简化路径处理：
  
  有数十个地方将路径转换为某种规范形式。
  确保每个提供的路径，无论是由 mudlib、运行时配置还是
  驱动程序本身生成的，都是规范的，然后从那里工作...

## vm/internal/simulate.cc ##
- give_uid_to_object
  在调用 master::creator_file 后双重检查 ```!ret```。
