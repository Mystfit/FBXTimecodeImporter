# FBXTimecodeImportPlugin
 Adds FBX timecode importing functionality to UE5 and associated tools for synchronizing timecodes in the sequencer.


## Installation

To install, create a folder named `FBXTimecodeImport` in either your engine plugins directory or Unreal project plugins directory and copy the contents of this repository inside or download the latest zip from the [releases section](https://github.wgtn.ac.nz/malletby/FBXTimecodeImport/releases).

## Usage

### Injecting timecode in animation sequences
Import an FBX animation asset into UE as normal. The plugin automatically will inject the local timespan from the FBX file into the custom timecode bone attributes defined in the `Bone Timecode Custom Attribute Name Settings` array in your project settings. 

The default attribute names are `TCHour`, `TCMinute`, `TCSecond`, `TCFrame`, `TCSubframe`, `TCRate`.

### Sequencer commands
![Image showing the snap section to timecode command in the Unreal sequencer window](https://media.github.wgtn.ac.nz/user/54/files/7dc6adfb-2098-4f5a-8672-65256351998c)

A new button will have been added to the toolbar of your sequencer containing timecode related commands. The command `Snap section to source timecode` will move a selected section in the sequencer to the timecode specified in its source timecode property.

## Configuration

You can stop the plugin from automatically injecting timecode attributes by disabling `Auto Inject Timecode Into Anim Sequences` in the plugin's project settings.
