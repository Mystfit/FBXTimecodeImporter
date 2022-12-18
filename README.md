# FBXTimecodeImportPlugin
 Adds FBX timecode importing functionality to UE5 and associated tools for synchronizing timecodes in the sequencer.


## Installation

To install, create a folder named `FBXTimecodeImport` in either your engine plugins directory or Unreal project plugins directory and copy the contents of this repository inside or download the latest zip from the [releases section](https://github.com/Mystfit/FBXTimecodeImporter/releases).

## Usage

### Setup

![image](https://user-images.githubusercontent.com/795851/206091983-89c958e3-ff52-4ab3-834e-ff50971fe92d.png)

Make sure that `Audo inject timecode into anim sequences` is checked in `Project Settings->FBX Timecode Importing`.

### Injecting timecode in animation sequences
Import an FBX animation asset into UE as normal. The plugin automatically will inject the local timespan from the FBX file into the custom timecode bone attributes defined in the `Bone Timecode Custom Attribute Name Settings` array in your project settings. The default attribute names are `TCHour`, `TCMinute`, `TCSecond`, `TCFrame`, `TCSubframe`, `TCRate`.

When you add your animation sequence to a new level sequence, it will automatically populate the `Source Timecode` property accessible when you right+click the animation section and access the section properties.

![image](https://user-images.githubusercontent.com/795851/206092634-af38b2fa-171a-47d6-b62c-d1b21ecf999b.png)


### Sequencer commands

![image](https://user-images.githubusercontent.com/795851/206092328-ab04f82f-be50-4156-90ce-620a2ff75136.png)

In the sequencer, you can snap your animation sequence to its source timecode property using the `Snap Sections to Timeline Using Source Timecode` command.

## Configuration

You can stop the plugin from automatically injecting timecode attributes by disabling `Auto Inject Timecode Into Anim Sequences` in the plugin's project settings.
