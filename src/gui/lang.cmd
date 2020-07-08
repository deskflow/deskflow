lupdate -noobsolete gui.pro -ts gui.ts res/lang/*.ts
sed -i "s/filename\=\"\.\.\/\.\.\//filename\=\"/g" res/lang/*.ts