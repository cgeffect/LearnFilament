
macos 使用cmake编译, 并且把头文件和库文件安装到当前目录脚本
```
cd filament
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../release/filament ../..
make -j12 
make install
```
#### 示例demo
ios-demo/hello-triangle:
- 是filament官方的ios/samples下的hello-triangle示例程序
- 我已经修改了Xcode的编译配置, 所有工程依赖的的资源都在ios-demo/hello-triangle文件夹下
- 在hello-triangle的deploy/ios-release/filament/include/目录下有一个bakedColor.inc文件, 这个文件不属于filament的头文件, 它是bakedColor.mat预生成的的文件, 只不过在Xcode中我只写了triangle的deploy/ios-release/filament/include/的头文件查找目录, 所有就放在了triangle的deploy/ios-release/filament/include/目录下

ios-demo/HelloCocoaPods:
- 是使用cocoapods管理filament的示例程序
- 我已经修改了cocoapods的配置, 所有工程依赖的的资源都在ios-demo/HelloCocoaPods文件夹下

#### 参考资料
https://stunlock.gg/posts/filament_offscreen_renderering/<br/>
https://www.cnblogs.com/zhyan8/p/18024343<br/>
https://zhuanlan.zhihu.com/p/617065339<br/>
https://awesometop.cn/posts/715ef45ab89444a7b7d262a674cb3305<br/>
