<meta http-equiv="content-type" content="text/html; charset=utf-8" />

[![www](http://img171.imageshack.us/img171/946/juick.th.png)](http://img171.imageshack.us/img171/946/juick.png)
[![www](http://img38.imageshack.us/img38/3261/jubonologinru.th.png)](http://img38.imageshack.us/img38/3261/jubonologinru.png)


# Установка:

Установить плагин можно одним из способов:

- Скачать архив:

        wget http://github.com/mad/pidgin-juick-plugin/tarball/master
        tar xzf pktfag-pidgin-juick-plugin-*.tgz
        cd pktfag-pidgin-juick-plugin*
        cp juick.so ~/.purple/plugins/

    пользователи Windows, копируйте в папку `%APPDATA%\.purple\plugins\`

- Создать локальный репозиторий:

        git clone git://github.com/pidgin/pidgin-juick-plugin.git
        cd pidgin-juick-plugin
        cp juick.so ~/.purple/plugins/

Для активации плагина откройте окно пиджина, и нажмите Ctrl+U, появится окно
со списком плагинов - напротив Juick 0.1 поставьте галочку.

# Компиляция плагина:

Для того что бы скомпилировать плагин, наберите (в каталоге с плагином):

    make

если всё пройдёт удачно, то в текущем каталоге появится файл juick.so

