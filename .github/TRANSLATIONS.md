## Translate QontrolPanel

### Install Qt linguist

Please follow qt installation in build guide to install Qt linguist.

### Testing your changes

You can override the qm files in your QSS install (default: `%localappdata%\programs\QontrolPanel\bin\i18n`) with `file` -> `release as...`

### Validating your changes

Once satisfied with your changes, only commit the `.ts` file.  
On changes merged, a workflow will handle the compiled translation generation.

Users will be able to update translation within the program with those newly generated ones.
