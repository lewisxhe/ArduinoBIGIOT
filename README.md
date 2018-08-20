# BIGIOT Platform lib
### 1.简介:
* 封装了连接[贝壳物联平台](https://www.bigiot.net)的功能，易于使用，调用三个api即可快速接入贝壳物联平台实现天猫精灵控制家里电器开关
* 登陆具有强制类型，如果设备在线，会强制将在线设备踢下后重新进行登陆
* 集成[ArduinoMD5](https://github.com/tzikis/ArduinoMD5/),支持加密登陆,目前只支持`8181`,`8282`端口
* 支持数据流上传,可单次上传多个数据流
* 支持上传经纬度数据
* 支持发送报警信息
* 自动发送心跳包
* 支持贝壳所有命令下发
* 添加多路连接Demo，使用esp8266目前测试能稳定4路连接，再多的没有测试.

### 2.使用说明
* 打开`Arduino`->`项目`->`加载库`->`添加一个zip库`
* 安装[ArduinoJson v5.13.2](https://github.com/bblanchon/ArduinoJson/releases/tag/v5.13.2),注意是v5版本
* 具体使用方法参考`examples`,example是基于`esp8266`,其他硬件暂未测试

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
bool login(  deviceId,   apiKey , usrKey);
```

```
/** 
 * @note   接收命令
 * @retval 返回状态码，可在下面表里查询
 */
int waiting();
```


```
/** 
 * @note   上传一组或者多组数据流
 * @retval None
 */
void update_data_stream(BIGIOT_Data_t *data, int len);
```



```
/** 
 * @note   上传一组定位数据
 * @retval None
 */
void update_location_data(BIGIOT_location_t *data);
```

```
/** 
 * @note   发送警报数据 
 * @retval None
 */
void send_alarm_message(alarm_method_t manner, String mes)
```


### 4.waiting() 返回状态码说明:
| BIGIOT_LOST_CONNECTED |-1 |连接丢失|
|------------|--------- |--------   |
| BIGIOT_INVALD_COMMAND | 0 | 无效  |
| BIGIOT_STOP_COMMAND   | 1 | 关闭  |
| BIGIOT_PLAY_COMMAND   | 2 | 打开  |
| BIGIOT_OFFON_COMMAND  | 3 | 电源  | 
| BIGIOT_MINUS_COMMAND  | 4 | 减小  | 
| BIGIOT_UP_COMMAND     | 5 | 上    | 
| BIGIOT_PLUS_COMMAND   | 6 | 增加  |  
| BIGIOT_LEFT_COMMAND   | 7 | 左    | 
| BIGIOT_PAUSE_COMMAND  | 8 | 暂停  | 
| BIGIOT_RIGHT_COMMAND  | 9 | 右    | 
| BIGIOT_BACKWARD_COMMAND| 10| 下一首| 
| BIGIOT_DOWN_COMMAND   | 11| 下    | 
| BIGIOT_FPRWARD_COMMAND| 12| 上一首| 

### 5.alarm_method_t 参数说明:
| BIGIOT_EMAIL_M | email |
|------------|--------- |
| BIGIOT_QQ_M | QQ |   
| BIGIOT_WEIBO_M   | weibo | 

 