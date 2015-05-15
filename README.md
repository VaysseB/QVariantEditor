# QVariantEditor

Edit file made of QVariant object.

## Features

*   Simple navigation and edition through the graphical interface.
*   Explore any file structure made of QVariant. Use double-click in column "Value", if it is a list or a collection (map/hash).
*   Edit internal values by double-clicking on it. Convert value to another QVariant's type using a pre-definied list.
*   Dump and read QVariant from any QIODevice (preferably a QFile).
*   For developers: QVariantTree is reusable as-is, if you need a tree helper class for QVariant.

## Limit

*   This editor is simple, it can only edit a small number of QVariant's types :

```
    QVariant::Invalid
    QVariant::Bool
    QVariant::UInt
    QVariant::Int
    QVariant::Double
    QVariant::String
    QVariant::List
    QVariant::Map
    QVariant::Hash
```

*   The root value must be a QVariantList.
*   The code doesn't follow strictly Qt's rules.


## Project

*   It's a small project, which aim is to help me in my everyday work.
*   My first goal isn't to made the ultimate editor, but if someone wants to extend it, please be free to do so.


