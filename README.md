## 工程介绍
使用webrtc Peer Connection传输navigator组件捕获到的桌面视频流数据并保存YUV420P，使用websocket传输rtc信令。

## 编译
```shell
mkdir build
cd build
cmake ..
make -j4
```

## 使用
运行build目录下的wasm-display-test，然后在demo5_WebSocket目录下使用浏览器打开index.html，点开始，然后点击call，wasm-display-test程序就会自动保存YUV在当前目录下，可通过YUVPlayer查看。

## 依赖
- websocketpp
- rapidjson
