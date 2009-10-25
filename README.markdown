<meta http-equiv="content-type" content="text/html; charset=utf-8" />

[![www](http://img171.imageshack.us/img171/946/juick.th.png)](http://img171.imageshack.us/img171/946/juick.png)
[![www](http://img38.imageshack.us/img38/3261/jubonologinru.th.png)](http://img38.imageshack.us/img38/3261/jubonologinru.png)


# Установка

Установить плагин можно одним из способов:

- Скачать архив:

        wget http://github.com/pktfag/pidgin-juick-plugin/tarball/master
        tar xzf pktfag-pidgin-juick-plugin-*.tar.gz
        cd pktfag-pidgin-juick-plugin*
        cp juick.so ~/.purple/plugins/

    пользователи Windows, копируйте в папку `%APPDATA%\\.purple\plugins\`

- Создать локальный репозиторий:

        git clone git://github.com/pktfag/pidgin-juick-plugin.git
        cd pidgin-juick-plugin
        cp juick.so ~/.purple/plugins/

Для активации плагина откройте окно пиджина, и нажмите Ctrl+U, появится окно
со списком плагинов - напротив Juick 0.2 поставьте галочку.

# Компиляция плагина

Для того что бы скомпилировать плагин, наберите (в каталоге с плагином):

    make

если всё пройдёт удачно, то в текущем каталоге появится файл juick.so

# Отличия от плагина mad
 1. Жуйктеги проставляются для длинных сообщений, в которых общее число тегов > 100;
 2. Проставляется реальное время каждого сообщения;
 3. Работает с ботом jubo@nologin.ru;
 4. UTF8 для внутренней обработки сообщений, в результате, правильно работает на windows;
 5. Отсутствуют аватары и горячие клавиши, но это временно :);

Тестировался плагин на pidgin 2.6, но должен работать на pidgin 2.5, если это не так, то сообщайте.

Конфликтует с плагином convcolors (Conversation Colors - Цвета беседы). Поэтому для работы плагина Juick, надо отключать плагин convcolors.

# TODO
 1. На русский язык перевести;
 2. Добавить горячие клавиши;
 3. Добавить аватары;
 4. Добавить загрузку превью картинок;

