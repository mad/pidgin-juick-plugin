<meta http-equiv="content-type" content="text/html; charset=utf-8" />

[![www](http://img129.imageshack.us/img129/203/pidginavatar.th.png)](http://img129.imageshack.us/img129/203/pidginavatar.png)
[![www](http://img263.imageshack.us/img263/3552/pidginwin.th.png)](http://img263.imageshack.us/img263/3552/pidginwin.png)

# Установка:

Установить плагин можно одним из способов:

- Скачать архив:

        wget http://github.com/mad/pidgin-juick-plugin/tarball/master
        tar xzf mad-pidgin-juick-plugin-*.tgz
        cd mad-pidgin-juick-plugin*
        cp juick.so ~/.purple/plugins/

	 для пользователей Windows, скопируйте `juick.dll` в папку
	`%APPDATA%\.purple\plugins\`

- Создать локальный репозиторий:

        git clone git://github.com/mad/pidgin-juick-plugin.git
        cd pidgin-juick-plugin
        cp juick.so ~/.purple/plugins/

- Для пользователей gentoo:

    С помощью [ebuild](http://github.com/bobrik/callisto/blob/4dc73a3b9c435d5233a7b546cf7a94d03f0f04cb/x11-plugins/pidgin-juick-plugin/pidgin-juick-plugin-9999.ebuild)

    или через оверлей callisto, подключить оверлей можно так:

        layman -o http://bobrik.name/gentoo/overlays.xml -f -a callisto

Для активации плагина откройте окно пиджина, и нажмите Ctrl+U, появится окно
со списком плагинов - напротив Juick 0.1 поставьте галочку.

# Компиляция плагина:

Для того что бы скомпилировать плагин, наберите (в каталоге с плагином):

    make

если всё пройдёт удачно, то в текущем каталоге появится файл juick.so

#  Примечание:

Плагин поддерживает отображение аватарок - по умолчанию отключены (в
настройках плагина) из за существенной паузы при получения сообщений с не
загруженными аватарками.

`\C-r` - номер (id) последнего ответа

`\C-s` - последние 10 сообщений

