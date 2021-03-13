# bigdou

```p
OMNeT++代码，版本551
inet版本3.6.6，simulte版本1.1.0，veins版本5.0
```

```p
host
host.周期性生成数据包，附带源信息，群发
host.收到群发，取出srcip，记为邻居，更新邻居
host.周期性上报，将邻居信息写入文本
```


```p
eNB
eNB.读取文本，生成拓扑，由拓扑计算match_bc，数据由match_bc方案下发
eNB.下发消息，包含链路
host.收到消息，按路径发送给下一个节点
```
