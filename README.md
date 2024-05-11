# Remove Ammo from Dropped Guns
A Fallout 4 F4SE (Fallout 4 Script Extender) mod which removes ammo from guns dropped by NPCs and puts it in their inventory, so you don't have to loot and then drop every gun to get all the ammo. I had some trouble getting everything set up to update my mod to the new F4SE 0.7.1, and an example like this would have made things a lot easier. Disclaimer: I don't have much experience with C++ and I mostly just messed around until everything worked, starting from [Ryan-rsm-McKenzie/ExamplePlugin-SKSE64](https://github.com/Ryan-rsm-McKenzie/ExamplePlugin-SKSE64).

### Requirements
- Visual Studio (I used 2022, but I think anything 2013+ should work)
  - Desktop development with C++
- CMake 3.18+ (I used 3.29.3)

### Building
1. Create an empty directory, open a Command Prompt window, navigate to it and run the following commands:

```
git clone https://github.com/ScottyDoesKnow/AmmoRemover
git clone https://github.com/ianpatt/common
git clone https://github.com/ianpatt/f4se
cmake -B common/build -S common -DCMAKE_INSTALL_PREFIX=extern common
cmake --build common/build --config Release --target install
cmake -B f4se/build -S f4se -DCMAKE_INSTALL_PREFIX=extern f4se
cmake --build f4se/build --config Release
```

2. Open \<your directory>\AmmoRemover\AmmoRemover.sln
3. Set solution configuration to Release
4. Build ALL_BUILD
5. Build AmmoRemover
6. AmmoRemover.dll should end up in \<your directory>\AmmoRemover\x64\Release

### Notes
- You can enable the Post-Build Event for AmmoRemover and modify the path to have it copied to your Fallout 4 F4SE folder automatically for faster testing.
- The other solution configurations don't build, I assume you need to run the cmake commands with different --config values for that but haven't tried.
- The cmake commands hardcode some values, so it'll break if you move or rename your directory.
- I haven't touched INSTALL or ZERO_CHECK to see what they do.