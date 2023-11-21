# Lab3-2: UDP reliable service - GBN

在实验3-1（建立连接、差错检测、接收确认、超时重传）的基础上，将停等机制改成基于滑动窗口的流量控制机制，发送窗口和接收窗口采用相同大小，支持**累积确认**，完成给定测试文件的传输。

## Protocol Design

- client 可以发出 n 个未得到确认的 packet
- 每收到一个 ack，就把窗口往前移动
- server 累计确认，只确认连续接收 packet 的最大 seq
- 如果 server 收到了没有按顺序发送的 packet，就把最后一个确认的 ack 重传
- client 忽视收到的重复 ack
- client 如果一定时间内没有收到新的 ack，就重传所有未被确认的 packet