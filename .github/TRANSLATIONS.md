## Translate QuickSoundSwitcher

### Install Qt linguist

Please follow qt installation in build guide to install Qt linguist.

### Testing your changes

You can override the qm files in your QSS install (default: `%localappdata%\programs\QuickSoundSwitcher\bin\i18n`) with `file` -> `release as...`

### Validating your changes

Once satisfied with your changes, select `file` -> `release as...` in Qt linguist.  
For your udpated translation to be available for users to download, you need to override `%PROJECTROOT%/i18n/compiled/your_compiled_translation.qm`.  
Make sure to also commit your changes on the `.ts` file.

see [13d50ec](https://github.com/Odizinne/QuickSoundSwitcher/commit/13d50ec91bc786d0da9aa2f32080f9f29576bb84) for reference.