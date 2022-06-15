Hello

## 模板测试(Stencil Testing)

​	模板测试用指定的参考值与片段在模板缓冲的模板值进行比较，若没有通过比较则舍弃片段值。模板测试在片段着色器之后，深度测试之前被执行。

> 模板测试的目的：
>
> 利用已经本次绘制的物体，产生一个区域，在下次绘制中利用这个区域做一些效果。
>
> 模板测试的有两个要点：
>
> ​	1.模板测试，用于剔除片段 
>
> ​	2.模板缓冲更新，用于更新出一个模板区域出来，为下次绘制做准备

- `glStencilMask()`模板掩码，在写入模板缓冲时会进行&操作
- `glStencilFunc(GLenum func, GLint ref, GLuint mask)`模板函数
  - `func`应用在模板值和`ref`上，判断是否通过测试
  - `mask`对模板值和`ref`进行掩码操作
- `glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)`被用来更新模板缓冲中的模板值。默认情况下`glStencilOp`是设置为`(GL_KEEP, GL_KEEP, GL_KEEP)`的，所以不论任何测试的结果是如何，模板缓冲都会保留它的值。默认的行为不会更新模板缓冲，所以如果你想写入模板缓冲的话，你需要至少对其中一个选项设置不同的值。
  - `sfail`模板测试失败时采取的行为。
  - `dpfail`模板测试通过，但深度测试失败时采取的行为。
  - `dppass`模板测试和深度测试都通过时采取的行为。

​	在代码中

- 在画地板时把模板掩码设为0，可以禁止写入模板缓冲。
- 画出物体后（所有都通过模板测试，再通过深度测试后），更新对应的模板值为1。
- 把物体放大，模板值不为1的地方通过模板测试（放大后的区域减掉了原来的物体）

## Gamma校正

