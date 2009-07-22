<meta http-equiv="content-type" content="text/html; charset=utf-8" />

[![www](http://img4.imageshack.us/img4/9979/newrxp.th.png)](http://img4.imageshack.us/img4/9979/newrxp.png)

# Установка:

Установить плагин можно одним из способов:

- Скачать архив:

        wget http://github.com/mad/pidgin-juick-plugin/tarball/master
        tar xzf mad-pidgin-juick-plugin-*.tgz
        cd mad-pidgin-juick-plugin*
        cp juick.so ~/.purple/plugins/

- Создать локальный репозиторий:

        git clone git://github.com/mad/pidgin-juick-plugin.git
        cd pidgin-juick-plugin
        cp juick.so ~/.purple/plugins/

- Для пользователей gentoo:

    С помощью [ebuild](http://github.com/bobrik/callisto/blob/4dc73a3b9c435d5233a7b546cf7a94d03f0f04cb/x11-plugins/pidgin-juick-plugin/pidgin-juick-plugin-9999.ebuild)

    или через оверлей callisto, подключить оверлей можно так:

        layman -o http://bobrik.name/gentoo/overlays.xml -f -a callisto

ля активации плагина откройте окно пиджина, и нажмите Ctrl+U, появится окно
со списком плагинов - напротив Juick 0.1 поставьте галочку.

# Компиляция плагина:

Для того что бы скомпилировать плагин, наберите (в каталоге с плагином):

    make

если всё пройдёт удачно, то в текущем каталоге появится файл juick.so

#  Примечание:

`\C-r` - номер (id) последнего ответа

`\C-s` - последние 10 сообщений

