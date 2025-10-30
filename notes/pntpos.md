# 单点定位函数 pntpos() 解算流程

```plantuml
@startuml
start

if (原始观测量的个数>0?) then (yes)
    :初始化变量和参数;
    :设置电离层和对流层模型;
    :**satposs**：解算卫星的位置速度、钟差钟漂和方差，取得健康标志位;
    :**estpos**：用伪距估计接收机位置;

    if (未解算出接收机位置 且 卫星数>=6 且 启用了RAIM配置项) then (yes)
        :**raim_fde**：完好性检验;
    else (no)
    endif
    
    if (卫星位置解算成功?) then (yes)
        :**estvel**：用多普勒观测量估计接收机速度;
    else (no)
    endif

    :保存卫星的方位角和仰角;

    if (卫星状态有效?) then (yes)
        :将结果保存至ssat中;
    else (no)
    endif
else (no)
endif

stop
@enduml
```

## 卫星的位置速度、钟差钟漂、方差解算函数 satposs() 流程

见 [satposs.md](satposs.md#卫星的位置速度钟差钟漂方差解算函数-satposs-流程)

## 位置估计函数 estpos() 解算流程

单点定位解算函数 pntpos 的关键步骤在函数 **estpos** 中，而函数 estpos 调用函数 **rescode** 进行残差的计算。速度估计函数 estvel 的原理与位置估计函数 estpos 类似。

```plantuml
@startuml
start

:初始化v、设计矩阵，方差向量;
:将上一历元的定位解作为本历元最小二乘迭代的初值
（若没有上一历元的定位值，则初值为0）;

while (高斯-牛顿迭代)
    :**rescode**：计算伪距残差;

    if (方程个数不小于未知数个数) then (yes)

        :用第i次测量误差的先验方差的倒数作为权重矩阵;
        :最小二乘估计;

        if (两次迭代差值的模长<0.0001) then (yes)
            :将定位解保存至sol中;
            partition **valsol**：定位解的检验 {
                :卡方检验;
                :GDOP值检验;
            }
        else (no)
        endif
    else (no)
    endif
endwhile

:迭代不收敛;

stop
@enduml
```

### 伪距残差解算函数 rescode() 流程

```plantuml
@startuml
start

:将上次迭代的定位解作为本次迭代的初值;
:ecef2pos：将上次迭代的接收机位置由地心地固坐标系转换为大地坐标系;

while (遍历每一个原始观测量)

    if (与下一个观测量不重复) then (yes)
    :satexclude：检测需排除的卫星;
    :geodist：计算近似接收机位置到卫星的卫地距，
        得到近似接收机位置指向sat号卫星的单位矢量e;

    if (迭代次数>0) then (yes)
        :satazel：计算卫星仰角，去除低仰角卫星;
        :snrmask：去除低信噪比卫星;
        :ionocorr：电离层延迟校正;
        :sat2freq：将电离层延迟校正应用于对应频率上;
        :tropcorr：对流层延迟校正;
    else (no)
    endif

    :prange：卫星码偏差修正;
    :计算第nv个伪距残差;
    :计算设计矩阵的第nv行;
    :varerr：计算伪距残差的方差;
    else (no)
    endif
endwhile

:将所有原始观测量均未用到的导航系统与GPS的钟差置0;

stop
@enduml
```

卫星码偏差修正：[https://mp.weixin.qq.com/s/nNxB-8Y6DHn-z6irCgcF1A](https://mp.weixin.qq.com/s/nNxB-8Y6DHn-z6irCgcF1A)
