# QVariantEditor

Edit variable/file made of QVariant.

## Features

*   Navigation and edition through tree interface.
*   Edit multiple files.
*   Create from scratch new files, or edit existing.
*   Reuse: you can extract the qvariant tree widget to your application.

## Limit

*   The root value must be a QVariantList.
*   The code doesn't follow strictly Qt's rules (I try though).
*   As the editor is build, it only supports about Qt qvariant predefined type. Any types that are declared with Q_DECLARE_METATYPE or Q_OBJECT cannot be known by the app, but with small adaptation, I can make it adaptable for your types.


## Project

*   It's a small project, which aim is to help me in my everyday work.
*   My first goal isn't to made the ultimate editor, but if someone wants to extend it, please be free to do so.

## License

I included the elementary-xfce icon theme as fallback if no theme is found on the system.
It's license in GPL.