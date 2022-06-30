
## 1 技巧

keil中将axf文件生成反汇编码

`Operations for Target...` --> `User`中，`After Build/Rebuild`下，勾选`Run #1`,`User Command`设置为`fromelf  --bin  --output=test.bin  test\test.axf`；勾选`Run #2`,`User Command`设置为`fromelf  --text  -a -c  --output=test.dis  test\test.axf`