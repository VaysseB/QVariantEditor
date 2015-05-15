#!/bin/bash


lupdate -source-language en_GB -target-language fr_FR -ts qve_fr_FR.ts -recursive -pro QVariantEditor.pro

linguist qve_fr_FR.ts

lrelease qve_fr_FR.ts
