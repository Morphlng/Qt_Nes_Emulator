# Qt_Nes_Emulator

A Nes(FC) Emulator written based on Qt5

[国内仓库](https://gitee.com/Morphlng/qt-nes-emulator)

## Description

This is a NES Emulator wrote with and `ONLY WITH Qt5`, no other thrid party headers/resources need to run.

- [x] CPU
- [x] PPU (referenced from OneLoneCoder's [olcNES](https://github.com/OneLoneCoder/olcNES))
- [x] APU (I used the Simple_APU implementation from [Blargg's Nes_Snd_Emu](http://blargg.8bitalley.com/libs/audio.html), so there is no IRQ support for now)
- [x] Cartridge
- [x] Mapper 0/1/2/3/66 (Working with Mapper4, smb3 can run but others can't)
- [x] Controller 
- [x] Debugger (You can only check the CPU Register and Memory for now...And Performance alert!!!)

## How to Compile?

Easy peasy, classic three steps:

1. Install Qt v5.15.2 with MinGW 64-bit Compiler
2. Open QtCreator, choose the .pro file
3. Release Mode, Run!

## User Guide

1.  The Release version is for those who just want to play games. Packed with windeployqt on `Windows`
2.  Start-ChooseFile, Select a `.nes` file and run! (You have to **make sure the mapper your rom is using are already supported by the emulator**)
3.  Debugger-OpenDebugger, Open a window called debugger. You can check the Memory (Only the 2KB on Bus) and CPU registers. `ALERT`, you will see performance drop significantly after opening debugger.
4.  Option-toggleSound. If you want to play silently (and to improve performance a little bit), use this.
5.  Option-Save/Load。While you're running a game, `Save` will create a real time savefile, under path:`./save/Game_title/TimeStamp.sav`; click `Load` if you want to continue from a savefile
6.  Control Mapping

    | Controller No. | Up     | Down     | Left     | Right     | B   | A   | Select | Start |
    | -------------- | ------ | -------- | -------- | --------- | --- | --- | ------ | ----- |
    | Controller1    | W      | S        | A        | D         | J   | K   | LShift | Space |
    | Controller2    | Key_Up | Key_Down | Key_Left | Key_Right | Z   | X   | \[     | \]    |

## Credits

|             [HaloOrangeWang](https://github.com/HaloOrangeWang)              |                  [Javidx9](https://github.com/OneLoneCoder)                   |                 [James Athey](https://github.com/jamesathey)                 |
| :--------------------------------------------------------------------------: | :---------------------------------------------------------------------------: | :--------------------------------------------------------------------------: |
| <img width="60" src="https://avatars.githubusercontent.com/u/30406969?v=4"/> | <img width="60" src="https://avatars.githubusercontent.com/u/25419386?v=4" /> | <img width="60" src="https://avatars.githubusercontent.com/u/5478953?v=4" /> |