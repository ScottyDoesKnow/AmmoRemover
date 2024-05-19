# Remove Ammo from Dropped Guns
A Fallout 4 F4SE (Fallout 4 Script Extender) mod which removes ammo from guns dropped by NPCs and puts it in their inventory, so you don't have to loot and then drop every gun to get all the ammo. I had some trouble getting everything set up to update my mod to F4SE 0.7.1, and an example like this would have made things a lot easier. Disclaimer: I don't have much experience with C++ and I mostly just messed around until everything worked, starting from [Ryan-rsm-McKenzie/ExamplePlugin-SKSE64](https://github.com/Ryan-rsm-McKenzie/ExamplePlugin-SKSE64).

### Requirements
- Visual Studio (I used 2022, but I think anything 2013+ should work)
  - Desktop development with C++
- CMake 3.18+ (I used 3.29.3)

### Building
1. Create an empty directory, open a Command Prompt window, navigate to your directory and run the following commands:

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
4. Set f4se to build as a static library
   - Right click f4se, Properties, General, Configuration Type: Dynamic Library -> Static Library
5. Build ALL_BUILD
6. Build AmmoRemover
7. AmmoRemover.dll should end up in \<your directory>\AmmoRemover\x64\Release

### Install on Build
Modify the path in CustomMacros.props to point to your Fallout 4 directory (mind the trailing backslash), then enable the Post-Build Event on the AmmoRemover project to have it copy AmmoRemover.dll and AmmoRemoverDefaults.ini to \<your Fallout 4 directory>\Data\F4SE\Plugins on successful build.

### Notes
- DO NOT CLEAN EVER! Or you'll have to run the cmake commands again.
- The AmmoRemover project points to f4se_1_10_984.lib in Properties -> Linker -> Input, which will need to be updated for future versions of F4SE.
- The other solution configurations don't build unless you delete all the folders other than AmmoRemover and run the commands (other than git clone AmmoRemover) with different --config values. I've only tried Debug and Release.
- The cmake commands hardcode some values, so it'll break if you move or rename your directory.
- I haven't touched INSTALL or ZERO_CHECK to see what they do.