# ESP8266 Tmall Genie project
### 1.功能:
* 封装了连接[贝壳物联平台](www.bigiot.net)的功能，易于使用，调用三个api即可快速接入贝壳物联平台实现天猫精灵控制家里电器开关
* 目前没有添加加密登陆功能，请先使用`8282`端口,后续再添加加密登陆功能
* 登陆具有强制类型，如果设备在线，会强制将在线设备踢下后重新进行登陆


### 2.使用说明
* 打开`Arduino`->`项目`->`加载库`->`添加一个zip库`
* 使用本工程需要安装[ArduinoJson](https://github.com/bblanchon/ArduinoJson),安装方法同上
* 具体使用方法参考`examples`,example是基于`esp8266`,其他暂未测试

### 3.API 说明
```
/** 
 * @note   连接到服务器
 * @retval 连接成功返回 1, 失败 0
 */
int connect( ip,  port)
```

```
/** 
 * @note   登陆到贝壳
 * @retval 登陆成功返回 true, 失败false
 */
bool login(  deviceId,   apiKey);
```

```
/** 
 * @note   接收命令
 * @retval 返回状态码，可在下面表里查询
 */
int waiting();
```

### 4.waiting() 返回状态码说明:
| BIGIOT_LOST_CONNECTED |-1 |连接丢失|
|------------|--------- |--------  |
| BIGIOT_INVALD_COMMAND | 0 | 无效 |
| BIGIOT_STOP_COMMAND   | 1 | 关闭 |
| BIGIOT_PLAY_COMMAND   | 2 | 打开 |
